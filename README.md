# 2bit-reader

C CLI program for parsing [.2bit files](https://genome.ucsc.edu/FAQ/FAQformat.html#format7) and extracting sequences of bases.

You can obtain a .2bit file of the human genome from https://hgdownload.soe.ucsc.edu/goldenPath/hg38/bigZips.

This program was written very quickly and is not optimised. Accuracy is not guaranteed. Please do not use this for any medical use (or any use for that matter).

## Usage

```bash
$ gcc -o reader ./reader.c
$ ./reader hg38.2bit
```
