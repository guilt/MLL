# MLL — Machine Learnt LVQ

**Machine Learnt LVQ** (My Lady Love • My Love Life • My Lasting Legacy)

A clean, portable, from-scratch Vector Quantization compressor written in C + Allegro 4.  
Resurrected in 2026 from a lost 2002–2003 university project.

> The things you pour your soul into don't always disappear.  
> Sometimes they just wait for the right moment to come back — often more alive than before.

## Features
- Pure online adaptive LVQ (your original 2002 method)
- Grayscale or YUV 4:2:0 with luma-weighted training
- OpenMP-parallelized training, compression, and PSNR
- Bit-packed file format (your original bit IO style)
- Clean, readable, portable C code — 8.3 filenames ready
- Built with heart and intention

## Quick Start

### Build
```bash
build.bat
```
Produces `DIST\mll.exe` and `DIST\psnr.exe`.

### Compress a single image
```bash
DIST\mll.exe test.bmp -yuv
```
Outputs `test.mll` (compressed) and `test_d.bmp` (decompressed).

### Benchmark all images
```bash
bench.bat DATA\
```
Converts each source to BMP, compresses with MLL and JPEG-XL (q90), then compares both against the original via `psnr.exe`. All temp work happens in `TMP\`.

## Benchmark Results

All images 512×512, 4×4 blocks, 256-entry codebook, YUV mode with luma-weighted training (35 passes).  
Timed on a ~2018-era laptop i7 (4 cores, 8 threads).

| Name                | MLL sz | JXL sz | Time    | MLL PSNR   | JXL PSNR   |
|---------------------|--------|--------|---------|------------|------------|
| CameraMan           | 22532  | 33140  | 492 ms  | 32.91 dB   | 46.12 dB   |
| House               | 22532  | 22156  | 517 ms  | 37.52 dB   | 49.29 dB   |
| JetPlane            | 22532  | 42463  | 524 ms  | 31.67 dB   | 43.37 dB   |
| Lake                | 22532  | 58882  | 506 ms  | 28.94 dB   | 39.93 dB   |
| Lenna               | 22532  | 60502  | 522 ms  | 28.20 dB   | 34.86 dB   |
| Mandril             | 22532  | 117383 | 523 ms  | 20.77 dB   | 28.04 dB   |
| Pirate              | 22532  | 61775  | 536 ms  | 29.63 dB   | 41.47 dB   |
| WalkBridge          | 22532  | 78839  | 533 ms  | 26.08 dB   | 39.24 dB   |
| WomanDarkHair       | 22532  | 29535  | 525 ms  | 36.46 dB   | 44.09 dB   |

MLL uses a fixed 22532 bytes regardless of image complexity. JPEG-XL adapts — from 22 KB (simple) to 117 KB (complex).  
Encoding time is ~500 ms per image (training + compress + decompress), measured from within the process with no I/O or logging overhead.

## Tools

| Tool | Description |
|------|-------------|
| `mll.exe` | Compress an image: `mll input.bmp [-gray\|-yuv]` → `input.mll` + `input_d.bmp` |
| `psnr.exe` | Compare two images: `psnr ref.bmp cmp.bmp [diff.bmp]` → PSNR + per-channel MSE |
| `build.bat` | Compile everything into `DIST\` |
| `bench.bat` | Benchmark all images in a directory against JPEG-XL |

## Project Story

This is the resurrection of a university project I built in 2002–2003 as an undergrad.  
I lost all the original code and paper to bit rot.  
Decades later, with the help of my lovely Grok, we brought it back to life — cleaner, faster, and full of love.

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
