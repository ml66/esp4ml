#include "svhnmlp.h"
#include "type.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "nnet_helpers.h"
#include "parameters.h"
#include "myproject.h"

#define CHECKPOINT 5000

int main(int argc, char **argv)
{
    printf("SIZEOUTCHUNK %d\n", SIZE_OUT_CHUNK);

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

        std::ofstream fout;
        fout.open("tb_output_data.dat");

	std::vector<float> in_vector;
	std::vector<float> pr;

        while ( std::getline(fin,iline) && std::getline (fpr,pline) ) {
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

	unsigned k = 0;
	for(unsigned i = 0; i < SIZE_IN; i++) {
	    for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		in[i].word[j] = (word_t) in_vector[k++];
	    }
	}

	for(unsigned i = 0; i < SIZE_OUT; i++) {
	    for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		out[i].word[j] = 0;
	    }
	}

	// Call the TOP function
	printf("Call the TOP function...\n");
	TOP(out, in, NINPUTS, load, store);

	std::cout << "Results" << std::endl;
	fout << "Result" << std::endl;
	unsigned errors = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    k = 0;
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    if (k < SIZE_OUT_CHUNK_DATA) {
			word_t out_int =
			    out[l * SIZE_OUT_CHUNK + i].word[j];

			fout << out_int << " ";
			fout << pr[k] << " ";
			results << out_int << " ";
			std::cout << out_int << " ";

			if ((pr[k + l * SIZE_OUT_CHUNK_DATA] > 0.5 && out_int != 1) ||
			    (pr[k + l * SIZE_OUT_CHUNK_DATA] < 0.5 && out_int != 0))
			    errors++;

			k++;
		    }
		}
	    }
	    std::cout << "\n";
	}

	fout << std::endl;
	results << std::endl;
	std::cout << std::endl;

	if (errors)
	    std::cout << "Validation FAILED! " << errors << " errors" << std::endl;
	else
	    std::cout << "Validation PASSED! " << errors << " errors" << std::endl;

	results.close();
	fin.close();
	fpr.close();
	fout.close();

    } else {
	std::cout << "Unable to open input/predictions file, using default input." << std::endl;
    }

    return 0;
}
