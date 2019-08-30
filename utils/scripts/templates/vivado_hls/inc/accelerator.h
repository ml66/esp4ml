#ifndef INC_<ACCELERATOR_NAME>_H
#define INC_<ACCELERATOR_NAME>_H

#include "type.h"
#include <cstdio>

#ifdef DMA32
void <accelerator_name>_dma32(
#else
void <accelerator_name>_dma64(
#endif
    fx_pt *out, fx_pt* in1,
    const unsigned conf_info_size, // add all the cfg params
                                    // specified in the xml file 
    dma_info_t *load_ctrl, dma_info_t *store_ctrl);
#endif
