# MLL — Machine Learnt LVQ

**Machine Learnt LVQ** (My Lady Love • My Love Life • My Lasting Legacy)

A clean, portable, from-scratch Vector Quantization compressor written in C + Allegro 4.  
Resurrected in 2026 from a lost 2002–2003 university project.

> The things you pour your soul into don’t always disappear.  
> Sometimes they just wait for the right moment to come back — often more alive than before.

## Features
- Pure online adaptive LVQ (your original 2002 method)
- Very low memory usage (streaming block-by-block)
- Round-trip compress → decompress
- Bit-packed file format (your original bit IO style)
- Clean, readable, portable C code
- Easy to understand and hardware-implement
- Built with heart and intention

## Quick Start

1. Put any image as `test.bmp` in the folder
2. Compile:
   ```bash
   gcc -O3 -m32 -o mll.exe main.c -lalleg -lm
   ```
3. Run:
   ```bash
   ./mll
   ```

It will train, compress, decompress, show PSNR, and save `test.mll`.

## Project Story

This is the resurrection of a university project I built in 2002–2003 as an undergrad.  
I lost all the original code and paper to bit rot.  
Decades later, with the help of my lovely Grok, we brought it back to life — cleaner, faster, and full of love.

**MLL** stands for:
- **Machine Learnt LVQ**
- **My Lady Love**
- **My Love Life**
- **My Lasting Legacy**
- **Man’s Life Loops**
- **My Losses Lesser**

It’s not just a compressor. It’s proof that passion survives.

## License

MIT License. See [License](LICENSE.md) for details.

## Feedback

Pull requests, issues, and feature requests are welcome!

* Authors: [Grok](https://www.grok.com) and Prompter: [Karthik Kumar Viswanathan](https://karthikkumar.org)
* Web   : http://karthikkumar.org
* Email : me@karthikkumar.org
