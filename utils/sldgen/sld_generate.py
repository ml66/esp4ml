#!/usr/bin/env python3

# Copyright (c) 2011-2019 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

###############################################################################
#
# ESP Wrapper Generator for Accelerators
#
###############################################################################

import shutil
import os
import xml.etree.ElementTree
import glob
import sys
import re
import math

def get_immediate_subdirectories(a_dir):
  return [name for name in os.listdir(a_dir)
        if os.path.isdir(os.path.join(a_dir, name))]

def print_usage():
  print("Usage                    : ./sld_generate.py <dma_width> <rtl_path> <template_path> <out_path>")
  print("")
  print("")
  print("      <dma_width>        : Bit-width for the DMA channel (currently supporting 32 bits only)")
  print("")
  print("      <rtl_path>         : Path to accelerators' RTL for the target technology")
  print("")
  print("      <template_path>    : Path to file templates")
  print("")
  print("      <out_path>         : Output path")
  print("")

#
### Data structures #
#

class Parameter():
  def __init__(self):
    self.name = ""
    self.desc = ""
    self.size = 0
    self.reg = 0
    self.readonly = False
    self.value = 0

  def __str__(self):
    return "            " + self.name + ", " + self.desc + ", " + str(self.size) + "bits, register " + str(self.reg) + ", def. value " + str(self.value) + "\n"

class Implementation():
  def __init__(self):
    self.name = ""
    self.dma_width = 0

  def __str__(self):
    return self.name

class Accelerator():
  def __init__(self):
    self.name = ""
    self.hlscfg = []
    self.desc = ""
    self.data = 0
    self.device_id = ""
    self.hls_tool = ""
    self.param = []

  def __str__(self):
    params = "\n          {\n"
    for i in range(0, len(self.param)):
      params = params + str(self.param[i])
    params = params + "          }"
    cfgs = "["
    for i in range(0, len(self.hlscfg) - 1):
      cfgs = cfgs + str(self.hlscfg[i]) + " | "
    cfgs = cfgs + str(self.hlscfg[len(self.hlscfg) - 1])
    cfgs = cfgs + "]"
    return "          " + self.name + "_" + cfgs + ": " + self.desc + ", " + str(self.data) + "MB, ID " + self.device_id + str(params)

class AxiAccelerator():
  def __init__(self):
    self.name = ""
    self.desc = ""
    self.device_id = ""
    self.clocks = [ ]
    self.resets = [ ]
    self.interrupt = ""
    self.axi_prefix = ""
    self.apb_prefix = ""
    self.addr_widh = 32
    self.id_width = 8
    self.user_width = 0

  def __str__(self):
    return "          " + self.name + ": " + self.desc + ", ID " + self.device_id

class Component():
  def __init__(self):
    self.name = ""
    self.hlscfg = []

  def __str__(self):
    cfgs = "["
    for i in range(0, len(self.hlscfg) - 1):
      cfgs = cfgs + str(self.hlscfg[i]) + " | "
    cfgs = cfgs + str(self.hlscfg[len(self.hlscfg) - 1])
    cfgs = cfgs + "]"
    return "          " + self.name + "_" + cfgs

#
### Globals (updated based on DMA_WIDTH)
#
bits_per_line = 128
phys_addr_bits = 32
word_offset_bits = 2
byte_offset_bits = 2
offset_bits = 4


#
### VHDL writer ###
#

def gen_device_id(accelerator_list, axi_accelerator_list, template_dir, out_dir):
  f = open(out_dir + '/sld_devices.vhd', 'w')
  with open(template_dir + '/sld_devices.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<hlscfg>>") >= 0:
        conf_id = 1
        for acc in accelerator_list:
          for cfg in acc.hlscfg:
            f.write("  constant HLSCFG_" + acc.name.upper() + "_" + cfg.name.upper() + " " + ": hlscfg_t := " + str(conf_id) + ";\n")
            conf_id = conf_id + 1
      elif tline.find("-- <<devid>>") >= 0:
        for acc in accelerator_list:
          f.write("  constant SLD_" + acc.name.upper() + " " + ": devid_t := 16#" + acc.device_id + "#;\n")
        for acc in axi_accelerator_list:
          f.write("  constant SLD_" + acc.name.upper() + " " + ": devid_t := 16#" + acc.device_id + "#;\n")
      elif tline.find("-- <<ddesc>>") >= 0:
        for acc in accelerator_list + axi_accelerator_list:
          desc = acc.desc
          if len(acc.desc) < 31:
            desc = acc.desc + (31 - len(acc.desc))*" "
          elif len(acc.desc) > 31:
            desc = acc.desc[0:30]
          f.write("    SLD_" + acc.name.upper() + " " + "=> \"" + desc + "\",\n")
      else:
        f.write(tline)


def write_axi_acc_interface(f, acc, dma_width):
  for clk in acc.clocks:
    f.write("  " + clk + " : in std_logic;\n")
  for rst in acc.resets:
    f.write("  " + rst + " : in std_logic;\n")
  f.write("  " + acc.apb_prefix + "psel : in std_ulogic;\n")
  f.write("  " + acc.apb_prefix + "penable : in std_ulogic;\n")
  f.write("  " + acc.apb_prefix + "paddr : in std_logic_vector(31 downto 0);\n")
  f.write("  " + acc.apb_prefix + "pwrite : in std_ulogic;\n")
  f.write("  " + acc.apb_prefix + "pwdata : in std_logic_vector(31 downto 0);\n")
  f.write("  " + acc.apb_prefix + "prdata : out std_Logic_vector(31 downto 0);\n")
  f.write("  " + acc.apb_prefix + "pready : out std_logic;\n")
  f.write("  " + acc.apb_prefix + "pslverr : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "awid : out std_logic_vector(" + str(acc.id_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awaddr : out std_logic_vector(" + str(acc.addr_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awlen : out std_logic_vector(7 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awsize : out std_logic_vector(2 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awburst : out std_logic_vector(1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awlock : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "awcache : out std_logic_vector(3 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awprot : out std_logic_vector(2 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awvalid : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "awqos : out std_logic_vector(3 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awatop : out std_logic_vector(5 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awregion : out std_logic_vector(3 downto 0);\n")
  if acc.user_width != "0":
    f.write("  " + acc.axi_prefix + "awuser : out std_logic_vector(" + str(acc.user_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "awready : in std_logic;\n")
  f.write("  " + acc.axi_prefix + "wdata : out std_logic_vector (" + str(dma_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "wstrb : out std_logic_vector (" + str(dma_width) + "/8 - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "wlast : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "wvalid : out std_logic;\n")
  if acc.user_width != "0":
    f.write("  " + acc.axi_prefix + "wuser : out std_logic_vector(" + str(acc.user_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "wready : in std_logic;\n")
  f.write("  " + acc.axi_prefix + "arid  : out std_logic_vector (" + str(acc.id_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "araddr : out std_logic_vector (" + str(acc.addr_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arlen : out std_logic_vector (7 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arsize : out std_logic_vector (2 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arburst : out std_logic_vector (1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arlock : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "arcache : out std_logic_vector (3 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arprot : out std_logic_vector (2 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arvalid : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "arqos : out std_logic_vector (3 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arregion : out std_logic_vector(3 downto 0);\n")
  if acc.user_width != "0":
    f.write("  " + acc.axi_prefix + "aruser : out std_logic_vector(" + str(acc.user_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "arready : in std_logic;\n")
  f.write("  " + acc.axi_prefix + "rready : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "rid : in std_logic_vector (" + str(acc.id_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "rdata : in std_logic_vector (" + str(dma_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "rresp : in std_logic_vector (1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "rlast : in std_logic;\n")
  f.write("  " + acc.axi_prefix + "rvalid : in std_logic;\n")
  if acc.user_width != "0":
    f.write("  " + acc.axi_prefix + "ruser : in std_logic_vector(" + str(acc.user_width) + " - 1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "bready : out std_logic;\n")
  f.write("  " + acc.axi_prefix + "bid : in std_logic_vector (" + str(acc.id_width) + "-1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "bresp : in std_logic_vector (1 downto 0);\n")
  f.write("  " + acc.axi_prefix + "bvalid : in std_logic")
  if acc.user_width != "0" or acc.interrupt != "":
    f.write(";\n")
  if acc.user_width != "0":
    f.write("  " + acc.axi_prefix + "buser : in std_logic_vector(" + str(acc.user_width) + "-1 downto 0)")
    if acc.interrupt != "":
      f.write(";\n")
  if acc.interrupt != "":
    f.write("  " + acc.interrupt + " : out std_logic\n")

def bind_apb3(f, prefix):
  f.write("      " + prefix + "psel => apbi.psel(pindex),\n");
  f.write("      " + prefix + "penable => apbi.penable,\n");
  f.write("      " + prefix + "paddr => apbi.paddr,\n");
  f.write("      " + prefix + "pwrite => apbi.pwrite,\n");
  f.write("      " + prefix + "pwdata => apbi.pwdata,\n");
  f.write("      " + prefix + "prdata => apbo(pindex).prdata,\n");
  f.write("      " + prefix + "pready => open,\n");
  f.write("      " + prefix + "pslverr => open, -- TODO: handle APB3 error\n");


def bind_axi(f, acc, dma_width):
  f.write("      " + acc.axi_prefix + "awid => mosi(0).aw.id(" + str(acc.id_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "awaddr => mosi(0).aw.addr(" + str(acc.addr_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "awlen => mosi(0).aw.len,\n")
  f.write("      " + acc.axi_prefix + "awsize => mosi(0).aw.size,\n")
  f.write("      " + acc.axi_prefix + "awburst => mosi(0).aw.burst,\n")
  f.write("      " + acc.axi_prefix + "awlock => mosi(0).aw.lock,\n")
  f.write("      " + acc.axi_prefix + "awcache => mosi(0).aw.cache,\n")
  f.write("      " + acc.axi_prefix + "awprot => mosi(0).aw.prot,\n")
  f.write("      " + acc.axi_prefix + "awvalid => mosi(0).aw.valid,\n")
  f.write("      " + acc.axi_prefix + "awqos => mosi(0).aw.qos,\n")
  f.write("      " + acc.axi_prefix + "awatop => mosi(0).aw.atop,\n")
  f.write("      " + acc.axi_prefix + "awregion => mosi(0).aw.region,\n")
  if acc.user_width != "0":
    f.write("      " + acc.axi_prefix + "awuser => mosi(0).aw.user(" + str(acc.user_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "awready => somi(0).aw.ready,\n")
  f.write("      " + acc.axi_prefix + "wdata => mosi(0).w.data,\n")
  f.write("      " + acc.axi_prefix + "wstrb => mosi(0).w.strb,\n")
  f.write("      " + acc.axi_prefix + "wlast => mosi(0).w.last,\n")
  f.write("      " + acc.axi_prefix + "wvalid => mosi(0).w.valid,\n")
  if acc.user_width != "0":
    f.write("      " + acc.axi_prefix + "wuser => mosi(0).w.user(" + str(acc.user_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "wready => somi(0).w.ready,\n")
  f.write("      " + acc.axi_prefix + "arid  => mosi(0).ar.id(" + str(acc.id_width) + " - 1 downto 0) ,\n")
  f.write("      " + acc.axi_prefix + "araddr => mosi(0).ar.addr(" + str(acc.addr_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "arlen => mosi(0).ar.len,\n")
  f.write("      " + acc.axi_prefix + "arsize => mosi(0).ar.size,\n")
  f.write("      " + acc.axi_prefix + "arburst => mosi(0).ar.burst,\n")
  f.write("      " + acc.axi_prefix + "arlock => mosi(0).ar.lock,\n")
  f.write("      " + acc.axi_prefix + "arcache => mosi(0).ar.cache,\n")
  f.write("      " + acc.axi_prefix + "arprot => mosi(0).ar.prot,\n")
  f.write("      " + acc.axi_prefix + "arvalid => mosi(0).ar.valid,\n")
  f.write("      " + acc.axi_prefix + "arqos => mosi(0).ar.qos,\n")
  f.write("      " + acc.axi_prefix + "arregion => mosi(0).ar.region,\n")
  if acc.user_width != "0":
    f.write("      " + acc.axi_prefix + "aruser => mosi(0).ar.user(" + str(acc.user_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "arready => somi(0).ar.ready,\n")
  f.write("      " + acc.axi_prefix + "rready => mosi(0).r.ready,\n")
  f.write("      " + acc.axi_prefix + "rid => somi(0).r.id(" + str(acc.id_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "rdata => somi(0).r.data,\n")
  f.write("      " + acc.axi_prefix + "rresp => somi(0).r.resp,\n")
  f.write("      " + acc.axi_prefix + "rlast => somi(0).r.last,\n")
  f.write("      " + acc.axi_prefix + "rvalid => somi(0).r.valid,\n")
  if acc.user_width != "0":
    f.write("      " + acc.axi_prefix + "ruser => somi(0).r.user(" + str(acc.user_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "bready => mosi(0).b.ready,\n")
  f.write("      " + acc.axi_prefix + "bid => somi(0).b.id(" + str(acc.id_width) + " - 1 downto 0),\n")
  f.write("      " + acc.axi_prefix + "bresp => somi(0).b.resp,\n")
  f.write("      " + acc.axi_prefix + "bvalid => somi(0).b.valid")
  if acc.user_width != "0":
    f.write(",\n")
    f.write("      " + acc.axi_prefix + "buser => somi(0).b.user")


def tie_unused_axi(f, acc, dma_width):
  f.write("      mosi(0).aw.id(XID_WIDTH - 1 downto " + str(acc.id_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).aw.addr(GLOB_PHYS_ADDR_BITS - 1 downto " + str(acc.addr_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).aw.user(XUSER_WIDTH - 1 downto " + str(acc.user_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).w.user(XUSER_WIDTH - 1 downto " + str(acc.user_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).ar.id(XID_WIDTH - 1 downto " + str(acc.id_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).ar.addr(GLOB_PHYS_ADDR_BITS - 1 downto " + str(acc.addr_width) + ") <= (others => '0');\n")
  f.write("      mosi(0).ar.user(XUSER_WIDTH - 1 downto " + str(acc.user_width) + ") <= (others => '0');\n")
  f.write("      somi(0).r.id(XID_WIDTH - 1 downto " + str(acc.id_width) + ") <= (others => '0');\n")
  f.write("      somi(0).r.user(XUSER_WIDTH - 1 downto " + str(acc.user_width) + ") <= (others => '0');\n")
  f.write("      somi(0).b.id(XID_WIDTH - 1 downto " + str(acc.id_width) + ") <= (others => '0');\n")
  f.write("      somi(0).b.user(XUSER_WIDTH - 1 downto " + str(acc.user_width) + ") <= (others => '0');\n")


def write_axi_acc_port_map(f, acc, dma_width):
  f.write("    port map(\n")
  for clk in acc.clocks:
    f.write("      " + clk + " => clk,\n")
  for rst in acc.resets:
    f.write("      " + rst + " => rst,\n")
  bind_apb3(f, acc.apb_prefix)
  bind_axi(f, acc, dma_width)
  if acc.interrupt != "":
    f.write(",\n")
    f.write("      " + acc.interrupt + " => acc_done\n")
  f.write("    );\n")


def write_acc_interface(f, acc, dma_width, rst, is_vivadohls_if):
  for param in acc.param:
    if not param.readonly:
      spacing = " "
      if 17 - len(param.name) > 0:
        spacing = (17-len(param.name))*" "
      f.write("      conf_info_" + param.name + spacing + ": in  std_logic_vector(" + str(param.size - 1) + " downto 0);\n")

  if not is_vivadohls_if:
    f.write("      clk                        : in  std_ulogic;\n")
    spacing = (27-len(rst))*" "
    f.write("      " + rst + spacing       + ": in  std_ulogic;\n")
    f.write("      conf_done                  : in  std_ulogic;\n")
    f.write("      dma_read_ctrl_valid        : out std_ulogic;\n")
    f.write("      dma_read_ctrl_ready        : in  std_ulogic;\n")
    f.write("      dma_read_ctrl_data_index   : out std_logic_vector(" + str(31) + " downto 0);\n")
    f.write("      dma_read_ctrl_data_length  : out std_logic_vector(" + str(31) + " downto 0);\n")
    f.write("      dma_read_ctrl_data_size    : out std_logic_vector(" + str(2) + " downto 0);\n")
    f.write("      dma_write_ctrl_valid       : out std_ulogic;\n")
    f.write("      dma_write_ctrl_ready       : in  std_ulogic;\n")
    f.write("      dma_write_ctrl_data_index  : out std_logic_vector(" + str(31) + " downto 0);\n")
    f.write("      dma_write_ctrl_data_length : out std_logic_vector(" + str(31) + " downto 0);\n")
    f.write("      dma_write_ctrl_data_size   : out std_logic_vector(" + str(2) + " downto 0);\n")
    f.write("      dma_read_chnl_valid        : in  std_ulogic;\n")
    f.write("      dma_read_chnl_ready        : out std_ulogic;\n")
    f.write("      dma_read_chnl_data         : in  std_logic_vector(" + str(dma_width - 1) + " downto 0);\n")
    f.write("      dma_write_chnl_valid       : out std_ulogic;\n")
    f.write("      dma_write_chnl_ready       : in  std_ulogic;\n")
    f.write("      dma_write_chnl_data        : out std_logic_vector(" + str(dma_width - 1) + " downto 0);\n")
    f.write("      acc_done                   : out std_ulogic\n")
  else:
    f.write("      ap_clk                     : in  std_ulogic;\n")
    spacing = (27-len(rst))*" "
    f.write("      " + rst + spacing       + ": in  std_ulogic;\n")
    f.write("      ap_start                   : in  std_ulogic;\n")
    f.write("      ap_done                    : out std_ulogic;\n")
    f.write("      ap_idle                    : out std_ulogic;\n")
    f.write("      ap_ready                   : out std_ulogic;\n")
    f.write("      out_word_V_din             : out std_logic_vector (" + str(dma_width - 1) + " downto 0);\n")
    f.write("      out_word_V_full_n          : in  std_logic;\n")
    f.write("      out_word_V_write           : out std_logic;\n")
    f.write("      in1_word_V_dout            : in  std_logic_vector (" + str(dma_width - 1) + " downto 0);\n")
    f.write("      in1_word_V_empty_n         : in  std_logic;\n")
    f.write("      in1_word_V_read            : out std_logic;\n")
    f.write("      load_ctrl_din              : out std_logic_vector (" + str(95) + " downto 0);\n")
    f.write("      load_ctrl_full_n           : in  std_logic;\n")
    f.write("      load_ctrl_write            : out std_logic;\n")
    f.write("      store_ctrl_din             : out std_logic_vector (" + str(95) + " downto 0);\n")
    f.write("      store_ctrl_full_n          : in  std_logic;\n")
    f.write("      store_ctrl_write           : out std_logic\n")

def write_acc_signals(f, dma_width):
  f.write("\n")
  f.write("signal rst : std_ulogic;\n")
  f.write("\n")
  f.write("-- signals for start fsm\n")
  f.write("\n")
  f.write("type start_state_t is (low, high);\n")
  f.write("signal start_state, start_state_next : start_state_t;\n")
  f.write("signal ap_start_reg : std_ulogic;\n")
  f.write("signal ap_start_reg_next : std_ulogic;\n")
  f.write("\n")
  f.write("signal acc_done_int : std_ulogic;\n")
  f.write("\n")
  f.write("-- signals for ctrl fsm\n")
  f.write("\n")
  f.write("signal load_ctrl_write   : std_ulogic;\n")
  f.write("signal load_ctrl_full_n  : std_ulogic;\n")
  f.write("signal load_ctrl_din     : std_logic_vector(95 downto 0);\n")
  f.write("signal store_ctrl_write  : std_ulogic;\n")
  f.write("signal store_ctrl_full_n : std_ulogic;\n")
  f.write("signal store_ctrl_din    : std_logic_vector(95 downto 0);\n")
  f.write("\n")
  f.write("signal dma_read_ctrl_data : std_logic_vector(95 downto 0);\n")
  f.write("signal dma_write_ctrl_data : std_logic_vector(95 downto 0);\n")
  f.write("signal dma_read_ctrl_valid_n : std_ulogic;\n")
  f.write("signal dma_write_ctrl_valid_n : std_ulogic;\n")
  f.write("signal dma_read_chnl_ready_n : std_ulogic;\n")
  f.write("signal dma_write_chnl_valid_n : std_ulogic;\n")
  f.write("\n")
  f.write("signal load_ctrl_full : std_ulogic;\n")
  f.write("signal store_ctrl_full : std_ulogic;\n")
  f.write("signal load_chnl_empty : std_ulogic;\n")
  f.write("signal store_chnl_full : std_ulogic;\n")
  f.write("\n")
  f.write("signal load_chnl_empty_n : std_ulogic;\n")
  f.write("signal load_chnl_read    : std_ulogic;\n")
  f.write("signal load_chnl_data    : std_logic_vector(" + str(dma_width - 1) + " downto 0);\n")
  f.write("signal store_chnl_write  : std_ulogic;\n")
  f.write("signal store_chnl_full_n : std_ulogic;\n")
  f.write("signal store_chnl_data   : std_logic_vector(" + str(dma_width - 1) + " downto 0);\n")
  f.write("\n")
  f.write("component fifo0 is\n")
  f.write("  generic(\n")
  f.write("    depth : integer := 2;\n")
  f.write("    width : integer := " + str(dma_width) + ");\n")
  f.write("  port(\n")
  f.write("    clk             : in std_logic;\n")
  f.write("    rst             : in std_logic;\n")
  f.write("    rdreq           : in std_logic;\n")
  f.write("    wrreq           : in std_logic;\n")
  f.write("    data_in         : in std_logic_vector(width-1 downto 0);\n")
  f.write("    --request registers\n")
  f.write("    empty           : out std_logic;\n")
  f.write("    full            : out std_logic;\n")
  f.write("    data_out        : out std_logic_vector(width-1 downto 0));\n")
  f.write("end component;\n")
  f.write("\n")


def write_acc_port_map(f, acc, dma_width, rst, is_noc_interface, is_vivadohls_if):

  if not is_vivadohls_if:
    f.write("    port map(\n")
    for param in acc.param:
      if not param.readonly:
        spacing = " "
        if 16 - len(param.name) > 0:
          spacing = (16-len(param.name))*" "
        if is_noc_interface:
          f.write("      conf_info_" + param.name + spacing + " => " + "bank(" + acc.name.upper() + "_" + param.name.upper() + "_REG)(" + str(param.size - 1) + " downto 0),\n")
        else:
          f.write("      conf_info_" + param.name + spacing + " => " + "conf_info_" + param.name +",\n")
    f.write("      clk                        => clk,\n")
    spacing = (27-len(rst))*" "
    f.write("      " + rst + spacing       + "=> acc_rst,\n")
    f.write("      conf_done                  => conf_done,\n")
    f.write("      dma_read_ctrl_valid        => dma_read_ctrl_valid,\n")
    f.write("      dma_read_ctrl_ready        => dma_read_ctrl_ready,\n")
    f.write("      dma_read_ctrl_data_index   => dma_read_ctrl_data_index,\n")
    f.write("      dma_read_ctrl_data_length  => dma_read_ctrl_data_length,\n")
    f.write("      dma_read_ctrl_data_size    => dma_read_ctrl_data_size,\n")
    f.write("      dma_write_ctrl_valid       => dma_write_ctrl_valid,\n")
    f.write("      dma_write_ctrl_ready       => dma_write_ctrl_ready,\n")
    f.write("      dma_write_ctrl_data_index  => dma_write_ctrl_data_index,\n")
    f.write("      dma_write_ctrl_data_length => dma_write_ctrl_data_length,\n")
    f.write("      dma_write_ctrl_data_size   => dma_write_ctrl_data_size,\n")
    f.write("      dma_read_chnl_valid        => dma_read_chnl_valid,\n")
    f.write("      dma_read_chnl_ready        => dma_read_chnl_ready,\n")
    f.write("      dma_read_chnl_data         => dma_read_chnl_data,\n")
    f.write("      dma_write_chnl_valid       => dma_write_chnl_valid,\n")
    f.write("      dma_write_chnl_ready       => dma_write_chnl_ready,\n")
    f.write("      dma_write_chnl_data        => dma_write_chnl_data,\n")
    f.write("      acc_done                   => acc_done\n")
    f.write("    );\n")

  else:

    f.write("    port map(\n")
    for param in acc.param:
      if not param.readonly:
        spacing = " "
        if 16 - len(param.name) > 0:
          spacing = (16-len(param.name))*" "
        if is_noc_interface:
          f.write("      conf_info_" + param.name + spacing + " => " + "bank(" + acc.name.upper() + "_" + param.name.upper() + "_REG)(" + str(param.size - 1) + " downto 0),\n")
        else:
          f.write("      conf_info_" + param.name + spacing + " => " + "conf_info_" + param.name +",\n")
    f.write("      ap_clk                     => clk,\n")
    f.write("      ap_rst                     => rst,\n")
    f.write("      ap_start                   => ap_start_reg,\n")
    f.write("      load_ctrl_write            => load_ctrl_write,\n")
    f.write("      load_ctrl_full_n           => load_ctrl_full_n,\n")
    f.write("      load_ctrl_din              => load_ctrl_din, \n")
    f.write("      store_ctrl_write           => store_ctrl_write,\n")
    f.write("      store_ctrl_full_n          => store_ctrl_full_n,\n")
    f.write("      store_ctrl_din             => store_ctrl_din,\n")
    f.write("      in1_word_V_empty_n         => load_chnl_empty_n,\n")
    f.write("      in1_word_V_read            => load_chnl_read,\n")
    f.write("      in1_word_V_dout            => load_chnl_data,\n")
    f.write("      out_word_V_write           => store_chnl_write,\n")
    f.write("      out_word_V_full_n          => store_chnl_full_n,\n")
    f.write("      out_word_V_din             => store_chnl_data,\n")
    f.write("      ap_done                    => acc_done_int,\n")
    f.write("      ap_idle                    => open,\n")
    f.write("      ap_ready                   => open\n")
    f.write("      );\n")
    f.write("\n")
    f.write("  rst <= not acc_rst;\n")
    f.write("  \n")
    f.write("  -- READ CTRL FIFO\n")
    f.write("\n")
    f.write("  load_ctrl_full_n <= not load_ctrl_full;\n")
    f.write("  dma_read_ctrl_valid <= not dma_read_ctrl_valid_n;\n")
    f.write("  \n")
    f.write("  fifo_read_ctrl: fifo0\n")
    f.write("    generic map(\n")
    f.write("      depth => 4,\n")
    f.write("      width => 96)\n")
    f.write("    port map(\n")
    f.write("      clk => clk,\n")
    f.write("      rst => acc_rst,\n")
    f.write("      rdreq => dma_read_ctrl_ready,\n")
    f.write("      wrreq => load_ctrl_write,\n")
    f.write("      data_in => load_ctrl_din,\n")
    f.write("      empty => dma_read_ctrl_valid_n,\n")
    f.write("      full => load_ctrl_full,\n")
    f.write("      data_out => dma_read_ctrl_data);\n")
    f.write("\n")
    f.write("  -- WRITE CTRL FIFO\n")
    f.write("\n")
    f.write("  store_ctrl_full_n <= not store_ctrl_full;\n")
    f.write("  dma_write_ctrl_valid <= not dma_write_ctrl_valid_n;\n")
    f.write("\n")
    f.write("  fifo_write_ctrl: fifo0\n")
    f.write("    generic map(\n")
    f.write("      depth => 4,\n")
    f.write("      width => 96)\n")
    f.write("    port map(\n")
    f.write("      clk => clk,\n")
    f.write("      rst => acc_rst,\n")
    f.write("      rdreq => dma_write_ctrl_ready,\n")
    f.write("      wrreq => store_ctrl_write,\n")
    f.write("      data_in => store_ctrl_din,\n")
    f.write("      empty => dma_write_ctrl_valid_n,\n")
    f.write("      full => store_ctrl_full,\n")
    f.write("      data_out => dma_write_ctrl_data);\n")
    f.write("\n")
    f.write("  -- READ CHANNEL FIFO\n")
    f.write("\n")
    f.write("  load_chnl_empty_n <= not load_chnl_empty;\n")
    f.write("  dma_read_chnl_ready <= not dma_read_chnl_ready_n;\n")
    f.write("\n")
    f.write("  fifo_read_chnl: fifo0\n")
    f.write("    generic map(\n")
    f.write("      depth => 8,\n")
    f.write("      width => " + str(dma_width) + ")\n")
    f.write("    port map(\n")
    f.write("      clk => clk,\n")
    f.write("      rst => acc_rst,\n")
    f.write("      rdreq => load_chnl_read,\n")
    f.write("      wrreq => dma_read_chnl_valid, \n")
    f.write("      data_in => dma_read_chnl_data,\n")
    f.write("      empty => load_chnl_empty,\n")
    f.write("      full => dma_read_chnl_ready_n,\n")
    f.write("      data_out => load_chnl_data);\n")
    f.write("\n")
    f.write("  -- WRITE CHANNEL FIFO\n")
    f.write("\n")
    f.write("  store_chnl_full_n <= not store_chnl_full;\n")
    f.write("  dma_write_chnl_valid <= not dma_write_chnl_valid_n;\n")
    f.write("\n")
    f.write("  fifo_write_chnl: fifo0\n")
    f.write("    generic map(\n")
    f.write("      depth => 8,\n")
    f.write("      width => " + str(dma_width) + ")\n")
    f.write("    port map(\n")
    f.write("      clk => clk,\n")
    f.write("      rst => acc_rst,\n")
    f.write("      rdreq => dma_write_chnl_ready,\n")
    f.write("      wrreq => store_chnl_write,\n")
    f.write("      data_in => store_chnl_data,\n")
    f.write("      empty => dma_write_chnl_valid_n,\n")
    f.write("      full => store_chnl_full,\n")
    f.write("      data_out => dma_write_chnl_data);\n")
    f.write("  \n")
    f.write("  -- START FSM\n")
    f.write("\n")
    f.write("  acc_done <= acc_done_int;\n")
    f.write("  \n")
    f.write("  ap_start_sync: process(clk, acc_rst)\n")
    f.write("  begin\n")
    f.write("    if acc_rst = '0' then\n")
    f.write("      ap_start_reg <= '0';\n")
    f.write("      start_state <= low;\n")
    f.write("    elsif clk'event and clk = '1' then\n")
    f.write("      ap_start_reg <= ap_start_reg_next;\n")
    f.write("      start_state <= start_state_next;\n")
    f.write("    end if;\n")
    f.write("  end process ap_start_sync;\n")
    f.write("\n")
    f.write("  ap_start_fsm: process(ap_start_reg, conf_done, acc_done_int, start_state)\n")
    f.write("    variable state : start_state_t;\n")
    f.write("    variable reg : std_ulogic;\n")
    f.write("\n")
    f.write("  begin\n")
    f.write("\n")
    f.write("    state := start_state;\n")
    f.write("    reg := ap_start_reg;\n")
    f.write("\n")
    f.write("    case start_state is\n")
    f.write("\n")
    f.write("      when low =>\n")
    f.write("        reg := '0';\n")
    f.write("        if (conf_done = '1') then\n")
    f.write("          reg := '1';\n")
    f.write("          state := high;\n")
    f.write("        end if;\n")
    f.write("\n")
    f.write("      when high =>\n")
    f.write("        reg := '1';\n")
    f.write("        if (acc_done_int = '1') then\n")
    f.write("          reg := '0';\n")
    f.write("          state := low;\n")
    f.write("        end if;\n")
    f.write("\n")
    f.write("    end case;\n")
    f.write("    \n")
    f.write("    start_state_next <= state;\n")
    f.write("    ap_start_reg_next <= reg;\n")
    f.write("\n")
    f.write("  end process ap_start_fsm;\n")
    f.write("\n")
    f.write("  ---- CTRL FSM\n")
    f.write("  \n")
    f.write("  dma_read_ctrl_data_size    <= dma_read_ctrl_data(66 downto 64);\n")
    f.write("  dma_read_ctrl_data_length  <= dma_read_ctrl_data(63 downto 32);\n")
    f.write("  dma_read_ctrl_data_index   <= dma_read_ctrl_data(31 downto  0);\n")
    f.write("  dma_write_ctrl_data_size   <= dma_write_ctrl_data(66 downto 64);\n")
    f.write("  dma_write_ctrl_data_length <= dma_write_ctrl_data(63 downto 32);\n")
    f.write("  dma_write_ctrl_data_index  <= dma_write_ctrl_data(31 downto  0);\n")
    f.write("\n")

# TODO replace all hardcoded vector lengths with constants
def write_cache_interface(f, cac, is_llc):
  if (is_llc):
    f.write("      clk                          : in std_ulogic;\n")
    f.write("      rst                          : in std_ulogic;\n")
    f.write("      llc_req_in_valid             : in std_ulogic;\n")
    f.write("      llc_req_in_data_coh_msg      : in std_logic_vector(2 downto 0);\n")
    f.write("      llc_req_in_data_hprot        : in std_logic_vector(1 downto 0);\n")
    f.write("      llc_req_in_data_addr         : in std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_req_in_data_word_offset  : in std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_req_in_data_valid_words  : in std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_req_in_data_line         : in std_logic_vector(" + str(bits_per_line - 1) + "  downto 0);\n")
    f.write("      llc_req_in_data_req_id       : in std_logic_vector(3 downto 0);\n")
    f.write("      llc_dma_req_in_valid             : in std_ulogic;\n")
    f.write("      llc_dma_req_in_data_coh_msg      : in std_logic_vector(2 downto 0);\n")
    f.write("      llc_dma_req_in_data_hprot        : in std_logic_vector(1 downto 0);\n")
    f.write("      llc_dma_req_in_data_addr         : in std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_dma_req_in_data_word_offset  : in std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_dma_req_in_data_valid_words  : in std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_dma_req_in_data_line         : in std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_dma_req_in_data_req_id       : in std_logic_vector(3 downto 0);\n")
    f.write("      llc_rsp_in_valid             : in std_ulogic;\n")
    f.write("      llc_rsp_in_data_coh_msg      : in std_logic_vector(1 downto 0);\n")
    f.write("      llc_rsp_in_data_addr         : in std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_rsp_in_data_line         : in std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_rsp_in_data_req_id       : in std_logic_vector(3 downto 0);\n")
    f.write("      llc_mem_rsp_valid            : in std_ulogic;\n")
    f.write("      llc_mem_rsp_data_line        : in std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_rst_tb_valid             : in std_ulogic;\n")
    f.write("      llc_rst_tb_data              : in std_ulogic;\n")
    f.write("      llc_rsp_out_ready            : in std_ulogic;\n")
    f.write("      llc_dma_rsp_out_ready            : in std_ulogic;\n")
    f.write("      llc_fwd_out_ready            : in std_ulogic;\n")
    f.write("      llc_mem_req_ready            : in std_ulogic;\n")
    f.write("      llc_rst_tb_done_ready        : in std_ulogic;\n")
    f.write("      llc_stats_ready              : in std_ulogic;\n")
    f.write("      llc_req_in_ready             : out std_ulogic;\n")
    f.write("      llc_dma_req_in_ready             : out std_ulogic;\n")
    f.write("      llc_rsp_in_ready             : out std_ulogic;\n")
    f.write("      llc_mem_rsp_ready            : out std_ulogic;\n")
    f.write("      llc_rst_tb_ready             : out std_ulogic;\n")
    f.write("      llc_rsp_out_valid            : out std_ulogic;\n")
    f.write("      llc_rsp_out_data_coh_msg     : out std_logic_vector(1 downto 0);\n")
    f.write("      llc_rsp_out_data_addr        : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_rsp_out_data_line        : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_rsp_out_data_invack_cnt  : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_rsp_out_data_req_id      : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_rsp_out_data_dest_id     : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_rsp_out_data_word_offset : out std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_dma_rsp_out_valid            : out std_ulogic;\n")
    f.write("      llc_dma_rsp_out_data_coh_msg     : out std_logic_vector(1 downto 0);\n")
    f.write("      llc_dma_rsp_out_data_addr        : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_dma_rsp_out_data_line        : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_dma_rsp_out_data_invack_cnt  : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_dma_rsp_out_data_req_id      : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_dma_rsp_out_data_dest_id     : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_dma_rsp_out_data_word_offset : out std_logic_vector(" + str(word_offset_bits - 1) + " downto 0);\n")
    f.write("      llc_fwd_out_valid            : out std_ulogic;\n")
    f.write("      llc_fwd_out_data_coh_msg     : out std_logic_vector(2 downto 0);\n")
    f.write("      llc_fwd_out_data_addr        : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_fwd_out_data_req_id      : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_fwd_out_data_dest_id     : out std_logic_vector(3 downto 0);\n")
    f.write("      llc_mem_req_valid            : out std_ulogic;\n")
    f.write("      llc_mem_req_data_hwrite      : out std_ulogic;\n")
    f.write("      llc_mem_req_data_hsize       : out std_logic_vector(2 downto 0);\n")
    f.write("      llc_mem_req_data_hprot       : out std_logic_vector(1 downto 0);\n")
    f.write("      llc_mem_req_data_addr        : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      llc_mem_req_data_line        : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      llc_stats_valid              : out std_ulogic;\n")
    f.write("      llc_stats_data               : out std_ulogic;\n")
    f.write("      llc_rst_tb_done_valid        : out std_ulogic;\n")
    f.write("      llc_rst_tb_done_data         : out std_ulogic\n")
  else:
    f.write("      clk                       : in  std_ulogic;\n")
    f.write("      rst                       : in  std_ulogic;\n")
    f.write("      l2_cpu_req_valid          : in  std_ulogic;\n")
    f.write("      l2_cpu_req_data_cpu_msg   : in  std_logic_vector(1 downto 0);\n")
    f.write("      l2_cpu_req_data_hsize     : in  std_logic_vector(2 downto 0);\n")
    f.write("      l2_cpu_req_data_hprot     : in  std_logic_vector(1 downto 0);\n")
    f.write("      l2_cpu_req_data_addr      : in  std_logic_vector(" + str(phys_addr_bits - 1) + " downto 0);\n")
    f.write("      l2_cpu_req_data_word      : in  std_logic_vector(" + str(dma_width - 1) + " downto 0);\n")
    f.write("      l2_fwd_in_valid           : in  std_ulogic;\n")
    f.write("      l2_fwd_in_data_coh_msg    : in  std_logic_vector(2 downto 0);\n")
    f.write("      l2_fwd_in_data_addr       : in  std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      l2_fwd_in_data_req_id     : in  std_logic_vector(3 downto 0);\n")
    f.write("      l2_rsp_in_valid           : in  std_ulogic;\n")
    f.write("      l2_rsp_in_data_coh_msg    : in  std_logic_vector(1 downto 0);\n")
    f.write("      l2_rsp_in_data_addr       : in  std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      l2_rsp_in_data_line       : in  std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      l2_rsp_in_data_invack_cnt : in  std_logic_vector(3 downto 0);\n")
    f.write("      l2_flush_valid            : in  std_ulogic;\n")
    f.write("      l2_flush_data             : in  std_ulogic;\n")
    f.write("      l2_rd_rsp_ready           : in  std_ulogic;\n")
    f.write("      l2_inval_ready            : in  std_ulogic;\n")
    f.write("      l2_req_out_ready          : in  std_ulogic;\n")
    f.write("      l2_rsp_out_ready          : in  std_ulogic;\n")
    f.write("      l2_stats_ready            : in  std_ulogic;\n")
    f.write("      flush_done                : out std_ulogic;\n")
    f.write("      l2_cpu_req_ready          : out std_ulogic;\n")
    f.write("      l2_fwd_in_ready           : out std_ulogic;\n")
    f.write("      l2_rsp_in_ready           : out std_ulogic;\n")
    f.write("      l2_flush_ready            : out std_ulogic;\n")
    f.write("      l2_rd_rsp_valid           : out std_ulogic;\n")
    f.write("      l2_rd_rsp_data_line       : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      l2_inval_valid            : out std_ulogic;\n")
    f.write("      l2_inval_data             : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      l2_req_out_valid          : out std_ulogic;\n")
    f.write("      l2_req_out_data_coh_msg   : out std_logic_vector(1 downto 0);\n")
    f.write("      l2_req_out_data_hprot     : out std_logic_vector(1 downto 0);\n")
    f.write("      l2_req_out_data_addr      : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      l2_req_out_data_line      : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      l2_rsp_out_valid          : out std_ulogic;\n")
    f.write("      l2_rsp_out_data_coh_msg   : out std_logic_vector(1 downto 0);\n")
    f.write("      l2_rsp_out_data_req_id    : out std_logic_vector(3 downto 0);\n")
    f.write("      l2_rsp_out_data_to_req    : out std_logic_vector(1 downto 0);\n")
    f.write("      l2_rsp_out_data_addr      : out std_logic_vector(" + str(phys_addr_bits - offset_bits - 1) + " downto 0);\n")
    f.write("      l2_rsp_out_data_line      : out std_logic_vector(" + str(bits_per_line - 1) + " downto 0);\n")
    f.write("      l2_stats_valid            : out std_ulogic;\n")
    f.write("      l2_stats_data             : out std_ulogic\n")


def write_cache_port_map(f, cac, is_llc):
  f.write("    port map(\n")
  if is_llc:
    f.write("      clk                          => clk,\n")
    f.write("      rst                          => rst,\n")
    f.write("      llc_req_in_valid             => llc_req_in_valid,\n")
    f.write("      llc_req_in_data_coh_msg      => llc_req_in_data_coh_msg,\n")
    f.write("      llc_req_in_data_hprot        => llc_req_in_data_hprot,\n")
    f.write("      llc_req_in_data_addr         => llc_req_in_data_addr,\n")
    f.write("      llc_req_in_data_word_offset  => llc_req_in_data_word_offset,\n")
    f.write("      llc_req_in_data_valid_words  => llc_req_in_data_valid_words,\n")
    f.write("      llc_req_in_data_line         => llc_req_in_data_line,\n")
    f.write("      llc_req_in_data_req_id       => llc_req_in_data_req_id,\n")
    f.write("      llc_dma_req_in_valid             => llc_dma_req_in_valid,\n")
    f.write("      llc_dma_req_in_data_coh_msg      => llc_dma_req_in_data_coh_msg,\n")
    f.write("      llc_dma_req_in_data_hprot        => llc_dma_req_in_data_hprot,\n")
    f.write("      llc_dma_req_in_data_addr         => llc_dma_req_in_data_addr,\n")
    f.write("      llc_dma_req_in_data_word_offset  => llc_dma_req_in_data_word_offset,\n")
    f.write("      llc_dma_req_in_data_valid_words  => llc_dma_req_in_data_valid_words,\n")
    f.write("      llc_dma_req_in_data_line         => llc_dma_req_in_data_line,\n")
    f.write("      llc_dma_req_in_data_req_id       => llc_dma_req_in_data_req_id,\n")
    f.write("      llc_rsp_in_valid             => llc_rsp_in_valid,\n")
    f.write("      llc_rsp_in_data_coh_msg      => llc_rsp_in_data_coh_msg,\n")
    f.write("      llc_rsp_in_data_addr         => llc_rsp_in_data_addr,\n")
    f.write("      llc_rsp_in_data_line         => llc_rsp_in_data_line,\n")
    f.write("      llc_rsp_in_data_req_id       => llc_rsp_in_data_req_id,\n")
    f.write("      llc_mem_rsp_valid            => llc_mem_rsp_valid,\n")
    f.write("      llc_mem_rsp_data_line        => llc_mem_rsp_data_line,\n")
    f.write("      llc_rst_tb_valid             => llc_rst_tb_valid,\n")
    f.write("      llc_rst_tb_data              => llc_rst_tb_data,\n")
    f.write("      llc_rsp_out_ready            => llc_rsp_out_ready,\n")
    f.write("      llc_dma_rsp_out_ready            => llc_dma_rsp_out_ready,\n")
    f.write("      llc_fwd_out_ready            => llc_fwd_out_ready,\n")
    f.write("      llc_mem_req_ready            => llc_mem_req_ready,\n")
    f.write("      llc_rst_tb_done_ready        => llc_rst_tb_done_ready,\n")
    f.write("      llc_stats_ready              => llc_stats_ready,\n")
    f.write("      llc_req_in_ready             => llc_req_in_ready,\n")
    f.write("      llc_dma_req_in_ready             => llc_dma_req_in_ready,\n")
    f.write("      llc_rsp_in_ready             => llc_rsp_in_ready,\n")
    f.write("      llc_mem_rsp_ready            => llc_mem_rsp_ready,\n")
    f.write("      llc_rst_tb_ready             => llc_rst_tb_ready,\n")
    f.write("      llc_rsp_out_valid            => llc_rsp_out_valid,\n")
    f.write("      llc_rsp_out_data_coh_msg     => llc_rsp_out_data_coh_msg,\n")
    f.write("      llc_rsp_out_data_addr        => llc_rsp_out_data_addr,\n")
    f.write("      llc_rsp_out_data_line        => llc_rsp_out_data_line,\n")
    f.write("      llc_rsp_out_data_invack_cnt  => llc_rsp_out_data_invack_cnt,\n")
    f.write("      llc_rsp_out_data_req_id      => llc_rsp_out_data_req_id,\n")
    f.write("      llc_rsp_out_data_dest_id     => llc_rsp_out_data_dest_id,\n")
    f.write("      llc_rsp_out_data_word_offset => llc_rsp_out_data_word_offset,\n")
    f.write("      llc_dma_rsp_out_valid            => llc_dma_rsp_out_valid,\n")
    f.write("      llc_dma_rsp_out_data_coh_msg     => llc_dma_rsp_out_data_coh_msg,\n")
    f.write("      llc_dma_rsp_out_data_addr        => llc_dma_rsp_out_data_addr,\n")
    f.write("      llc_dma_rsp_out_data_line        => llc_dma_rsp_out_data_line,\n")
    f.write("      llc_dma_rsp_out_data_invack_cnt  => llc_dma_rsp_out_data_invack_cnt,\n")
    f.write("      llc_dma_rsp_out_data_req_id      => llc_dma_rsp_out_data_req_id,\n")
    f.write("      llc_dma_rsp_out_data_dest_id     => llc_dma_rsp_out_data_dest_id,\n")
    f.write("      llc_dma_rsp_out_data_word_offset => llc_dma_rsp_out_data_word_offset,\n")
    f.write("      llc_fwd_out_valid            => llc_fwd_out_valid,\n")
    f.write("      llc_fwd_out_data_coh_msg     => llc_fwd_out_data_coh_msg,\n")
    f.write("      llc_fwd_out_data_addr        => llc_fwd_out_data_addr,\n")
    f.write("      llc_fwd_out_data_req_id      => llc_fwd_out_data_req_id,\n")
    f.write("      llc_fwd_out_data_dest_id     => llc_fwd_out_data_dest_id,\n")
    f.write("      llc_mem_req_valid            => llc_mem_req_valid,\n")
    f.write("      llc_mem_req_data_hwrite      => llc_mem_req_data_hwrite,\n")
    f.write("      llc_mem_req_data_hsize       => llc_mem_req_data_hsize,\n")
    f.write("      llc_mem_req_data_hprot       => llc_mem_req_data_hprot,\n")
    f.write("      llc_mem_req_data_addr        => llc_mem_req_data_addr,\n")
    f.write("      llc_mem_req_data_line        => llc_mem_req_data_line,\n")
    f.write("      llc_stats_valid              => llc_stats_valid,\n")
    f.write("      llc_stats_data               => llc_stats_data,\n")
    f.write("      llc_rst_tb_done_valid        => llc_rst_tb_done_valid,\n")
    f.write("      llc_rst_tb_done_data         => llc_rst_tb_done_data\n")
  else:
    f.write("      clk                       => clk,\n")
    f.write("      rst                       => rst,\n")
    f.write("      l2_cpu_req_valid          => l2_cpu_req_valid,\n")
    f.write("      l2_cpu_req_data_cpu_msg   => l2_cpu_req_data_cpu_msg,\n")
    f.write("      l2_cpu_req_data_hsize     => l2_cpu_req_data_hsize,\n")
    f.write("      l2_cpu_req_data_hprot     => l2_cpu_req_data_hprot,\n")
    f.write("      l2_cpu_req_data_addr      => l2_cpu_req_data_addr,\n")
    f.write("      l2_cpu_req_data_word      => l2_cpu_req_data_word,\n")
    f.write("      l2_fwd_in_valid           => l2_fwd_in_valid,\n")
    f.write("      l2_fwd_in_data_coh_msg    => l2_fwd_in_data_coh_msg,\n")
    f.write("      l2_fwd_in_data_addr       => l2_fwd_in_data_addr,\n")
    f.write("      l2_fwd_in_data_req_id     => l2_fwd_in_data_req_id,\n")
    f.write("      l2_rsp_in_valid           => l2_rsp_in_valid,\n")
    f.write("      l2_rsp_in_data_coh_msg    => l2_rsp_in_data_coh_msg,\n")
    f.write("      l2_rsp_in_data_addr       => l2_rsp_in_data_addr,\n")
    f.write("      l2_rsp_in_data_line       => l2_rsp_in_data_line,\n")
    f.write("      l2_rsp_in_data_invack_cnt => l2_rsp_in_data_invack_cnt,\n")
    f.write("      l2_flush_valid            => l2_flush_valid,\n")
    f.write("      l2_flush_data             => l2_flush_data,\n")
    f.write("      l2_rd_rsp_ready           => l2_rd_rsp_ready,\n")
    f.write("      l2_inval_ready            => l2_inval_ready,\n")
    f.write("      l2_req_out_ready          => l2_req_out_ready,\n")
    f.write("      l2_rsp_out_ready          => l2_rsp_out_ready,\n")
    f.write("      l2_stats_ready            => l2_stats_ready,\n")
    f.write("      flush_done                => flush_done,\n")
    f.write("      l2_cpu_req_ready          => l2_cpu_req_ready,\n")
    f.write("      l2_fwd_in_ready           => l2_fwd_in_ready,\n")
    f.write("      l2_rsp_in_ready           => l2_rsp_in_ready,\n")
    f.write("      l2_flush_ready            => l2_flush_ready,\n")
    f.write("      l2_rd_rsp_valid           => l2_rd_rsp_valid,\n")
    f.write("      l2_rd_rsp_data_line       => l2_rd_rsp_data_line,\n")
    f.write("      l2_inval_valid            => l2_inval_valid,\n")
    f.write("      l2_inval_data             => l2_inval_data,\n")
    f.write("      l2_req_out_valid          => l2_req_out_valid,\n")
    f.write("      l2_req_out_data_coh_msg   => l2_req_out_data_coh_msg,\n")
    f.write("      l2_req_out_data_hprot     => l2_req_out_data_hprot,\n")
    f.write("      l2_req_out_data_addr      => l2_req_out_data_addr,\n")
    f.write("      l2_req_out_data_line      => l2_req_out_data_line,\n")
    f.write("      l2_rsp_out_valid          => l2_rsp_out_valid,\n")
    f.write("      l2_rsp_out_data_coh_msg   => l2_rsp_out_data_coh_msg,\n")
    f.write("      l2_rsp_out_data_req_id    => l2_rsp_out_data_req_id,\n")
    f.write("      l2_rsp_out_data_to_req    => l2_rsp_out_data_to_req,\n")
    f.write("      l2_rsp_out_data_addr      => l2_rsp_out_data_addr,\n")
    f.write("      l2_rsp_out_data_line      => l2_rsp_out_data_line,\n")
    f.write("      l2_stats_valid            => l2_stats_valid,\n")
    f.write("      l2_stats_data             => l2_stats_data\n")
  f.write("    );\n")


# Component declaration matching HLS-generated verilog
def gen_tech_dep(accelerator_list, cache_list, dma_width, template_dir, out_dir):
  f = open(out_dir + '/allacc.vhd', 'w')
  with open(template_dir + '/allacc.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<accelerators-components>>") < 0:
        f.write(tline)
        continue
      for acc in accelerator_list:
        for impl in acc.hlscfg:
          f.write("\n")
          if acc.hls_tool == 'stratus_hls':
            f.write("  component " + acc.name + "_" + impl.name + "\n")
            f.write("    port (\n")
            write_acc_interface(f, acc, dma_width, "rst", False)
          else:
            f.write("  component " + acc.name + "_" + impl.name + "_top\n")
            f.write("    port (\n")
            write_acc_interface(f, acc, dma_width, "ap_rst", True)
          f.write("    );\n")
          f.write("  end component;\n\n")
          f.write("\n")
  f.close()
  ftemplate.close()
  f = open(out_dir + '/allcaches.vhd', 'w')
  with open(template_dir + '/allcaches.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<caches-components>>") < 0:
        f.write(tline)
        continue
      for cac in cache_list:
        is_llc = cac.name == "llc"
        for impl in cac.hlscfg:
          f.write("\n")
          f.write("  component " + cac.name + "_" + impl + "\n")
          f.write("    port (\n")
          write_cache_interface(f, cac, is_llc)
          f.write("    );\n")
          f.write("  end component;\n\n")
          f.write("\n")
  f.close()
  ftemplate.close()


# Component declaration independent from technology and implementation
def gen_tech_indep(accelerator_list, axi_accelerator_list, cache_list, dma_width, template_dir, out_dir):
  f = open(out_dir + '/genacc.vhd', 'w')
  with open(template_dir + '/genacc.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<accelerators-components>>") < 0:
        f.write(tline)
        continue
      for acc in accelerator_list:
        f.write("\n")
        f.write("  component " + acc.name + "_rtl\n")
        f.write("    generic (\n")
        f.write("      hls_conf  : hlscfg_t\n")
        f.write("    );\n")
        f.write("\n")
        f.write("    port (\n")
        write_acc_interface(f, acc, dma_width, "acc_rst", False)
        f.write("    );\n")
        f.write("  end component;\n\n")
        f.write("\n")
      for acc in axi_accelerator_list:
        f.write("\n")
        f.write("  component " + acc.name + "_wrapper\n")
        f.write("    port (\n")
        write_axi_acc_interface(f, acc, dma_width)
        f.write("    );\n")
        f.write("  end component;\n\n")
        f.write("\n")
  f.close()
  ftemplate.close()
  f = open(out_dir + '/gencaches.vhd', 'w')
  with open(template_dir + '/gencaches.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<caches-components>>") < 0:
        f.write(tline)
        continue
      for cac in cache_list:
        is_llc = cac.name == "llc"
        f.write("\n")
        f.write("  component " + cac.name + "\n")
        f.write("    generic (\n")
        f.write("      sets  : integer;\n")
        f.write("      ways  : integer\n")
        f.write("    );\n")
        f.write("\n")
        f.write("    port (\n")
        write_cache_interface(f, cac, is_llc)
        f.write("    );\n")
        f.write("  end component;\n\n")
        f.write("\n")
  f.close()
  ftemplate.close()


# Mapping from generic components to technology and implementation dependent ones
def gen_tech_indep_impl(accelerator_list, cache_list, dma_width, template_dir, out_dir):
  f = open(out_dir + '/accelerators.vhd', 'w')
  with open(template_dir + '/accelerators.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<accelerators-entities>>") < 0:
        f.write(tline)
        continue
      for acc in accelerator_list:
        f.write("library ieee;\n")
        f.write("use ieee.std_logic_1164.all;\n")
        f.write("use work.sld_devices.all;\n")
        f.write("use work.allacc.all;\n")
        f.write("\n")
        f.write("entity " + acc.name + "_rtl is\n\n")
        f.write("    generic (\n")
        f.write("      hls_conf  : hlscfg_t\n")
        f.write("    );\n")
        f.write("\n")
        f.write("    port (\n")
        write_acc_interface(f, acc, dma_width, "acc_rst", False)
        f.write("    );\n")
        f.write("\n")
        f.write("end entity " + acc.name + "_rtl;\n\n")
        f.write("\n")
        f.write("architecture mapping of " + acc.name + "_rtl is\n\n")
        if acc.hls_tool == 'vivado_hls':
          write_acc_signals(f, dma_width)
        f.write("begin  -- mapping\n\n")
        for impl in acc.hlscfg:
          f.write("\n")
          f.write("  " + impl.name + "_gen: if hls_conf = HLSCFG_" + acc.name.upper() + "_" + impl.name.upper() + " generate\n")
          if acc.hls_tool == 'stratus_hls':
            f.write("    " + acc.name + "_" + impl.name + "_i: " + acc.name + "_" + impl.name + "\n")
            write_acc_port_map(f, acc, dma_width, "rst", False, False)
          else:
            f.write("    " + acc.name + "_" + impl.name + "_top_i: " + acc.name + "_" + impl.name + "_top\n")
            write_acc_port_map(f, acc, dma_width, "rst", False, True)
          f.write("  end generate " +  impl.name + "_gen;\n\n")
        f.write("end mapping;\n\n")
  f.close()
  ftemplate.close()
  f = open(out_dir + '/caches.vhd', 'w')
  with open(template_dir + '/caches.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<caches-entities>>") < 0:
        f.write(tline)
        continue
      for cac in cache_list:
        is_llc = cac.name == "llc"
        f.write("library ieee;\n")
        f.write("use ieee.std_logic_1164.all;\n")
        f.write("use work.sld_devices.all;\n")
        f.write("use work.allcaches.all;\n")
        f.write("\n")
        f.write("entity " + cac.name + " is\n\n")
        f.write("    generic (\n")
        f.write("      sets             : integer;\n")
        f.write("      ways             : integer\n")
        f.write("    );\n")
        f.write("\n")
        f.write("    port (\n")
        write_cache_interface(f, cac, is_llc)
        f.write("    );\n")
        f.write("\n")
        f.write("end entity " + cac.name + ";\n\n")
        f.write("\n")
        f.write("architecture mapping of " + cac.name + " is\n\n")
        f.write("begin  -- mapping\n\n")
        for impl in cac.hlscfg:
          info = re.split('_|x', impl)
          sets = 0
          ways = 0
          this_addr_bits = 0
          this_word_offset_bits = 0
          this_offset_bits = 0
          for item in info:
            if re.match(r'[0-9]+sets', item, re.M|re.I):
              sets = int(item.replace("sets", ""))
            elif re.match(r'[0-9]+ways', item, re.M|re.I):
              ways = int(item.replace("ways", ""))
            elif re.match(r'[0-9]+$', item, re.M|re.I):
              this_word_offset_bits = int(math.log2(int(item)))
            elif re.match(r'[0-9]+line', item, re.M|re.I):
              this_offset_bits = int(int(math.log2((int(item.replace("line", "")))/8) + word_offset_bits))
            elif re.match(r'[0-9]+addr', item, re.M|re.I):
              this_addr_bits = int(item.replace("addr", ""))
          if sets * ways == 0:
            print("    ERROR: hls config must report number of sets and ways, both different from zero")
            sys.exit(1)
          if this_word_offset_bits != word_offset_bits or this_offset_bits != offset_bits or this_addr_bits != phys_addr_bits:
            print("    INFO: skipping cache implementation " + impl + " incompatible with SoC architecture")
            continue
          f.write("\n")
          f.write("  " + impl + "_gen: if sets = " + str(sets) + " and ways = " + str(ways) + " generate\n")
          f.write("    " + cac.name + "_" + impl + "_i: " + cac.name + "_" + impl + "\n")
          write_cache_port_map(f, cac, is_llc)
          f.write("  end generate " +  impl + "_gen;\n\n")
        f.write("end mapping;\n\n")
  f.close()
  ftemplate.close()


# Component declaration of NoC wrappers
def gen_interfaces(accelerator_list, axi_accelerator_list, dma_width, template_dir, out_dir):
  f = open(out_dir + '/sldacc.vhd', 'w')
  with open(template_dir + '/sldacc.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<wrappers-components>>") < 0:
        f.write(tline)
        continue
      for acc in accelerator_list + axi_accelerator_list:
        f.write("\n")
        f.write("  component noc_" + acc.name + "\n")
        f.write("    generic (\n")
        f.write("      hls_conf       : hlscfg_t;\n")
        f.write("      tech           : integer;\n")
        f.write("      local_y        : local_yx;\n")
        f.write("      local_x        : local_yx;\n")
        f.write("      mem_num        : integer;\n")
        f.write("      cacheable_mem_num : integer;\n")
        f.write("      mem_info       : tile_mem_info_vector(0 to MEM_MAX_NUM);\n")
        f.write("      io_y           : local_yx;\n")
        f.write("      io_x           : local_yx;\n")
        f.write("      pindex         : integer;\n")
        f.write("      paddr          : integer;\n")
        f.write("      pmask          : integer;\n")
        f.write("      pirq           : integer;\n")
        f.write("      scatter_gather : integer := 1;\n")
        f.write("      sets           : integer;\n")
        f.write("      ways           : integer;\n")
        f.write("      cache_tile_id  : cache_attribute_array;\n")
        f.write("      cache_y        : yx_vec(0 to 2**NL2_MAX_LOG2 - 1);\n")
        f.write("      cache_x        : yx_vec(0 to 2**NL2_MAX_LOG2 - 1);\n")
        f.write("      has_l2         : integer := 1;\n")
        f.write("      has_dvfs       : integer := 1;\n")
        f.write("      has_pll        : integer;\n")
        f.write("      extra_clk_buf  : integer;\n")
        f.write("      local_apb_en   : std_logic_vector(0 to NAPBSLV - 1)\n")
        f.write("    );\n")
        f.write("\n")
        f.write("    port (\n")
        f.write("      rst               : in  std_ulogic;\n")
        f.write("      clk               : in  std_ulogic;\n")
        f.write("      refclk            : in  std_ulogic;\n")
        f.write("      pllbypass         : in  std_ulogic;\n")
        f.write("      pllclk            : out std_ulogic;\n")
        f.write("      coherence_req_wrreq        : out std_ulogic;\n")
        f.write("      coherence_req_data_in      : out noc_flit_type;\n")
        f.write("      coherence_req_full         : in  std_ulogic;\n")
        f.write("      coherence_fwd_rdreq        : out std_ulogic;\n")
        f.write("      coherence_fwd_data_out     : in  noc_flit_type;\n")
        f.write("      coherence_fwd_empty        : in  std_ulogic;\n")
        f.write("      coherence_rsp_rcv_rdreq    : out std_ulogic;\n")
        f.write("      coherence_rsp_rcv_data_out : in  noc_flit_type;\n")
        f.write("      coherence_rsp_rcv_empty    : in  std_ulogic;\n")
        f.write("      coherence_rsp_snd_wrreq    : out std_ulogic;\n")
        f.write("      coherence_rsp_snd_data_in  : out noc_flit_type;\n")
        f.write("      coherence_rsp_snd_full     : in  std_ulogic;\n")
        f.write("      dma_rcv_rdreq     : out std_ulogic;\n")
        f.write("      dma_rcv_data_out  : in  noc_flit_type;\n")
        f.write("      dma_rcv_empty     : in  std_ulogic;\n")
        f.write("      dma_snd_wrreq     : out std_ulogic;\n")
        f.write("      dma_snd_data_in   : out noc_flit_type;\n")
        f.write("      dma_snd_full      : in  std_ulogic;\n")
        f.write("      coherent_dma_rcv_rdreq     : out std_ulogic;\n")
        f.write("      coherent_dma_rcv_data_out  : in  noc_flit_type;\n")
        f.write("      coherent_dma_rcv_empty     : in  std_ulogic;\n")
        f.write("      coherent_dma_snd_wrreq     : out std_ulogic;\n")
        f.write("      coherent_dma_snd_data_in   : out noc_flit_type;\n")
        f.write("      coherent_dma_snd_full      : in  std_ulogic;\n")
        f.write("      interrupt_wrreq   : out std_ulogic;\n")
        f.write("      interrupt_data_in : out misc_noc_flit_type;\n")
        f.write("      interrupt_full    : in  std_ulogic;\n")
        f.write("      apb_snd_wrreq     : out std_ulogic;\n")
        f.write("      apb_snd_data_in   : out misc_noc_flit_type;\n")
        f.write("      apb_snd_full      : in  std_ulogic;\n")
        f.write("      apb_rcv_rdreq     : out std_ulogic;\n")
        f.write("      apb_rcv_data_out  : in  misc_noc_flit_type;\n")
        f.write("      apb_rcv_empty     : in  std_ulogic;\n")
        f.write("      mon_dvfs_in       : in  monitor_dvfs_type;\n")
        f.write("      mon_acc           : out monitor_acc_type;\n")
        f.write("      mon_cache         : out monitor_cache_type;\n")
        f.write("      mon_dvfs          : out monitor_dvfs_type\n")
        f.write("    );\n")
        f.write("  end component;\n\n")
        f.write("\n")
  f.close()
  ftemplate.close()


def gen_noc_interface(acc, dma_width, template_dir, out_dir, is_axi):
  f = open(out_dir + "/noc_" + acc.name + ".vhd", 'w')

  if not is_axi:
    extra_str = ""
  else:
    extra_str = "2axi"

  template_file = template_dir + '/noc' + extra_str + '_interface.vhd'

  with open(template_file, 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<entity>>") >= 0:
        f.write("entity noc" + "_" + acc.name + " is\n")
      elif tline.find("-- <<architecture>>") >= 0:
        f.write("architecture rtl of noc" + "_" + acc.name + " is\n")
      elif tline.find("-- <<devid>>") >= 0:
        f.write("  constant devid         : devid_t                := SLD_" + acc.name.upper() + ";\n")
      elif tline.find("-- <<tlb_entries>>") >= 0:
        f.write("  constant tlb_entries   : integer                := " + str(acc.data)  + ";\n")
      elif tline.find("-- <<user_registers>>") >= 0:
        for param in acc.param:
          f.write("  -- bank(" + str(param.reg) + "): " + param.desc + "\n")
          f.write("  constant " + acc.name.upper() + "_" + param.name.upper() + "_REG : integer range 0 to MAXREGNUM - 1 := " + str(param.reg) + ";\n\n")
      elif tline.find("-- <<user_read_only>>") >= 0:
        for param in acc.param:
          if param.readonly:
            f.write("    " + acc.name.upper() + "_" + param.name.upper() + "_REG" + (31 - len(acc.name) - len(param.name))*" " + "=> '1',\n")
      elif tline.find("-- <<user_mask>>") >= 0:
        for param in acc.param:
          f.write("    " + acc.name.upper() + "_" + param.name.upper() + "_REG" + (31 - len(acc.name) - len(param.name))*" " + "=> '1',\n")
      elif tline.find("-- <<user_read_only_default>>") >= 0:
        for param in acc.param:
          if param.readonly:
            f.write("    " + acc.name.upper() + "_" + param.name.upper() + "_REG" + (31 - len(acc.name) - len(param.name))*" " + "=> X\"" + format(param.value, '08x') + "\",\n")
      elif tline.find("-- <<axi_unused>>") >= 0:
        tie_unused_axi(f, acc, dma_width)
      elif tline.find("-- <<accelerator_instance>>") >= 0:
        f.write("  " + acc.name + "_rlt_i: " + acc.name)
        if is_axi:
          f.write("_wrapper\n")
          write_axi_acc_port_map(f, acc, dma_width)
        else:
          f.write("_rtl\n")
          f.write("    generic map (\n")
          f.write("      hls_conf => hls_conf\n")
          f.write("    )\n")
          write_acc_port_map(f, acc, dma_width, "acc_rst", True, False)
      else:
        f.write(tline)


def gen_tile_acc(accelerator_list, axi_acceleratorlist, template_dir, out_dir):
  f = open(out_dir + "/tile_acc.vhd", 'w')
  with open(template_dir + '/tile_acc.vhd', 'r') as ftemplate:
    for tline in ftemplate:
      if tline.find("-- <<accelerator-wrappers-gen>>") >= 0:
        for acc in accelerator_list + axi_accelerator_list:
          f.write("  " + acc.name + "_gen: if this_device = SLD_" + acc.name.upper() + " generate\n")
          f.write("    noc_" + acc.name + "_i: noc_" + acc.name + "\n")
          f.write("      generic map (\n")
          f.write("        hls_conf       => this_hls_conf,\n")
          f.write("        tech           => CFG_MEMTECH,\n")
          f.write("        local_y        => this_local_y,\n")
          f.write("        local_x        => this_local_x,\n")
          f.write("        mem_num        => CFG_NMEM_TILE + CFG_SVGA_ENABLE,\n")
          f.write("        cacheable_mem_num => CFG_NMEM_TILE,\n")
          f.write("        mem_info       => tile_acc_mem_list,\n")
          f.write("        io_y           => io_y,\n")
          f.write("        io_x           => io_x,\n")
          f.write("        pindex         => this_pindex,\n")
          f.write("        paddr          => this_paddr,\n")
          f.write("        pmask          => this_pmask,\n")
          f.write("        pirq           => this_pirq,\n")
          f.write("        scatter_gather => this_scatter_gather,\n")
          f.write("        sets           => CFG_ACC_L2_SETS,\n")
          f.write("        ways           => CFG_ACC_L2_WAYS,\n")
          f.write("        cache_tile_id  => cache_tile_id,\n")
          f.write("        cache_y        => cache_y,\n")
          f.write("        cache_x        => cache_x,\n")
          f.write("        has_l2         => this_has_l2,\n")
          f.write("        has_dvfs       => this_has_dvfs,\n")
          f.write("        has_pll        => this_has_pll,\n")
          f.write("        extra_clk_buf  => this_extra_clk_buf,\n")
          f.write("        local_apb_en   => this_local_apb_mask)\n")
          f.write("      port map (\n")
          f.write("        rst               => rst,\n")
          f.write("        clk               => clk_feedthru,\n")
          f.write("        refclk            => refclk,\n")
          f.write("        pllbypass         => pllbypass,\n")
          f.write("        pllclk            => clk_feedthru,\n")
          f.write("        coherence_req_wrreq        => coherence_req_wrreq,\n")
          f.write("        coherence_req_data_in      => coherence_req_data_in,\n")
          f.write("        coherence_req_full         => coherence_req_full,\n")
          f.write("        coherent_dma_rcv_rdreq     => coherent_dma_rcv_rdreq,\n")
          f.write("        coherent_dma_rcv_data_out  => coherent_dma_rcv_data_out,\n")
          f.write("        coherent_dma_rcv_empty     => coherent_dma_rcv_empty,\n")
          f.write("        coherence_fwd_rdreq        => coherence_fwd_rdreq,\n")
          f.write("        coherence_fwd_data_out     => coherence_fwd_data_out,\n")
          f.write("        coherence_fwd_empty        => coherence_fwd_empty,\n")
          f.write("        coherent_dma_snd_wrreq     => coherent_dma_snd_wrreq,\n")
          f.write("        coherent_dma_snd_data_in   => coherent_dma_snd_data_in,\n")
          f.write("        coherent_dma_snd_full      => coherent_dma_snd_full,\n")
          f.write("        coherence_rsp_rcv_rdreq    => coherence_rsp_rcv_rdreq,\n")
          f.write("        coherence_rsp_rcv_data_out => coherence_rsp_rcv_data_out,\n")
          f.write("        coherence_rsp_rcv_empty    => coherence_rsp_rcv_empty,\n")
          f.write("        coherence_rsp_snd_wrreq    => coherence_rsp_snd_wrreq,\n")
          f.write("        coherence_rsp_snd_data_in  => coherence_rsp_snd_data_in,\n")
          f.write("        coherence_rsp_snd_full     => coherence_rsp_snd_full,\n")
          f.write("        dma_rcv_rdreq     => dma_rcv_rdreq,\n")
          f.write("        dma_rcv_data_out  => dma_rcv_data_out,\n")
          f.write("        dma_rcv_empty     => dma_rcv_empty,\n")
          f.write("        dma_snd_wrreq     => dma_snd_wrreq,\n")
          f.write("        dma_snd_data_in   => dma_snd_data_in,\n")
          f.write("        dma_snd_full      => dma_snd_full,\n")
          f.write("        interrupt_wrreq   => interrupt_wrreq,\n")
          f.write("        interrupt_data_in => interrupt_data_in,\n")
          f.write("        interrupt_full    => interrupt_full,\n")
          f.write("        apb_snd_wrreq     => apb_snd_wrreq,\n")
          f.write("        apb_snd_data_in   => apb_snd_data_in,\n")
          f.write("        apb_snd_full      => apb_snd_full,\n")
          f.write("        apb_rcv_rdreq     => apb_rcv_rdreq,\n")
          f.write("        apb_rcv_data_out  => apb_rcv_data_out,\n")
          f.write("        apb_rcv_empty     => apb_rcv_empty,\n")
          f.write("        mon_dvfs_in       => mon_dvfs_in,\n")
          f.write("        -- Monitor signals\n")
          f.write("        mon_acc           => mon_acc,\n")
          f.write("        mon_cache         => mon_cache,\n")
          f.write("        mon_dvfs          => mon_dvfs\n")
          f.write("      );\n")
          f.write("  end generate " + acc.name + "_gen;\n\n")
      else:
        f.write(tline)

#
### Main script ###
#

if len(sys.argv) != 6:
    print_usage()
    sys.exit(1)

dma_width = int(sys.argv[1])
acc_rtl_dir = sys.argv[2] + "/acc"
caches_rtl_dir = sys.argv[2] + "/sccs"
axi_acc_dir = sys.argv[3] + "/accelerators/dma" + str(dma_width)
template_dir = sys.argv[4]
out_dir = sys.argv[5]
accelerator_list = [ ]
axi_accelerator_list = [ ]
cache_list = [ ]

# Get scheduled accelerators
accelerators = next(os.walk(acc_rtl_dir))[1]
axi_accelerators = next(os.walk(axi_acc_dir))[1]

caches = [ ]
tmp_l2_dir = caches_rtl_dir + '/l2'
tmp_llc_dir = caches_rtl_dir + '/llc'
if os.path.exists(tmp_l2_dir):
  caches.append('l2')
if os.path.exists(tmp_llc_dir):
  caches.append('llc')


if (len(accelerators) == 0):
  print("    INFO: No accelerators found in " + acc_rtl_dir + ".")
  print("          Please run 'make accelerators' or make <accelerator>-hls.")
  print("          Get available accelerators with 'make print-available-accelerators'")

if (len(caches) == 0):
  print("    WARNING: No caches found in " + caches_rtl_dir + ".")
  print("             Please run 'make caches'.")

for acc in axi_accelerators:
  accd = AxiAccelerator()
  accd.name = acc

  elem = xml.etree.ElementTree.parse(axi_acc_dir + "/" + acc + "/" + acc + ".xml")
  e = elem.getroot()
  for xmlacc in e.findall('accelerator'):
    acc_name = xmlacc.get('name')
    if acc_name != acc:
      continue

    print("    INFO: Retrieving information for " + acc)
    if "desc" in xmlacc.attrib:
      accd.desc = xmlacc.get('desc')
    else:
      print("    ERROR: Missing description for " + acc)
      sys.exit(1)
    if "device_id" in xmlacc.attrib:
      accd.device_id = xmlacc.get('device_id')
    else:
      print("    ERROR: Missing device ID for " + acc)
      sys.exit(1)

    if "interrupt" in xmlacc.attrib:
      accd.interrupt = xmlacc.get('interrupt')

    if "axi_prefix" in xmlacc.attrib:
      accd.axi_prefix = xmlacc.get('axi_prefix')

    if "apb_prefix" in xmlacc.attrib:
      accd.apb_prefix = xmlacc.get('apb_prefix')

    if "addr_width" in xmlacc.attrib:
      accd.addr_width = xmlacc.get('addr_width')

    if "id_width" in xmlacc.attrib:
      accd.id_width = xmlacc.get('id_width')

    if "user_width" in xmlacc.attrib:
      accd.user_width = xmlacc.get('user_width')

    for xmlparam in xmlacc.findall('clock'):
      accd.clocks.append(xmlparam.get('name'))
    for xmlparam in xmlacc.findall('reset'):
      #TODO: get polarity from XML (assuming active low for now)
      accd.resets.append(xmlparam.get('name'))

    axi_accelerator_list.append(accd)
    print(str(accd))
    break

for acc in accelerators:
  accd = Accelerator()
  accd.name = acc

  # Get scheduled HLS configurations
  acc_dir = acc_rtl_dir + "/" + acc
  acc_dp = get_immediate_subdirectories(acc_dir)
  for dp_str in acc_dp:
    dp = dp_str.replace(acc + "_", "")
    dp_info = dp.split("_")
    skip = False
    for item in dp_info:
      if re.match(r'dma[1-9]+', item, re.M|re.I):
        dp_dma_width = int(item.replace("dma", ""))
        if dp_dma_width != dma_width:
          skip = True
          break;
    if skip:
      print("    INFO: System DMA_WIDTH is " + str(dma_width) + "; skipping " + acc + "_" + dp)
      continue
    print("    INFO: Found implementation " + dp + " for " + acc)
    impl = Implementation()
    impl.name = dp
    impl.dma_width = dma_width
    accd.hlscfg.append(impl)

  # Read accelerator parameters and info
  if len(accd.hlscfg) == 0:
    print("    WARNING: No valid HLS configuration found for " + acc)
    continue

  elem = xml.etree.ElementTree.parse(acc_dir + "/" + acc + ".xml")
  e = elem.getroot()
  for xmlacc in e.findall('accelerator'):
    acc_name = xmlacc.get('name')
    if acc_name != acc:
      continue

    print("    INFO: Retrieving information for " + acc)
    if "desc" in xmlacc.attrib:
      accd.desc = xmlacc.get('desc')
    else:
      print("    ERROR: Missing description for " + acc)
      sys.exit(1)
    if "data_size" in xmlacc.attrib:
      accd.data = int(xmlacc.get('data_size'))
    else:
      print("    ERROR: Missing memory footprint (MB) for " + acc)
      sys.exit(1)
    if "device_id" in xmlacc.attrib:
      accd.device_id = xmlacc.get('device_id')
    else:
      print("    ERROR: Missing device ID for " + acc)
      sys.exit(1)

    if "hls_tool" in xmlacc.attrib:
      accd.hls_tool = xmlacc.get('hls_tool')
      if not accd.hls_tool in ('stratus_hls', 'vivado_hls'):
        print("    ERROR: Wrong HLS tool for " + acc)
        sys.exit(1)
    else:
      print("    ERROR: Missing HLS tool for " + acc)
      sys.exit(1)

    reg = 16
    for xmlparam in xmlacc.findall('param'):
      param = Parameter()
      param.name = xmlparam.get('name')
      param.reg = reg
      reg += 1
      if "desc" in xmlparam.attrib:
        param.desc = xmlparam.get('desc')
      if "value" in xmlparam.attrib:
        param.value = int(xmlparam.get('value'))
        param.readonly = True
      if "size" in xmlparam.attrib:
        param.size = int(xmlparam.get('size'))
      else:
        param.size = 32
      if param.size > 32:
        print("    ERROR: configuration parameter " + param.name + " of " + acc + " has bit-width larger than 32")
        sys.exit(1)
      accd.param.append(param)
    accelerator_list.append(accd)
    print(str(accd))
    break


# Compute relevan bitwidths for cache interfaces
# based on DMA_WIDTH and a fixed 128-bits cache line
bits_per_line = 128
word_offset_bits = int(math.log2(128/dma_width))
byte_offset_bits = int(math.log2(dma_width/8))
offset_bits = word_offset_bits + byte_offset_bits

for cac in caches:
  cacd = Component()
  cacd.name = cac

  # Get scheduled HLS configurations
  cac_dir = caches_rtl_dir + "/" + cac
  cac_dp = get_immediate_subdirectories(cac_dir)
  for dp_str in cac_dp:
    dp = dp_str.replace(cac + "_", "")
    cacd.hlscfg.append(dp)
    print("    INFO: Found implementation " + dp + " for " + cac)
  if len(cacd.hlscfg) == 0:
    print("    WARNING: No valid HLS configuration found for " + cac)
    continue
  cache_list.append(cacd)


# Generate RTL
print("    INFO: Generating RTL to " + out_dir)
gen_device_id(accelerator_list, axi_accelerator_list, template_dir, out_dir)
gen_tech_dep(accelerator_list, cache_list, dma_width, template_dir, out_dir)
gen_tech_indep(accelerator_list, axi_accelerator_list, cache_list, dma_width, template_dir, out_dir)
gen_tech_indep_impl(accelerator_list, cache_list, dma_width, template_dir, out_dir)
gen_interfaces(accelerator_list, axi_accelerator_list, dma_width, template_dir, out_dir)
for acc in accelerator_list:
  gen_noc_interface(acc, dma_width, template_dir, out_dir, False)
for acc in axi_accelerator_list:
  gen_noc_interface(acc, dma_width, template_dir, out_dir, True)
gen_tile_acc(accelerator_list, axi_accelerator_list, template_dir, out_dir)
