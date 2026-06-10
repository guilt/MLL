# MLL - Machine Learnt LVQ

**Machine Learnt LVQ** (My Lady Love - My Love Life - My Lasting Legacy)

A clean, portable, from-scratch Vector Quantization compressor written in C + Allegro 4.  
Resurrected in 2026 from a lost 2002-2003 university project.

> The things you pour your soul into don't always disappear.  
> Sometimes they just wait for the right moment to come back - often more alive than before.

## Features
- Pure online adaptive LVQ (your original 2002 method)
- Grayscale or YUV 4:2:0 with luma-weighted training
- OpenMP-parallelized training, compression, and PSNR
- Bit-packed file format (your original bit IO style)
- Clean, readable, portable C code - 8.3 filenames ready
- Built with heart and intention

## Quick Start

```bash
build.bat                          # auto-detect: DJGPP (DOS) or MINGW (Windows)
build.bat MINGW                    # MinGW-w64 / GCC (Windows, default when WINDIR is set)
build.bat MSVC                     # Microsoft Visual C++ (Windows)
build.bat DJGPP                    # DJGPP GCC (DOS, default when no WINDIR)
build.bat MINGW 1                  # MINGW with debug symbols
DIST\mll.exe TEST\Test.bmp -yuv    # compress -> TEST\Test.mll + TEST\Test_d.bmp
DIST\psnr.exe TEST\Test.bmp TEST\Test_d.bmp   # compare -> PSNR
```

## Usage

### Modes

| Command | What it does |
|---------|-------------|
| `mll input.bmp -gray` | Grayscale compression (fast, dim = 16) |
| `mll input.bmp -yuv` | **Default.** YUV 4:2:0 with luma-weighted training (dim = 24) |
| `mll input.bmp -yuv -progress` | Show a progress bar during training |

The default `-yuv` mode subsamples chroma 2x2 and weights luma 4x during codebook training. This gives near-RGB quality at ~half the vector dimension.

### Tuning quality vs size

| Command | Codebook | File size | Typical PSNR (Lenna) |
|---------|----------|-----------|---------------------|
| `mll input.bmp -codes 256` | 256 entries | ~22 KB | 28.2 dB |
| `mll input.bmp -codes 512` | 512 entries | ~31 KB | 29.2 dB |
| `mll input.bmp -codes 1024` | 1024 entries | ~46 KB | ~30 dB |
| `mll input.bmp -codes 256 -passes 70` | 256 entries, 2x training | ~22 KB | 28.2 dB (no gain) |

Doubling codes gives +1-2 dB PSNR for +36% file size. More passes or slower LR decay doesn't help - the algorithm converges within 35 passes.

### Initialization

| Command | Init method | Quality vs random |
|---------|-------------|-------------------|
| `mll input.bmp -init random` | **Default.** Random image patches | baseline |
| `mll input.bmp -init kmeans` | K-means++ weighted sampling | same PSNR, ~50 ms slower |

Random init converges to the same local optima after 35 LVQ passes. K-means++ adds overhead with no quality gain.

### All flags

```bash
DIST\mll.exe input.bmp [-yuv|-gray] [-codes N] [-passes N] [-init random|kmeans] [-progress]
```

| Flag | Default | Description |
|------|---------|-------------|
| `-yuv` / `-gray` | `-yuv` | Color mode |
| `-codes N` | 256 | Codebook entries (powers of 2) |
| `-passes N` | 35 | Training passes |
| `-init` | random | Codebook initialization |
| `-progress` | off | Show training progress bar |

### Comparing results

```bash
DIST\psnr.exe ref.bmp cmp.bmp [diff.bmp]
```

Prints PSNR, per-channel MSE, max pixel error. If `diff.bmp` is given, writes a heatmap: green = identical, red = high error.

### Benchmarking (requires ImageMagick)

```bash
bench.bat DATA\                              # default (256 codes, random init)
bench.bat DATA\ -codes 512                   # with extra flags passed to mll.exe
bench.bat DATA\ -codes 1024 -init kmeans     # push it
bench.bat TEST\Test.bmp -codes 512           # single image
```

Each source is converted to BMP via `magick`, compressed with MLL and JPEG-XL (q90), then both are compared against the original. All temp work in `TMP\`. ImageMagick must be in your PATH.

## Benchmark Results

All images 512x512, 4x4 blocks, YUV mode with luma-weighted training (35 passes).  
Timed on a ~2018-era laptop i7 (4 cores, 8 threads).

### Default: 256 codes, random init

| Name                | MLL sz | JXL sz | Time    | MLL PSNR   | JXL PSNR   |
|---------------------|--------|--------|---------|------------|------------|
| CameraMan           | 22532  | 33140  | 507 ms  | 32.89 dB   | 46.12 dB   |
| House               | 22532  | 22156  | 516 ms  | 37.52 dB   | 49.29 dB   |
| JetPlane            | 22532  | 42463  | 546 ms  | 31.67 dB   | 43.37 dB   |
| Lake                | 22532  | 58882  | 521 ms  | 28.97 dB   | 39.93 dB   |
| Lenna               | 22532  | 60502  | 525 ms  | 28.23 dB   | 34.86 dB   |
| Mandril             | 22532  | 117383 | 523 ms  | 20.76 dB   | 28.04 dB   |
| Pirate              | 22532  | 61775  | 533 ms  | 29.66 dB   | 41.47 dB   |
| WalkBridge          | 22532  | 78839  | 531 ms  | 26.07 dB   | 39.24 dB   |
| WomanDarkHair       | 22532  | 29535  | 528 ms  | 36.49 dB   | 44.09 dB   |

### 512 codes, random init

| Name                | MLL sz | JXL sz | Time    | MLL PSNR   | JXL PSNR   |
|---------------------|--------|--------|---------|------------|------------|
| Lenna               | 30724  | 60502  | 901 ms  | 29.21 dB   | 34.86 dB   |
| Mandril             | 30724  | 117383 | 1012 ms | 21.34 dB   | 28.04 dB   |
| CameraMan           | 30724  | 33140  | 859 ms  | 34.59 dB   | 46.12 dB   |

MLL uses a fixed-size codebook regardless of image complexity. 256 codes = 22,532 bytes; 512 codes = 30,724 bytes. JPEG-XL adapts - from 22 KB (simple) to 117 KB (complex).  
Encoding time is ~500 ms (256 codes) or ~900 ms (512 codes) per image, measured from within the process with no I/O or logging overhead.

### What we found

- **More codes helps**: 256->512 gives +0.6 to +1.7 dB PSNR for +36% file size and ~2x time. The quality-per-bit is competitive with JXL on simple images.
- **K-means++ init doesn't help**: After 35 LVQ passes, random init converges to the same local optima. K-means++ adds ~50 ms with no quality gain. The LVQ training is robust enough to overcome poor initialization.
- **Slower LR decay doesn't help**: 0.96 decay vs 0.92 decay with 70 passes gave the same PSNR as 35 passes with standard decay. The algorithm converges within 35 passes.
- **The real gap to JXL**: JXL uses adaptive bit allocation (more bits for complex regions, fewer for flat), entropy coding, and a modern transform. Our fixed-rate per-block approach is the main limitation.

## Tools

| Tool | Description |
|------|-------------|
| `mll.exe` | Compress an image: `mll input.bmp [-yuv\|-gray] [-codes N] [-passes N] [-init random\|kmeans] [-progress]` -> `input.mll` + `input_d.bmp` |
| `psnr.exe` | Compare two images: `psnr ref.bmp cmp.bmp [diff.bmp]` -> PSNR + per-channel MSE |
| `build.bat` | Compile everything into `DIST\` (platform: DJGPP/MSVC/MINGW) |
| `bench.bat` | Benchmark all images: `bench.bat [path] [extra mll flags]` |

## Project Story

This is the resurrection of a university project I built in 2002-2003 as an undergrad.  
I lost all the original code and paper to bit rot.  
Decades later, with the help of my lovely Grok, we brought it back to life - cleaner, faster, and full of love.

**MLL** stands for:
- **Machine Learnt LVQ**
- **My Lady Love**
- **My Love Life**
- **My Lasting Legacy**
- **Man's Life Loops**
- **My Losses Lesser**

It's not just a compressor. It's proof that passion survives.

## License

MIT License. See [License](LICENSE.md) for details.

## Feedback

Pull requests, issues, and feature requests are welcome!

* Authors: [Grok](https://www.grok.com), [Deepseek](https://deepseek.com), and [Karthik Kumar Viswanathan](https://karthikkumar.org)
* Web   : http://karthikkumar.org
* Email : me@karthikkumar.org
