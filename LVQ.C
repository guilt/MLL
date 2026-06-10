#include "lvq.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DIM 128
#define LUMA_WEIGHT 4.0f

bool showProgress = false;
InitMode initMode = INIT_RANDOM;

// --- RGB <-> YUV ----------------------------------------------------------

static void rgbToYuv(float r, float g, float b, float* y, float* u, float* v) {
    *y = 0.299f * r + 0.587f * g + 0.114f * b;
    *u = -0.169f * r - 0.331f * g + 0.500f * b + 0.5f;
    *v =  0.500f * r - 0.419f * g - 0.081f * b + 0.5f;
}

static void yuvToRgb(float y, float u, float v, float* r, float* g, float* b) {
    *r = y + 1.402f * (v - 0.5f);
    *g = y - 0.344f * (u - 0.5f) - 0.714f * (v - 0.5f);
    *b = y + 1.772f * (u - 0.5f);
}

// --- Block helpers --------------------------------------------------------

static int blockDim(int bs, ColorMode mode) {
    return (mode == CM_YUV) ? bs * bs + (bs * bs / 4) * 2 : bs * bs;
}

static void blockToVec(BITMAP* img, int x, int y, int bs, float* out, ColorMode mode) {
    if (mode == CM_GRAY) {
        for (int by = 0; by < bs; by++)
            for (int bx = 0; bx < bs; bx++)
                out[by * bs + bx] = getr(getpixel(img, x + bx, y + by)) / 255.0f;
        return;
    }

    float rr, gg, bb, yy, uu, vv;
    for (int by = 0; by < bs; by++)
        for (int bx = 0; bx < bs; bx++) {
            int c = getpixel(img, x + bx, y + by);
            rr = getr(c) / 255.0f;
            gg = getg(c) / 255.0f;
            bb = getb(c) / 255.0f;
            rgbToYuv(rr, gg, bb, &yy, &uu, &vv);
            out[by * bs + bx] = yy;
        }
    int half = bs / 2, offY = bs * bs;
    for (int by = 0; by < half; by++)
        for (int bx = 0; bx < half; bx++) {
            float su = 0, sv = 0;
            for (int dy = 0; dy < 2; dy++)
                for (int dx = 0; dx < 2; dx++) {
                    int c = getpixel(img, x + bx * 2 + dx, y + by * 2 + dy);
                    rr = getr(c) / 255.0f;
                    gg = getg(c) / 255.0f;
                    bb = getb(c) / 255.0f;
                    rgbToYuv(rr, gg, bb, &yy, &uu, &vv);
                    su += uu; sv += vv;
                }
            out[offY + by * half + bx] = su / 4;
            out[offY + half * half + by * half + bx] = sv / 4;
        }
}

static void vecToBlock(BITMAP* dst, int x, int y, int bs, const float* vec, ColorMode mode) {
    if (mode == CM_GRAY) {
        for (int by = 0; by < bs; by++)
            for (int bx = 0; bx < bs; bx++) {
                int v = (int)(vec[by * bs + bx] * 255);
                putpixel(dst, x + bx, y + by, makecol(v, v, v));
            }
        return;
    }

    int half = bs / 2, offY = bs * bs;
    for (int by = 0; by < bs; by++)
        for (int bx = 0; bx < bs; bx++) {
            float yv = vec[by * bs + bx];
            float uv = vec[offY + (by / 2) * half + (bx / 2)];
            float vv = vec[offY + half * half + (by / 2) * half + (bx / 2)];
            float r, g, b;
            yuvToRgb(yv, uv, vv, &r, &g, &b);
            int ri = (int)(r * 255), gi = (int)(g * 255), bi = (int)(b * 255);
            putpixel(dst, x + bx, y + by, makecol(
                ri < 0 ? 0 : ri > 255 ? 255 : ri,
                gi < 0 ? 0 : gi > 255 ? 255 : gi,
                bi < 0 ? 0 : bi > 255 ? 255 : bi));
        }
}

// --- Weighted distance / adapt --------------------------------------------

static float distance(const float* a, const float* b, int dim, const float* w) {
    float sum = 0.0f;
    if (w) {
        for (int i = 0; i < dim; i++) {
            float d = a[i] - b[i];
            sum += w[i] * d * d;
        }
    } else {
        for (int i = 0; i < dim; i++) {
            float d = a[i] - b[i];
            sum += d * d;
        }
    }
    return sqrtf(sum);
}

static void adapt(float* weights, const float* input, int dim, float lr, const float* w) {
    if (w) {
        for (int i = 0; i < dim; i++) {
            float diff = input[i] - weights[i];
            float step = lr * w[i] * diff;
            float factor = 1.0f / (1.0f + 0.5f * fabsf(diff));
            weights[i] += step * factor;
        }
    } else {
        for (int i = 0; i < dim; i++) {
            float diff = input[i] - weights[i];
            float factor = 1.0f / (1.0f + 0.5f * fabsf(diff));
            weights[i] += lr * diff * factor;
        }
    }
}

// --- Weight table builder -------------------------------------------------

static void buildWeights(float* w, int bs, ColorMode mode) {
    if (mode == CM_GRAY) return;
    int dim = blockDim(bs, mode);
    for (int i = 0; i < dim; i++) w[i] = 1.0f;
    for (int i = 0; i < bs * bs; i++) w[i] = LUMA_WEIGHT;
}

// --- Public API -----------------------------------------------------------

Codebook* createAndTrainCodebook(BITMAP* img, int numCodes, int bs, int passes, ColorMode mode) {
    int w = img->w, h = img->h, dim = blockDim(bs, mode);
    Codebook* cb = malloc(sizeof(Codebook));
    cb->count = numCodes;
    cb->dim = dim;
    cb->blockSize = bs;
    cb->vectors = malloc(numCodes * sizeof(CodeVector));

    float* weights = NULL;
    float weightBuf[MAX_DIM];
    if (mode == CM_YUV) { buildWeights(weightBuf, bs, mode); weights = weightBuf; }

    int blocksH = (h) / bs, blocksW = (w) / bs;

    srand(time(NULL));

    if (initMode == INIT_KMEANS) {
        for (int i = 0; i < numCodes; i++)
            cb->vectors[i].weights = malloc(dim * sizeof(float));

        int totalB = blocksW * blocksH;
        float* minD = malloc(totalB * sizeof(float));
        float* buf = malloc(totalB * dim * sizeof(float));
        #pragma omp parallel for
        for (int bi = 0; bi < totalB; bi++) {
            int bx = bi % blocksW, by = bi / blocksW;
            blockToVec(img, bx * bs, by * bs, bs, buf + bi * dim, mode);
            minD[bi] = 1e30f;
        }

        int pick = rand() % totalB;
        memcpy(cb->vectors[0].weights, buf + pick * dim, dim * sizeof(float));

        for (int i = 1; i < numCodes; i++) {
            double total = 0.0;
            #pragma omp parallel for reduction(+:total)
            for (int bi = 0; bi < totalB; bi++) {
                float d = distance(buf + bi * dim, cb->vectors[i - 1].weights, dim, NULL);
                if (d < minD[bi]) minD[bi] = d;
                total += minD[bi] * minD[bi];
            }
            double r = (double)rand() / RAND_MAX * total;
            double cum = 0.0;
            for (int bi = 0; bi < totalB; bi++) {
                cum += minD[bi] * minD[bi];
                if (cum >= r) { memcpy(cb->vectors[i].weights, buf + bi * dim, dim * sizeof(float)); break; }
            }
        }
        free(minD);
        free(buf);
    } else {
        for (int i = 0; i < numCodes; i++) {
            cb->vectors[i].weights = malloc(dim * sizeof(float));
            int x = rand() % (w - bs);
            int y = rand() % (h - bs);
            blockToVec(img, x, y, bs, cb->vectors[i].weights, mode);
        }
    }

    float lr = 0.15f;

    for (int p = 0; p < passes; p++) {
        #pragma omp parallel for
        for (int by = 0; by < blocksH; by++) {
            float local[MAX_DIM];
            for (int bx = 0; bx < blocksW; bx++) {
                int y = by * bs, x = bx * bs;
                blockToVec(img, x, y, bs, local, mode);

                int winner = 0;
                float best = distance(local, cb->vectors[0].weights, dim, weights);
                for (int i = 1; i < numCodes; i++) {
                    float d = distance(local, cb->vectors[i].weights, dim, weights);
                    if (d < best) { best = d; winner = i; }
                }

                #pragma omp critical
                adapt(cb->vectors[winner].weights, local, dim, lr, weights);
            }
        }
        lr *= 0.92f;

        if (showProgress) {
            int barW = 30;
            int pct = (p + 1) * 100 / passes;
            int fill = pct * barW / 100;
            printf("\rTraining  [");
            for (int i = 0; i < fill; i++) putchar('#');
            for (int i = fill; i < barW; i++) putchar('.');
            printf("] %3d%%", pct);
        }
    }
    if (showProgress) printf("\n");
    return cb;
}

int* compressImage(BITMAP* src, BITMAP* dst, Codebook* cb, ColorMode mode) {
    int bs = cb->blockSize, dim = cb->dim;
    int blocksW = (src->w + bs - 1) / bs;
    int blocksH = (src->h + bs - 1) / bs;
    int total = blocksW * blocksH;

    float* weights = NULL;
    float weightBuf[MAX_DIM];
    if (mode == CM_YUV) { buildWeights(weightBuf, bs, mode); weights = weightBuf; }

    int* indices = malloc(total * sizeof(int));

    #pragma omp parallel for
    for (int by = 0; by < blocksH; by++) {
        float local[MAX_DIM];
        for (int bx = 0; bx < blocksW; bx++) {
            int y = by * bs, x = bx * bs;
            blockToVec(src, x, y, bs, local, mode);

            int best = 0;
            float bestD = distance(local, cb->vectors[0].weights, dim, weights);
            for (int i = 1; i < cb->count; i++) {
                float d = distance(local, cb->vectors[i].weights, dim, weights);
                if (d < bestD) { bestD = d; best = i; }
            }

            indices[by * blocksW + bx] = best;
            vecToBlock(dst, x, y, bs, cb->vectors[best].weights, mode);
        }
    }
    return indices;
}

void decompressImage(BITMAP* dst, Codebook* cb, const int* indices, ColorMode mode) {
    int bs = cb->blockSize, blockIdx = 0;
    for (int y = 0; y <= dst->h - bs; y += bs)
        for (int x = 0; x <= dst->w - bs; x += bs)
            vecToBlock(dst, x, y, bs, cb->vectors[indices[blockIdx++]].weights, mode);
}

double computePsnr(BITMAP* orig, BITMAP* comp) {
    long long sumR = 0, sumG = 0, sumB = 0;
    int total = orig->w * orig->h;
    #pragma omp parallel for reduction(+:sumR,sumG,sumB)
    for (int i = 0; i < total; i++) {
        int x = i % orig->w;
        int y = i / orig->w;
        int co = getpixel(orig, x, y);
        int cc = getpixel(comp, x, y);
        int dr = getr(co) - getr(cc);
        int dg = getg(co) - getg(cc);
        int db = getb(co) - getb(cc);
        sumR += (long long)dr * dr;
        sumG += (long long)dg * dg;
        sumB += (long long)db * db;
    }
    double mse = ((double)sumR + sumG + sumB) / (total * 3);
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
    bw->file = f; bw->buffer = 0; bw->bitPos = 0;
}

static void bitWriterWrite(BitWriter* bw, unsigned int value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        bw->buffer = (bw->buffer << 1) | ((value >> i) & 1);
        if (++bw->bitPos == 8) { fputc(bw->buffer, bw->file); bw->buffer = 0; bw->bitPos = 0; }
    }
}

static void bitWriterFlush(BitWriter* bw) {
    if (bw->bitPos > 0) { bw->buffer <<= (8 - bw->bitPos); fputc(bw->buffer, bw->file); }
}

int saveCompressed(const char* filename, Codebook* cb, const int* indices, int totalBlocks) {
    FILE* f = fopen(filename, "wb");
    if (!f) return 0;

    BitWriter bw;
    bitWriterInit(&bw, f);

    bitWriterWrite(&bw, cb->count, 16);
    bitWriterWrite(&bw, cb->blockSize, 8);
    bitWriterWrite(&bw, cb->dim, 8);

    for (int i = 0; i < cb->count; i++)
        for (int j = 0; j < cb->dim; j++) {
            unsigned char val = (unsigned char)(cb->vectors[i].weights[j] * 255.0f);
            bitWriterWrite(&bw, val, 8);
        }

    int bitsPerIndex = 0;
    while ((1u << bitsPerIndex) < (unsigned)cb->count) bitsPerIndex++;
    for (int i = 0; i < totalBlocks; i++)
        bitWriterWrite(&bw, indices[i], bitsPerIndex);

    bitWriterFlush(&bw);
    fclose(f);
    return 1;
}
