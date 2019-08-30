############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2018 Xilinx, Inc. All Rights Reserved.
############################################################

set_directive_interface -mode ap_none "adder_dma${dma}" conf_info_size
set_directive_interface -mode ap_fifo -depth 1 "adder_dma${dma}" load_ctrl
set_directive_interface -mode ap_fifo -depth 1 "adder_dma${dma}" store_ctrl
set_directive_data_pack "adder_dma${dma}" load_ctrl
set_directive_data_pack "adder_dma${dma}" store_ctrl
set_directive_interface -mode ap_fifo -depth 64 "adder_dma${dma}" in1
set_directive_interface -mode ap_fifo -depth 32 "adder_dma${dma}" out
set_directive_loop_tripcount -min 1 -max 1 -avg 1 "adder_dma${dma}/go"
