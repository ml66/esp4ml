#ifndef INC_TYPE_H
#define INC_TYPE_H

#include <ap_fixed.h>

#define CHUNK_SIZE 32

// work with fixed point
#define Nbit 32
#define Nbit_integer 10
typedef ap_fixed<Nbit,Nbit_integer> word;

// work with integers
// typedef int word;

// Ctrl
typedef struct dma_info {
    unsigned index;
    unsigned length;
    unsigned size;
} dma_info_t;

// The 'size' variable of 'dma_info' indicates the bit-width of the words
// processed by the accelerator. Here are the encodings:
#define SIZE_BYTE   sc_dt::sc_bv<3>(0)
#define SIZE_HWORD  sc_dt::sc_bv<3>(1)
#define SIZE_WORD   sc_dt::sc_bv<3>(2)
#define SIZE_DWORD  sc_dt::sc_bv<3>(3)
#define SIZE_4WORD  sc_dt::sc_bv<3>(4)
#define SIZE_8WORD  sc_dt::sc_bv<3>(5)
#define SIZE_16WORD sc_dt::sc_bv<3>(6)
#define SIZE_32WORD sc_dt::sc_bv<3>(7)

#endif
