#include "lvq.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>

static float distance(const float* a, const float* b, int dim) {
    float sum = 0.0f;
    for (int i = 0; i < dim; i++) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sqrtf(sum);
}

static void adapt(float* weights, const float* input, int dim, float lr) {
    for (int i = 0; i < dim; i++) {
        float diff = input[i] - weights[i];
        float factor = 1.0f / (1.0f + 0.5f * fabsf(diff));
        weights[i] += lr * diff * factor;
    }
}

Codebook* createAndTrainCodebook(BITMAP* img, int numCodes, int bs, int passes) {
    int w = img->w, h = img->h, dim = bs * bs;
    Codebook* cb = malloc(sizeof(Codebook));
    cb->count = numCodes;
    cb->dim = dim;
    cb->blockSize = bs;
    cb->vectors = malloc(numCodes * sizeof(CodeVector));

    srand(time(NULL));

    for (int i = 0; i < numCodes; i++) {
        cb->vectors[i].weights = malloc(dim * sizeof(float));
        int x = rand() % (w - bs);
        int y = rand() % (h - bs);
        int idx = 0;
        for (int by = 0; by < bs; by++)
            for (int bx = 0; bx < bs; bx++)
                cb->vectors[i].weights[idx++] = getr(getpixel(img, x+bx, y+by)) / 255.0f;
    }

    float lr = 0.15f;
    float input[64];

    for (int p = 0; p < passes; p++) {
        for (int y = 0; y <= h - bs; y += bs) {
            for (int x = 0; x <= w - bs; x += bs) {
                int idx = 0;
                for (int by = 0; by < bs; by++)
                    for (int bx = 0; bx < bs; bx++)
                        input[idx++] = getr(getpixel(img, x+bx, y+by)) / 255.0f;

                int winner = 0;
                float best = distance(input, cb->vectors[0].weights, dim);
                for (int i = 1; i < numCodes; i++) {
                    float d = distance(input, cb->vectors[i].weights, dim);
                    if (d < best) { best = d; winner = i; }
                }
                adapt(cb->vectors[winner].weights, input, dim, lr);
            }
        }
        lr *= 0.92f;
    }
    return cb;
}

int* compressImage(BITMAP* src, BITMAP* dst, Codebook* cb) {
    int bs = cb->blockSize;
    int dim = cb->dim;
    int blocksW = (src->w + bs - 1) / bs;
    int blocksH = (src->h + bs - 1) / bs;
    int total = blocksW * blocksH;

    int* indices = malloc(total * sizeof(int));
    float input[64];
    int blockIdx = 0;

    for (int y = 0; y <= src->h - bs; y += bs) {
        for (int x = 0; x <= src->w - bs; x += bs) {
            int idx = 0;
            for (int by = 0; by < bs; by++)
                for (int bx = 0; bx < bs; bx++)
                    input[idx++] = getr(getpixel(src, x+bx, y+by)) / 255.0f;

            int best = 0;
            float bestD = distance(input, cb->vectors[0].weights, dim);
            for (int i = 1; i < cb->count; i++) {
                float d = distance(input, cb->vectors[i].weights, dim);
                if (d < bestD) { bestD = d; best = i; }
            }

            indices[blockIdx++] = best;

            idx = 0;
            for (int by = 0; by < bs; by++)
                for (int bx = 0; bx < bs; bx++) {
                    int val = (int)(cb->vectors[best].weights[idx++] * 255);
                    putpixel(dst, x + bx, y + by, makecol(val, val, val));
                }
        }
    }
    return indices;
}

void decompressImage(BITMAP* dst, Codebook* cb, const int* indices) {
    int bs = cb->blockSize;
    int blockIdx = 0;
    for (int y = 0; y <= dst->h - bs; y += bs) {
        for (int x = 0; x <= dst->w - bs; x += bs) {
            int best = indices[blockIdx++];
            int idx = 0;
            for (int by = 0; by < bs; by++) {
                for (int bx = 0; bx < bs; bx++) {
                    int val = (int)(cb->vectors[best].weights[idx++] * 255);
                    putpixel(dst, x + bx, y + by, makecol(val, val, val));
                }
            }
        }
    }
}

double computePsnr(BITMAP* orig, BITMAP* comp) {
    long long sum = 0;
    int total = orig->w * orig->h;
    for (int y = 0; y < orig->h; y++)
        for (int x = 0; x < orig->w; x++) {
            int diff = getr(getpixel(orig, x, y)) - getr(getpixel(comp, x, y));
            sum += (long long)diff * diff;
        }
    double mse = (double)sum / total;
    return (mse < 1e-10) ? 100.0 : 20.0 * log10(255.0 / sqrt(mse));
}

void freeCodebook(Codebook* cb) {
    if (!cb) return;
    for (int i = 0; i < cb->count; i++) free(cb->vectors[i].weights);
    free(cb->vectors);
    free(cb);
}

// ====================== BIT IO SAVE ======================
typedef struct {
    FILE* file;
    unsigned char buffer;
    int   bitPos;
} BitWriter;

static void bitWriterInit(BitWriter* bw, FILE* f) {
    bw->file = f;
    bw->buffer = 0;
    bw->bitPos = 0;
}

static void bitWriterWrite(BitWriter* bw, unsigned int value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        bw->buffer = (bw->buffer << 1) | ((value >> i) & 1);
        if (++bw->bitPos == 8) {
            fputc(bw->buffer, bw->file);
            bw->buffer = 0;
            bw->bitPos = 0;
        }
    }
}

static void bitWriterFlush(BitWriter* bw) {
    if (bw->bitPos > 0) {
        bw->buffer <<= (8 - bw->bitPos);
        fputc(bw->buffer, bw->file);
    }
}

int saveCompressed(const char* filename, Codebook* cb, const int* indices, int totalBlocks)
{
    FILE* f = fopen(filename, "wb");
    if (!f) return 0;

    BitWriter bw;
    bitWriterInit(&bw, f);

    // Header
    bitWriterWrite(&bw, cb->count, 16);
    bitWriterWrite(&bw, cb->blockSize, 8);
    bitWriterWrite(&bw, cb->dim, 8);

    // Codebook
    for (int i = 0; i < cb->count; i++) {
        for (int j = 0; j < cb->dim; j++) {
            unsigned char val = (unsigned char)(cb->vectors[i].weights[j] * 255.0f);
            bitWriterWrite(&bw, val, 8);
        }
    }

    // Indices
    int bitsPerIndex = 0;
    while ((1u << bitsPerIndex) < (unsigned)cb->count) bitsPerIndex++;
    for (int i = 0; i < totalBlocks; i++) {
        bitWriterWrite(&bw, indices[i], bitsPerIndex);
    }

    bitWriterFlush(&bw);
    fclose(f);
    return 1;
}