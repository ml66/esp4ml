############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2018 Xilinx, Inc. All Rights Reserved.
############################################################

# TODO Repeat the line below for each configuration parameters int the xml file
set_directive_interface -mode ap_none "<accelerator_name>_basic_dma32" conf_info_size

set_directive_interface -mode ap_fifo -depth 1 "<accelerator_name>_basic_dma32" load_ctrl
set_directive_interface -mode ap_fifo -depth 1 "<accelerator_name>_basic_dma32" store_ctrl
set_directive_data_pack "<accelerator_name>_basic_dma32" load_ctrl
set_directive_data_pack "<accelerator_name>_basic_dma32" store_ctrl

# TODO Adjust depth according to CHUNK size
set_directive_interface -mode ap_fifo -depth 32 "<accelerator_name>_basic_dma32" in1
set_directive_interface -mode ap_fifo -depth 32 "<accelerator_name>_basic_dma32" out

set_directive_loop_tripcount -min 1 -max 1 -avg 1 "<accelerator_name>_basic_dma32/go"
