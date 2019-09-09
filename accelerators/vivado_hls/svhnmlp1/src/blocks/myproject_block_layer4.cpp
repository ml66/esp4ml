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
#include <iostream>

#include "myproject.h"
#include "myproject_block_layers.h"

//hls-fpga-machine-learning insert weights
#include "weights/w8.h"
#include "weights/b8.h"

void block_layer4(layer7_t layer7_out[N_LAYER_6], layer9_t layer9_out[N_LAYER_8]) {
    // #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0
    // #pragma HLS ARRAY_PARTITION variable=layer9_out complete dim=0

#ifndef __SYNTHESIS__
    // printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config8::n_in * config8::n_out >(w8, "w8.txt");
    load_txt_file< ap_fixed<18,8>, config8::n_out >(b8, "b8.txt");
#endif

    layer8_t layer8_out[N_LAYER_8];
    #pragma HLS ARRAY_PARTITION variable=layer8_out complete dim=0

    nnet::dense_large<layer7_t, layer8_t, config8>(layer7_out, layer8_out, w8, b8);
    nnet::relu<layer8_t, layer9_t, relu_config9>(layer8_out, layer9_out);
}
