#!/bin/python

import os
import sys
import numpy as np


fd_data = open("data.c", "w")
fd_gold = open("gold.c", "w")

in_array = np.loadtxt('tb_input_features.dat')
out_array = np.loadtxt('tb_output_predictions.dat')

(in_nimages, in_size) = in_array.shape
(out_nimages, out_size) = out_array.shape

assert (in_nimages == out_nimages)

for n in range(0, in_nimages):
    for i in range(0, in_size):
        fd_data.write("mem[" + str(n * in_size + i) + "] = " + str(in_array[n][i]) + ";\n")

    for i in range(0, out_size):
        fd_gold.write("mem[" + str(n * out_size + i) + "] = " + str(out_array[n][i]) + ";\n")

fd_data.close()
fd_gold.close()


