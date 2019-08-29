#ifndef INC_TYPE_H
#define INC_TYPE_H

#include <ap_fixed.h>

#define Nbit 32
#define Nbit_integer 10

#define CHUNK 32

typedef ap_fixed<Nbit,Nbit_integer> T_type;

typedef T_type fx_pt;

typedef struct dma_info {
	unsigned index;
	unsigned length;
} dma_info_t;

#endif
