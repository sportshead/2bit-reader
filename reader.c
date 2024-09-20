#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t signature;
  uint32_t version;
  uint32_t sequenceCount;
  uint32_t reserved;
} Header;

typedef struct {
  uint32_t dnaSize;

  uint32_t nBlockCount;
  uint32_t *nBlockStarts;
  uint32_t *nBlockSizes;

  uint32_t maskBlockCount;
  uint32_t *maskBlockStarts;
  uint32_t *maskBlockSizes;

  uint32_t reserved;

  uint32_t dnaOffset;
} SequenceRecord;

typedef struct {
  uint8_t nameSize;
  char *name;
  uint32_t offset;

  SequenceRecord *seq;
} IndexEntry;

#define BSWAP(x) __builtin_bswap32(x)
#define MAGIC 0x1A412743

#define READSWAP(x)                                                            \
  fread(&x, sizeof(x), 1, f);                                                  \
  if (bswap)                                                                   \
  x = BSWAP(x)

char DNA_BITS[4] = {'T', 'C', 'A', 'G'};

// https://stackoverflow.com/a/1068937
int numPlaces(uint32_t n) {
  if (n < 10)
    return 1;
  if (n < 100)
    return 2;
  if (n < 1000)
    return 3;
  if (n < 10000)
    return 4;
  if (n < 100000)
    return 5;
  if (n < 1000000)
    return 6;
  if (n < 10000000)
    return 7;
  if (n < 100000000)
    return 8;
  if (n < 1000000000)
    return 9;
  return 10;
}

int getInt(int max) {
  for (;;) {
    int i;
    int c = scanf("%d", &i);
    if (c == EOF || i == -1) {
      return -1;
    }
    if (c != 1) {
      while ((c = getchar()) != '\n' && c != EOF)
        ;
      printf("Invalid input\n");
      continue;
    }
    if (i < 0 || i >= max) {
      printf("Invalid index\n");
      continue;
    }

    return i;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");

  if (f == NULL) {
    perror(argv[1]);
    return 1;
  }

  bool bswap = false;

  Header header;
  fread(&header, sizeof(header), 1, f);

  if (header.signature != MAGIC) {
    if (BSWAP(header.signature) == MAGIC) {
      bswap = true;
      header.signature = BSWAP(header.signature);
      header.version = BSWAP(header.version);
      header.sequenceCount = BSWAP(header.sequenceCount);
      header.reserved = BSWAP(header.reserved);
    } else {
      printf("0x%08x != 0x1A412743: Not a 2bit file\n", header.signature);
      return 1;
    }
  }

  if (header.version != 0) {
    printf("Invalid file version %d\n", header.version);
    return 1;
  }

  printf("{ signature = 0x%08x, version = %d, sequenceCount = %d, reserved = "
         "0x%08x }\n",
         header.signature, header.version, header.sequenceCount,
         header.reserved);

  IndexEntry *index =
      (IndexEntry *)malloc(header.sequenceCount * sizeof(IndexEntry));

  for (int i = 0; i < header.sequenceCount; i++) {
    READSWAP(index[i].nameSize);

    index[i].name = (char *)malloc(index[i].nameSize + 1);
    fread(index[i].name, index[i].nameSize, 1, f);
    index[i].name[index[i].nameSize] = 0;

    READSWAP(index[i].offset);

    printf("%0*d. %s @ 0x%08x\n", numPlaces(header.sequenceCount), i,
           index[i].name, index[i].offset);
  }

  printf("\n\n");

  while (!feof(stdin)) {
    printf("Sequence index (0-%d, -1 to exit)> ", header.sequenceCount - 1);

    int i = getInt(header.sequenceCount - 1);
    if (i == -1) {
      break;
    }

    if (!index[i].seq) {
      SequenceRecord *seq = (SequenceRecord *)malloc(sizeof(SequenceRecord));

      fseek(f, index[i].offset, SEEK_SET);
      READSWAP(seq->dnaSize);

      READSWAP(seq->nBlockCount);
      seq->nBlockStarts =
          (uint32_t *)malloc(seq->nBlockCount * sizeof(uint32_t));
      seq->nBlockSizes =
          (uint32_t *)malloc(seq->nBlockCount * sizeof(uint32_t));
      for (int j = 0; j < seq->nBlockCount; j++) {
        READSWAP(seq->nBlockStarts[j]);
      }
      for (int j = 0; j < seq->nBlockCount; j++) {
        READSWAP(seq->nBlockSizes[j]);
      }

      READSWAP(seq->maskBlockCount);
      seq->maskBlockStarts =
          (uint32_t *)malloc(seq->maskBlockCount * sizeof(uint32_t));
      seq->maskBlockSizes =
          (uint32_t *)malloc(seq->maskBlockCount * sizeof(uint32_t));
      for (int j = 0; j < seq->maskBlockCount; j++) {
        READSWAP(seq->maskBlockStarts[j]);
      }
      for (int j = 0; j < seq->maskBlockCount; j++) {
        READSWAP(seq->maskBlockSizes[j]);
      }

      READSWAP(seq->reserved);

      seq->dnaOffset = ftell(f);

      index[i].seq = seq;
    }

    SequenceRecord *seq = index[i].seq;

    printf("Sequence %s (%0*d) is %d bases long. nBlockCount = %d, "
           "maskBlockCount = %d\n",
           index[i].name, numPlaces(header.sequenceCount), i, seq->dnaSize,
           seq->nBlockCount, seq->maskBlockCount);

    printf("Starting base offset (0-%d, -1 to exit)> ", seq->dnaSize - 1);
    int baseOffset = getInt(seq->dnaSize - 1);
    if (baseOffset == -1) {
      continue;
    }
    printf("Length to read (0-%d, -1 to exit)> ", seq->dnaSize - baseOffset);
    int length = getInt(seq->dnaSize - baseOffset);
    if (length == -1) {
      continue;
    }

    char midBase = baseOffset % 4;

    uint32_t pos = seq->dnaOffset + (baseOffset / 4);
    char packed;

    fseek(f, pos, SEEK_SET);
    fread(&packed, sizeof(packed), 1, f);
    printf("%x", packed);

    for (int j = 0; j < length; j++) {
      if (j % 12 == 0) {
        putchar('\n');
      }
      char baseIndex = ((j - midBase) % 4);

      if (baseIndex == 0 && j != 0) {
        fread(&packed, sizeof(packed), 1, f);
      }
      // printf("%d %x %d", j, packed, baseIndex);
      baseIndex = 3 - baseIndex;
      baseIndex *= 2;
      baseIndex = packed >> baseIndex;
      baseIndex &= 0b11;

      // printf("%d %c\n", baseIndex, DNA_BITS[baseIndex]);
      putchar(DNA_BITS[baseIndex]);
    }

    putchar('\n');
  }

  for (int i = 0; i < header.sequenceCount; i++) {
    free(index[i].name);

    if (index[i].seq) {
      free(index[i].seq->nBlockStarts);
      free(index[i].seq->nBlockSizes);
      free(index[i].seq->maskBlockStarts);
      free(index[i].seq->maskBlockSizes);
      free(index[i].seq);
    }
  }
  free(index);

  fclose(f);
  return 0;
}
