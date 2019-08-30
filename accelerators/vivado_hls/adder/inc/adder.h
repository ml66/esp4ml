#ifndef INC_ADDER_H
#define INC_ADDER_H

#include "type.h"
#include <cstdio>

#ifdef DMA32
void adder_dma32(
#else
void adder_dma64(
#endif
    word *out, word* in1, const unsigned conf_info_size,
    dma_info_t *load_ctrl, dma_info_t *store_ctrl);

#endif
