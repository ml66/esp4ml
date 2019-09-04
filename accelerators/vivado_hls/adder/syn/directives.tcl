# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

set_directive_interface -mode ap_none "top" conf_info_size
set_directive_interface -mode ap_fifo -depth 1 "top" load_ctrl
set_directive_interface -mode ap_fifo -depth 1 "top" store_ctrl
set_directive_data_pack "top" load_ctrl
set_directive_data_pack "top" store_ctrl
set_directive_interface -mode ap_fifo -depth 64 "top" in1
set_directive_interface -mode ap_fifo -depth 32 "top" out
set_directive_loop_tripcount -min 264 -max 264 -avg 264 "top/go"
set_directive_dataflow "top/go"
