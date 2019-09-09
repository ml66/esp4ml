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
#include "weights/w10.h"
#include "weights/b10.h"

void block_layer5(layer9_t layer9_out[N_LAYER_8], result_t layer11_out[N_LAYER_10]) {
    // #pragma HLS ARRAY_PARTITION variable=layer9_out complete dim=0
    // #pragma HLS ARRAY_PARTITION variable=layer11_out complete dim=0

#ifndef __SYNTHESIS__
    // printf("INFO: load weight and bias values from files\n");
    //hls-fpga-machine-learning insert load weights
    load_txt_file< ap_fixed<18,8>, config10::n_in * config10::n_out >(w10, "w10.txt");
    load_txt_file< ap_fixed<18,8>, config10::n_out >(b10, "b10.txt");
#endif

    layer10_t layer10_out[N_LAYER_10];
    #pragma HLS ARRAY_PARTITION variable=layer10_out complete dim=0

    nnet::dense_large<layer9_t, layer10_t, config10>(layer9_out, layer10_out, w10, b10);
    nnet::softmax<layer10_t, result_t, softmax_config11>(layer10_out, layer11_out);
}
