# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

include ../../common/common.mk

HLS_TCL=script.tcl
PROJECT_NAME="$(TARGET_NAME)_prj"
RTL_FOLDER="$(TARGET_NAME)_basic_dma32"

all: hls

hls: 
	@vivado_hls -f $(HLS_TCL)

install:
	@mkdir -p $(RTL_OUT)
	@mkdir -p $(RTL_OUT)/$(RTL_FOLDER)
ifneq ("$(TARGET_NAME)","")
	@cp ../$(TARGET_NAME).xml $(RTL_OUT)
endif
	@echo "HERE"
	@cp $(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/hdl/verilog/* $(RTL_OUT)/$(RTL_FOLDER)
	@if [ -e $(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/hdl/ip/* ]; then \
		cp $(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/hdl/ip/* $(RTL_OUT)/$(RTL_FOLDER); \
	fi;
	@if [ -e $(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/tmp.srcs/sources_1/ip ]; then \
		for dir in $$(ls $(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/tmp.srcs/sources_1/ip); do \
			indir=$(TARGET_NAME)/$(TARGET_NAME)_acc/impl/ip/tmp.srcs/sources_1/ip/$$dir; \
			infile=$$indir/*.xci; \
			cp $$infile $(RTL_OUT)/$(RTL_FOLDER); \
		done; \
	fi

# Clean out undesirable junk files from the project directory
# Uses the automatically created clean_all target from Makefile.prj
clean:
	@rm -rf $(TARGET_NAME) *.log
	@rm -f transcript vsim* *.wlf data.out
	@rm -f *~ *.bak *.BAK
	@rm -rf work debussy* vfast*
	@rm -f msg_help.html
	@rm -rf core
	@rm -rf core.*
	@rm -rf .stack.*
	@rm -rf *irun.history*
	@rm -rf *.pro *.pro.user
	@rm -rf *.log
	@rm -rf *.shm
	@rm -rf *.diag
	@rm -rf *.key
	@rm -rf *.csv
	@rm -rf .simvision
	@rm -rf .Xil
	@rm -rf .fail*

distclean: clean
	@rm -rf $(RTL_OUT)

.PHONY: install clean distclean

