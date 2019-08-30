#ifndef INC_ADDER_H
#define INC_ADDER_H

#include "type.h"
#include <cstdio>

void TOP(
    word *out, word* in1, const unsigned conf_info_size,
    dma_info_t *load_ctrl, dma_info_t *store_ctrl);

#endif
