//
//    rfnoc-hls-neuralnet: Vivado HLS code for neural-net building blocks
//
//    Copyright (C) 2017 EJ Kreinar
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef MYPROJECT_BLOCK_LAYERS_
#define MYPROJECT_BLOCK_LAYERS_

#include <iostream>

#include "myproject.h"

void block_layer1(input_t input1[N_INPUT_1_1], layer3_t layer3_out[N_LAYER_2]);
void block_layer2(layer3_t layer3_out[N_LAYER_2], layer5_t layer5_out[N_LAYER_4]);
void block_layer3(layer5_t layer5_out[N_LAYER_4], layer7_t layer7_out[N_LAYER_6]);
void block_layer4(layer7_t layer7_out[N_LAYER_6], layer9_t layer9_out[N_LAYER_8]);
void block_layer5(layer9_t layer9_out[N_LAYER_8], result_t layer11_out[N_LAYER_10]);

#endif
