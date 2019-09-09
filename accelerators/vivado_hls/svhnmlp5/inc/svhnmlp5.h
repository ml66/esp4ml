#ifndef INC_SVHNMLP5_H
#define INC_SVHNMLP5_H

#include "type.h"
#include <cstdio>

void TOP (dma_word_t *out, dma_word_t *in1, const unsigned conf_info_ninputs,
	  dma_info_t *load_ctrl, dma_info_t *store_ctrl);
#endif
