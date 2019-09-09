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
#include "weights/w4.h"
#include "weights/b4.h"

void block_layer2(layer3_t layer3_out[N_LAYER_2], layer5_t layer5_out[N_LAYER_4]) {
    // #pragma HLS ARRAY_PARTITION variable=layer3_out complete dim=0
    // #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0

#ifndef __SYNTHESIS__
    printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config4::n_in * config4::n_out >(w4, "w4.txt");
    load_txt_file< ap_fixed<18,8>, config4::n_out >(b4, "b4.txt");
#endif

    layer4_t layer4_out[N_LAYER_4];
    #pragma HLS ARRAY_PARTITION variable=layer4_out complete dim=0

    nnet::dense_large<layer3_t, layer4_t, config4>(layer3_out, layer4_out, w4, b4);
    nnet::relu<layer4_t, layer5_t, relu_config5>(layer4_out, layer5_out);
}
