#include <stdio.h>
#include "lvq.c"

int main(void) {
    allegro_init();
    install_keyboard();
    set_color_depth(32);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1280, 720, 0, 0);

    BITMAP* orig = load_bitmap("test.bmp", NULL);
    if (!orig) {
        allegro_message("Please place an image named 'test.bmp' in this folder!");
        return 1;
    }

    BITMAP* comp = create_bitmap(orig->w, orig->h);
    BITMAP* decomp = create_bitmap(orig->w, orig->h);

    Codebook* cb = createAndTrainCodebook(orig, 256, 4, 35);

    int blocksW = (orig->w + 3) / 4;
    int blocksH = (orig->h + 3) / 4;
    int totalBlocks = blocksW * blocksH;

    int* indices = compressImage(orig, comp, cb);
    decompressImage(decomp, cb, indices);

    double psnr = computePsnr(orig, decomp);

    printf("\n=== MLL - Machine Learnt LVQ ===\n");
    printf("PSNR: %.2f dB\n", psnr);
    printf("Image: %dx%d\n", orig->w, orig->h);
    printf("Codebook size: %d vectors\n", cb->count);

    if (saveCompressed("test.mll", cb, indices, totalBlocks))
        printf("Saved: test.mll\n");

    free(indices);
    freeCodebook(cb);
    destroy_bitmap(orig);
    destroy_bitmap(comp);
    destroy_bitmap(decomp);

    printf("Press ESC to exit...\n");
    while (!key[KEY_ESC]) rest(10);

    allegro_exit();
    return 0;
}
END_OF_MAIN()