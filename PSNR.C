#include <stdio.h>
#include <math.h>
#include <allegro.h>

static BITMAP* loadBitmap(const char* path) {
    BITMAP* b = load_bitmap(path, NULL);
    if (!b) printf("Failed: %s\n", path);
    return b;
}

static void saveBitmap(BITMAP* b, const char* path) {
    if (!save_bitmap(path, b, NULL))
        printf("diff:       %s\n", path);
    else
        printf("Failed:     %s\n", path);
}

static void destroyBitmap(BITMAP* b) {
    if (b) destroy_bitmap(b);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: psnr ref.bmp cmp.bmp [diff.bmp]\n");
        return 1;
    }

    allegro_init();
    set_color_depth(32);

    BITMAP* a = loadBitmap(argv[1]);
    if (!a) return 1;
    BITMAP* b = loadBitmap(argv[2]);
    if (!b) { destroyBitmap(a); return 1; }

    int w = a->w < b->w ? a->w : b->w;
    int h = a->h < b->h ? a->h : b->h;

    long long sumR = 0, sumG = 0, sumB = 0, maxErr = 0;
    BITMAP* diff = (argc > 3) ? create_bitmap(w, h) : NULL;

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int ca = getpixel(a, x, y);
            int cb = getpixel(b, x, y);
            int dr = getr(ca) - getr(cb);
            int dg = getg(ca) - getg(cb);
            int db = getb(ca) - getb(cb);
            sumR += (long long)dr * dr;
            sumG += (long long)dg * dg;
            sumB += (long long)db * db;

            int e = abs(dr);
            if (abs(dg) > e) e = abs(dg);
            if (abs(db) > e) e = abs(db);
            if (e > maxErr) maxErr = e;

            if (diff) {
                int intensity = MIN(e * 8, 255);
                putpixel(diff, x, y, makecol(intensity, 255 - intensity, 0));
            }
        }

    double mse = ((double)sumR + sumG + sumB) / ((long long)w * h * 3);
    double psnr = (mse < 1e-10) ? 100.0 : 20.0 * log10(255.0 / sqrt(mse));

    printf("Dimensions: %dx%d\n", w, h);
    printf("PSNR:       %.2f dB\n", psnr);
    printf("Max error:  %I64d\n", maxErr);
    printf("MSE (R):    %.4f\n", (double)sumR / (w * h));
    printf("MSE (G):    %.4f\n", (double)sumG / (w * h));
    printf("MSE (B):    %.4f\n", (double)sumB / (w * h));

    if (diff) {
        saveBitmap(diff, argv[3]);
        destroyBitmap(diff);
    }

    destroyBitmap(a);
    destroyBitmap(b);
    return 0;
}
END_OF_MAIN()
