// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VISIONCHIP_DIRECTIVES_HPP__
#define __VISIONCHIP_DIRECTIVES_HPP__

#define WORDS_PER_DMA (DMA_WIDTH >> 4)

#if defined(STRATUS_HLS)


#if (WORDS_PER_DMA == 2)

#define HLS_MAP_mem_buff_1                              \
    HLS_MAP_TO_MEMORY(mem_buff_1, "plm_data16_2w2r")
#define HLS_MAP_mem_buff_2                              \
    HLS_MAP_TO_MEMORY(mem_buff_2, "plm_data16_2w2r")

#elif (WORDS_PER_DMA == 4)

#define HLS_MAP_mem_buff_1                              \
    HLS_MAP_TO_MEMORY(mem_buff_1, "plm_data16_4w4r")
#define HLS_MAP_mem_buff_2                              \
    HLS_MAP_TO_MEMORY(mem_buff_2, "plm_data16_4w4r")

#else /* WORDS_PER_DMA */

#error Unsupported DMA width

#endif /* WORDS_PER_DMA */

#define HLS_MAP_mem_hist_1                              \
    HLS_MAP_TO_MEMORY(mem_hist_1, "plm_hist_1w1r")
#define HLS_MAP_mem_hist_2                      \
    HLS_MAP_TO_MEMORY(mem_hist_2, "plm_hist_1w1r")

#define HLS_PROTO(_s)                           \
    HLS_DEFINE_PROTOCOL(_s)

#define HLS_FLAT(_a)                            \
    HLS_FLATTEN_ARRAY(_a);

#define HLS_BREAK_DEP(_a)                       \
    HLS_BREAK_ARRAY_DEPENDENCY(_a)

#define HLS_UNROLL_SIMPLE                       \
    HLS_UNROLL_LOOP(ON)



#if defined(HLS_DIRECTIVES_BASIC)

#define HLS_PIPE_H1(_s)

#define HLS_PIPE_H2(_s)

#define HLS_DWT_XPOSE_CONSTR(_m, _s)


#elif defined(HLS_DIRECTIVES_FAST)

#define HLS_PIPE_H1(_s)                         \
    HLS_PIPELINE_LOOP(HARD_STALL, 1, _s);

#define HLS_PIPE_H2(_s)                         \
    HLS_PIPELINE_LOOP(HARD_STALL, 2, _s);

#define HLS_DWT_XPOSE_CONSTR(_m, _s)                    \
    HLS_CONSTRAIN_ARRAY_MAX_DISTANCE(_m, 2, _s);



#else

#error Unsupported or undefined HLS configuration


#endif /* HLS_DIRECTIVES_* */


#else /* !STRATUS_HLS */

#define HLS_MAP_mem_buff_1
#define HLS_MAP_mem_buff_2
#define HLS_MAP_mem_hist_1
#define HLS_MAP_mem_hist_2

#define HLS_PROTO(_s)
#define HLS_FLAT(_a)
#define HLS_BREAK_DEP(_a)
#define HLS_UNROLL_SIMPLE
#define HLS_PIPE_H1(_s)
#define HLS_PIPE_H2(_s)

#define HLS_DWT_XPOSE_CONSTR(_m, _s)

#endif /* STRATUS_HLS */

#endif /* __VISIONCHIP_DIRECTIVES_HPP_ */
