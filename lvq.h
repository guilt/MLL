#ifndef LVQ_H
#define LVQ_H

#include <allegro.h>

typedef struct {
    float* weights;
    int    dim;
} CodeVector;

typedef struct {
    CodeVector* vectors;
    int         count;
    int         dim;
    int         blockSize;
} Codebook;

Codebook* createAndTrainCodebook(BITMAP* img, int numCodes, int blockSize, int passes);
void      freeCodebook(Codebook* cb);

int*      compressImage(BITMAP* src, BITMAP* dst, Codebook* cb);
void      decompressImage(BITMAP* dst, Codebook* cb, const int* indices);

int       saveCompressed(const char* filename, Codebook* cb, const int* indices, int totalBlocks);

double    computePsnr(BITMAP* original, BITMAP* compressed);

#endif