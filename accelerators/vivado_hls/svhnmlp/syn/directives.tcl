# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

# Custom commands

add_files -tb ../inc/weights
add_files -tb ../tb_data
catch {config_array_partition -maximum_size 4096}

# TODO Repeat the line below for each configuration parameters int the xml file
set_directive_interface -mode ap_none "top" conf_info_ninputs

set_directive_interface -mode ap_fifo -depth 1 "top" load_ctrl
set_directive_interface -mode ap_fifo -depth 1 "top" store_ctrl
set_directive_data_pack "top" load_ctrl
set_directive_data_pack "top" store_ctrl

# TODO Adjust depth according to CHUNK size
set_directive_interface -mode ap_fifo -depth 512 "top" in1
set_directive_interface -mode ap_fifo -depth 8 "top" out

set_directive_loop_tripcount -min 256 -max 256 -avg 256 "top/go"
