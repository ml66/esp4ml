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
} dma_info_t;

#endif
