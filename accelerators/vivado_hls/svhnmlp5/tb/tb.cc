#include "svhnmlp5.h"
#include "type.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "nnet_helpers.h"
#include "parameters.h"
#include "myproject.h"
#include "myproject_block_layers.h"

#define CHECKPOINT 5000

int main(int argc, char **argv)
{
    printf("\n**** Start ****\n");

    input_t in[NINPUTS][SIZE_IN_CHUNK_TB];
    result_t out[NINPUTS][SIZE_OUT_CHUNK_TB];
    dma_word_t in_block[SIZE_IN];
    dma_word_t out_block[SIZE_OUT];
    dma_info_t *load  = (dma_info_t*) malloc(NCHUNK * sizeof(dma_info_t));
    dma_info_t *store = (dma_info_t*) malloc(NCHUNK * sizeof(dma_info_t));

    if (load == NULL || store == NULL)
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

	for(unsigned i = 0; i < NINPUTS; i++) {
	    for(unsigned j = 0; j < SIZE_IN_CHUNK_TB; j++) {
		in[i][j] = (word_t) in_vector[i * SIZE_IN_CHUNK_TB + j];
	    }
	}

	// ****************************************
	// NETWORK INSTANTIATION
	// ****************************************

	//hls-fpga-machine-learning insert layers
	layer3_t layer3_out[NINPUTS][N_LAYER_2];
	layer5_t layer5_out[NINPUTS][N_LAYER_4];
	layer7_t layer7_out[NINPUTS][N_LAYER_6];
	layer9_t layer9_out[NINPUTS][N_LAYER_8];

#if (BLOCK == 1)

	unsigned k = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    in_block[l * SIZE_IN_CHUNK + i].word[j] =
			(word_t) in[l][i * VALUES_PER_WORD + j];
		}
	    }
	}

	TOP(out_block, in_block, NINPUTS, load, store);

	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    layer3_out[l][i * VALUES_PER_WORD + j] = out_block[l * SIZE_OUT_CHUNK + i].word[j];
		}
	    }
	}

#else
	for (int l = 0; l < NINPUTS; l++)
	    block_layer1(in[l], layer3_out[l]);
#endif

#if (BLOCK == 2)

	unsigned k = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    in_block[l * SIZE_IN_CHUNK + i].word[j] =
			(word_t) layer3_out[l][i * VALUES_PER_WORD + j];
		}
	    }
	}

	TOP(out_block, in_block, NINPUTS, load, store);

	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    layer5_out[l][i * VALUES_PER_WORD + j] = out_block[l * SIZE_OUT_CHUNK + i].word[j];
		}
	    }
	}

#else
	for (int l = 0; l < NINPUTS; l++)
	    block_layer2(layer3_out[l], layer5_out[l]);
#endif    
#if (BLOCK == 3)
	unsigned k = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    in_block[l * SIZE_IN_CHUNK + i].word[j] =
			(word_t) layer5_out[l][i * VALUES_PER_WORD + j];
		}
	    }
	}

	TOP(out_block, in_block, NINPUTS, load, store);

	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    layer7_out[l][i * VALUES_PER_WORD + j] = out_block[l * SIZE_OUT_CHUNK + i].word[j];
		}
	    }
	}

#else
	for (int l = 0; l < NINPUTS; l++)
	    block_layer3(layer5_out[l], layer7_out[l]);
#endif    
#if (BLOCK == 4)
	unsigned k = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    in_block[l * SIZE_IN_CHUNK + i].word[j] =
			(word_t) layer7_out[l][i * VALUES_PER_WORD + j];
		}
	    }
	}

	TOP(out_block, in_block, NINPUTS, load, store);

	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    layer9_out[l][i * VALUES_PER_WORD + j] = out_block[l * SIZE_OUT_CHUNK + i].word[j];
		}
	    }
	}

#else
	for (int l = 0; l < NINPUTS; l++)
	    block_layer4(layer7_out[l], layer9_out[l]);
#endif    
#if (BLOCK == 5)
	unsigned k = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
		    in_block[l * SIZE_IN_CHUNK + i].word[j] =
			(word_t) layer9_out[l][i * VALUES_PER_WORD + j];
		}
	    }
	}

	TOP(out_block, in_block, NINPUTS, load, store);

	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK; i++) {
		for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
                    if (i * VALUES_PER_WORD + j < SIZE_OUT_CHUNK_TB)
		        out[l][i * VALUES_PER_WORD + j] = out_block[l * SIZE_OUT_CHUNK + i].word[j];
		}
	    }
	}

#else
        for (int l = 0; l < NINPUTS; l++)
	    block_layer5(layer9_out[l], out[l]);
#endif    

	fout << "Result" << std::endl;
	unsigned errors = 0;
	for(int l = 0; l < NINPUTS; l++) {
	    for(int i = 0; i < SIZE_OUT_CHUNK_TB; i++) {

	        fout << out[l][i] << " ";
	        fout << pr[l * SIZE_OUT_CHUNK_TB + i] << " ";
	        results << out[l][i] << " ";
	        std::cout << out[l][i] << " ";

	        if ((pr[l * SIZE_OUT_CHUNK_TB + i] > 0.5 && out[l][i] < 0.5) ||
	           (pr[l * SIZE_OUT_CHUNK_TB + i] < 0.5 && out[l][i] > 0.5))
		       errors++;
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

    free(load);
    free(store);

    return 0;
}
