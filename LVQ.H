#ifndef LVQ_H
#define LVQ_H

#include <stdbool.h>
#include <allegro.h>

extern bool showProgress;

typedef enum { CM_GRAY, CM_YUV } ColorMode;
typedef enum { INIT_RANDOM, INIT_KMEANS } InitMode;
extern InitMode initMode;

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

Codebook* createAndTrainCodebook(BITMAP* img, int numCodes, int blockSize, int passes, ColorMode mode);
void      freeCodebook(Codebook* cb);

int*      compressImage(BITMAP* src, BITMAP* dst, Codebook* cb, ColorMode mode);
void      decompressImage(BITMAP* dst, Codebook* cb, const int* indices, ColorMode mode);

int       saveCompressed(const char* filename, Codebook* cb, const int* indices, int totalBlocks);

double    computePsnr(BITMAP* original, BITMAP* compressed);

#endif
