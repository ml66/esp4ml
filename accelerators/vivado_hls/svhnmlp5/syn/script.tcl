# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

############################################################
# Project Parameters
############################################################

#
# Technology Libraries
#
set TECH $::env(TECH)
set ESP_ROOT $::env(ESP_ROOT)
set ACCELERATOR $::env(ACCELERATOR)
set TECH_PATH "$ESP_ROOT/tech/$TECH"

foreach dma [list 64] {

    foreach block [list 1 2 3 4 5] {

	# Create project
	open_project "${ACCELERATOR}_dma${dma}_blk${block}"

	set_top "top"

	add_files [glob ../src/*] -cflags "-I../inc -DDMA_SIZE=${dma} -DBLOCK=${block} -std=c++0x "
	add_files -tb ../tb/tb.cc -cflags "-I../inc -Wno-unknown-pragmas -Wno-unknown-pragmas -DDMA_SIZE=${dma} -DBLOCK=${block} -std=c++0x "

	if { $block == 1 } {
	    add_files ../src/blocks/myproject_block_layer1.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	} else {
	    add_files -tb ../src/blocks/myproject_block_layer1.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	}
	if { $block == 2 } {
	    add_files ../src/blocks/myproject_block_layer2.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	} else {
	    add_files -tb ../src/blocks/myproject_block_layer2.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	}
	if { $block == 3 } {
	    add_files ../src/blocks/myproject_block_layer3.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	} else {
	    add_files -tb ../src/blocks/myproject_block_layer3.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	}
	if { $block == 4 } {
	    add_files ../src/blocks/myproject_block_layer4.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	} else {
	    add_files -tb ../src/blocks/myproject_block_layer4.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	}
	if { $block == 5 } {
	    add_files ../src/blocks/myproject_block_layer5.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	} else {
	    add_files -tb ../src/blocks/myproject_block_layer5.cpp -cflags "-I../inc -I[file normalize ./nnet_utils] -std=c++0x"
	}

	open_solution "${ACCELERATOR}_acc"

	# Setup technology
	set fpga_techs [list "virtex7" "zynq7000" "virtexup"]

	if {[lsearch $fpga_techs $TECH] >= 0} {

	    if {$TECH eq "virtex7"} {
		set_part "xc7v2000tflg1925-2"
		create_clock -period 10 -name default
	    }
	    if {$TECH eq "zynq7000"} {
		set_part "xc7z020clg484-1"
		create_clock -period 10 -name default
	    }
	    if {$TECH eq "virtexup"} {
		set_part "xcvu9p-flga2104-2L-e"
		create_clock -period 10 -name default
	    }
	}

	# Config HLS
	config_rtl -prefix "${ACCELERATOR}_dma${dma}_blk{block}" 
	config_compile -no_signed_zeros=0 -unsafe_math_optimizations=0
	config_schedule -effort medium -relax_ii_for_timing=0 -verbose=0
	config_bind -effort medium
	config_sdx -optimization_level none -target none
	set_clock_uncertainty 12.5%
	source "./directives.tcl"

	# C Simulation
	csim_design

	# HLS
	# csynth_design

	# # C-RTL Cosimulation
	# add_files -tb ../tb/tb.cc -cflags "-I../inc -Wno-unknown-pragmas -Wno-unknown-pragmas -DDMA_SIZE=${dma} -std=c++0x -DRTL_SIM"
	# cosim_design

	# Export RTL
	# export_design -rtl verilog -format ip_catalog
    }
}

exit
