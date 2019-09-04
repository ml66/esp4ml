#ifndef INC_SVHNMLP_H
#define INC_SVHNMLP_H

#include "type.h"
#include <cstdio>

void TOP (word *out, word* in1, const unsigned n_chunks,
	  dma_info_t *load_ctrl, dma_info_t *store_ctrl);
#endif
