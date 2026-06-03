#include <stdio.h>
#include <string.h>
#include <time.h>
#include <allegro.h>
#include "lvq.h"

static BITMAP* loadBitmap(const char* path) {
    BITMAP* b = load_bitmap(path, NULL);
    if (!b) printf("Failed to load '%s'\n", path);
    return b;
}

static BITMAP* cloneBitmap(BITMAP* src) {
    return src ? create_bitmap(src->w, src->h) : NULL;
}

static void saveBitmap(BITMAP* b, const char* path) {
    if (!save_bitmap(path, b, NULL))
        printf("Saved: %s\n", path);
    else
        printf("Failed to save: %s\n", path);
}

static void destroyBitmap(BITMAP* b) {
    if (b) destroy_bitmap(b);
}

static void getStem(const char* path, char* out, int outSize) {
    const char* dot = strrchr(path, '.');
    int len = dot ? (int)(dot - path) : (int)strlen(path);
    if (len >= outSize) len = outSize - 1;
    strncpy(out, path, len);
    out[len] = 0;
}

int main(int argc, char* argv[]) {
    const char* inFile = "test.bmp";
    ColorMode mode = CM_GRAY;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-gray")     == 0) mode = CM_GRAY;
        else if (strcmp(argv[i], "-yuv")      == 0) mode = CM_YUV;
        else if (strcmp(argv[i], "-progress") == 0) showProgress = 1;
        else if (argv[i][0] != '-')                 inFile = argv[i];
    }

    allegro_init();
    set_color_depth(32);

    BITMAP* orig = loadBitmap(inFile);
    if (!orig) return 1;
    int w = orig->w, h = orig->h;

    char stem[256];
    getStem(inFile, stem, sizeof(stem));
    char mllPath[320], decPath[320];
    snprintf(mllPath, sizeof(mllPath), "%s.mll", stem);
    snprintf(decPath, sizeof(decPath), "%s_d.bmp", stem);

    BITMAP* comp = cloneBitmap(orig);
    BITMAP* decomp = cloneBitmap(orig);

    clock_t start = clock();

    Codebook* cb = createAndTrainCodebook(orig, 256, 4, 35, mode);

    int blocksW = (w + 3) / 4;
    int blocksH = (h + 3) / 4;

    int* indices = compressImage(orig, comp, cb, mode);
    decompressImage(decomp, cb, indices, mode);

    clock_t end = clock();
    double ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    fprintf(stderr, "Time: %.0f ms\n", ms);

    saveBitmap(decomp, decPath);

    if (!saveCompressed(mllPath, cb, indices, blocksW * blocksH))
        printf("Failed to save: %s\n", mllPath);
    else
        printf("Saved: %s\n", mllPath);

    free(indices);
    freeCodebook(cb);
    destroyBitmap(orig);
    destroyBitmap(comp);
    destroyBitmap(decomp);
    return 0;
}
END_OF_MAIN()
