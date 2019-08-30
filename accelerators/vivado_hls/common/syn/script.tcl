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

foreach dma [list 32 64] {

    # Create project
    open_project "${ACCELERATOR}_dma${dma}"

    set_top "${ACCELERATOR}_dma${dma}"

    add_files [glob ../src/*] -cflags "-DDMA${dma}"
    add_files [glob ../inc/*] -cflags "-DDMA${dma}"
    add_files -tb ../tb/tb.cc -cflags "-Wno-unknown-pragmas -Wno-unknown-pragmas -DDMA${dma}"

    open_solution "${ACCELERATOR}_acc"

    # Setup technology
    set fpga_techs [list "virtex7" "zynq7000" "virtexup"]

    if {[lsearch $fpga_techs $TECH] >= 0} {

	if {$TECH eq "virtex7"} {
	    set_part "xc7v2000tflg1925-2"
	}
	if {$TECH eq "zynq7000"} {
	    set_part "xc7z020clg484-1"
	}
	if {$TECH eq "virtexup"} {
	    set_part "xcvu9p-flga2104-2L-e"
	}
    }

    # Config HLS
    create_clock -period 10 -name default
    config_compile -no_signed_zeros=0 -unsafe_math_optimizations=0
    config_schedule -effort medium -relax_ii_for_timing=0 -verbose=0
    config_bind -effort medium
    config_sdx -optimization_level none -target none
    set_clock_uncertainty 12.5%
    source "./directives.tcl"

    # C Simulation
    csim_design

    # HLS
    csynth_design

    # C-RTL Cosimulation
    # cosim_design

    # Export RTL
    export_design -rtl verilog -format ip_catalog

}

exit
