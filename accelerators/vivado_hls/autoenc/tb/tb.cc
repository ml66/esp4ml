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
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "parameters.h"
#include "myproject.h"
#include "nnet_helpers.h"
#include "autoenc.h"
#include "type.h"

#define CHECKPOINT 5000

int main(int argc, char **argv)
{
    int retval = 0;
    unsigned errors;

    printf("\n**** Start ****\n");

    dma_word_t in[SIZE_IN];
    dma_word_t out[SIZE_OUT];
    dma_info_t load[NCHUNK];
    dma_info_t store[NCHUNK];

    if (in == NULL || out == NULL || load == NULL || store == NULL)
    {
    	printf("null operator...FAIL");
    	exit(1);
    }

    // Initializing input and output data

    //load input data from text file
    std::ifstream fin("tb_data/tb_input_features.dat");
    //load predictions from text file
    std::ifstream fpr("tb_data/tb_output_predictions.dat");
    std::ofstream fout;
    fout.open("tb_output_data.dat");

    std::string iline;
    std::string pline;
    int e = 0;

#ifdef RTL_SIM
    std::string RESULTS_LOG = "tb_data/rtl_cosim_results.log";
#else
    std::string RESULTS_LOG = "tb_data/csim_results.log";
#endif
    std::cout << "INFO: save inference results to file: " << RESULTS_LOG << std::endl;
    std::ofstream results;
    results.open(RESULTS_LOG);

    if (fin.is_open() && fpr.is_open()) {

	std::vector<float> in_vector;
	std::vector<float> pr;

	while ( std::getline(fin,iline) && std::getline (fpr,pline) ) {
	    if (e % CHECKPOINT == 0) std::cout << "Processing input " << e << std::endl;
	    e++;
	    char* cstr=const_cast<char*>(iline.c_str());
	    char* current;

	    current=strtok(cstr," ");
	    while(current!=NULL) {
		in_vector.push_back(atof(current));
		current=strtok(NULL," ");
	    }
	    cstr=const_cast<char*>(pline.c_str());

	    current=strtok(cstr," ");
	    while(current!=NULL) {
		pr.push_back(atof(current));
		current=strtok(NULL," ");
	    }
	}

	std::cout << "TB LOAD" << std::endl;
	unsigned k = 0;
	for(unsigned i = 0; i < SIZE_IN; i++) {
	    for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		in[i].word[j] = (fxpt_word_t) in_vector[k++];
	    }
	}
	std::cout << std::endl;

	// Call the TOP function
	printf("Call the TOP function...\n");
	TOP(out, in, NINPUTS, load, store);

	fout << "Result" << std::endl;
	errors = 0;
	k = 0;

	for(int i = 0; i < SIZE_OUT; i++) {
	    for(unsigned j = 0; j < VALUES_PER_WORD; j++) {

		fxpt_word_t out_int = out[i].word[j];

		fout << out_int << " ";
		fout << pr[k] << " ";
		results << out_int << " ";
		std::cout << out_int << " ";

		if (abs(pr[k] - (float) out_int)/pr[k] > 0.05)
		    errors++;

		k++;
	    }
	}

	fout << std::endl;
	results << std::endl;
	std::cout << std::endl;

	if (errors)
	    std::cout << "Validation FAILED! " << errors << " errors" << std::endl;
	else
	    std::cout << "Validation PASSED! " << errors << " errors" << std::endl;

    }

    results.close();
    fin.close();
    fpr.close();
    fout.close();

    return retval;
}
