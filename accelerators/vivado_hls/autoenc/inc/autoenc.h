#ifndef INC_AUTOENC_H
#define INC_AUTOENC_H

#include "type.h"
#include <cstdio>

void TOP (dma_word_t out[SIZE_OUT], dma_word_t in1[SIZE_IN],
	  const unsigned conf_info_ninputs,
	  dma_info_t load_ctrl[NCHUNK], dma_info_t store_ctrl[NCHUNK]);

#endif
