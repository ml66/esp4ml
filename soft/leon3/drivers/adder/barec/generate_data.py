import os
import sys
import numpy as np

array_size = 128

input_real = np.random.randint(low=1, high=100000, size=array_size)
input_imag = np.random.randint(low=1, high=100000, size=array_size)

fd_in = open("data.c", "w")
fd_gold = open("data_gold.c", "w")

for i in range(0, array_size):
    a = input_real[i]
    fd_in.write("mem[" + str(i*2) + "] = " + str(i*2) + ";\n")
    fd_in.write("mem[" + str(i*2+1) + "] = " + str(i*2+1) + ";\n")
    # fd_in.write("mem[" + str(i*2) + "] = " + str(input_real[i]) + ";\n")
    # fd_in.write("mem[" + str(i*2+1) + "] = " + str(input_imag[i]) + ";\n")
    output_gold = i*2 + i*2 + 1
    # output_gold = input_real[i] + input_imag[i]
    fd_gold.write("mem_gold[" + str(i) + "] = " + str(output_gold) + ";\n")

fd_in.close()
fd_gold.close()

