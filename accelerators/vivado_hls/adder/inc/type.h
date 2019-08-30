#ifndef INC_TYPE_H
#define INC_TYPE_H

#include <ap_fixed.h>

#define CHUNK_SIZE 32

// work with fixed point
#ifdef DMA32
#define Nbit 32
#define Nbit_integer 16
#else
#define Nbit 64
#define Nbit_integer 32
#endif
typedef ap_fixed<Nbit,Nbit_integer> word;

// Ctrl
typedef struct dma_info {
    unsigned index;
    unsigned length;
    unsigned size;
} dma_info_t;

// The 'size' variable of 'dma_info' indicates the bit-width of the words
// processed by the accelerator. Here are the encodings:
#define SIZE_BYTE   0
#define SIZE_HWORD  1
#define SIZE_WORD   2
#define SIZE_DWORD  3
#define SIZE_4WORD  4
#define SIZE_8WORD  5
#define SIZE_16WORD 6
#define SIZE_32WORD 7

#endif
