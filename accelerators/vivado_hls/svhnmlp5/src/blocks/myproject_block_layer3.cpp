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
#include "weights/w6.h"
#include "weights/b6.h"

void block_layer3(layer5_t layer5_out[N_LAYER_4], layer7_t layer7_out[N_LAYER_6]) {
    // #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0
    // #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0

#ifndef __SYNTHESIS__
    printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config6::n_in * config6::n_out >(w6, "w6.txt");
    load_txt_file< ap_fixed<18,8>, config6::n_out >(b6, "b6.txt");
#endif

    layer6_t layer6_out[N_LAYER_6];
    #pragma HLS ARRAY_PARTITION variable=layer6_out complete dim=0

    nnet::dense_large<layer5_t, layer6_t, config6>(layer5_out, layer6_out, w6, b6);
    nnet::relu<layer6_t, layer7_t, relu_config7>(layer6_out, layer7_out);
}
