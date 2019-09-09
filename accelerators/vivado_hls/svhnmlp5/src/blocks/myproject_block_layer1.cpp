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
#include "weights/w2.h"
#include "weights/b2.h"

void block_layer1(input_t input1[N_INPUT_1_1], layer3_t layer3_out[N_LAYER_2]) {
    // #pragma HLS ARRAY_PARTITION variable=input1 complete dim=0
    // #pragma HLS ARRAY_PARTITION variable=layer3_out complete dim=0

#ifndef __SYNTHESIS__
    printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config2::n_in * config2::n_out >(w2, "w2.txt");
    load_txt_file< ap_fixed<18,8>, config2::n_out >(b2, "b2.txt");
#endif

    layer2_t layer2_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0

    nnet::dense_large<input_t, layer2_t, config2>(input1, layer2_out, w2, b2);
    nnet::relu<layer2_t, layer3_t, relu_config3>(layer2_out, layer3_out);
}
