# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

# Custom commands

add_files -tb ../inc/weights
add_files -tb ../tb_data
catch {config_array_partition -maximum_size 4096}

# ESP commands

set_directive_interface -mode ap_none "top" conf_info_ninputs
set_directive_interface -mode ap_fifo -depth 1 "top" load_ctrl
set_directive_interface -mode ap_fifo -depth 1 "top" store_ctrl
set_directive_interface -mode ap_fifo -depth 256 "top" in1
set_directive_interface -mode ap_fifo -depth 256 "top" out
set_directive_data_pack "top" load_ctrl
set_directive_data_pack "top" store_ctrl
set_directive_data_pack "top" in1
set_directive_data_pack "top" out
set_directive_loop_tripcount -min 256 -max 256 -avg 256 "top/go"
set_directive_dataflow "top/go"
set_directive_unroll -factor 4 "store/store_label1"
set_directive_unroll -factor 4 "load/load_label0"
set_directive_array_partition -type cyclic -factor 4 -dim 1 "top" _inbuff
set_directive_array_partition -type cyclic -factor 4 -dim 1 "top" _outbuff
