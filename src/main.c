//
//  main.c
//  XC_s2fastqIO
//
//  Created by Mikel Hernaez on 11/4/14.
//  Copyright (c) 2014 Mikel Hernaez. All rights reserved.
//

#include <stdio.h>
#include <time.h>
#include "sam_block.h"
#include "Arithmetic_stream.h"
#include "read_compression.h"

#include <pthread.h>

/**
 * Displays a usage name
 * @param name Program name string
 */
void usage(const char *name) {
    printf("Usage: %s (options) [input file] [output folder] [ref file]\n", name);
    printf("Options are:\n");
    //printf("\t-q\t\t\t: Store quality values in compressed file (default)\n");
    printf("\t-x\t\t: Regenerate FASTQ file from compressed file\n");
    printf("\t-c [ratio]\t: Compress using [ratio] bits per bit of input entropy per symbol\n");
    //printf("\t-r [rate]\t: Compress using fixed [rate] bits per symbol\n");
    printf("\t-d [M|L|A]\t: Optimize for MSE, Log(1+L1), L1 distortions, respectively (default: MSE)\n");
    //printf("\t-c [#]\t\t: Compress using [#] clusters (default: 3)\n");
    //printf("\t-u [FILE]\t: Write the uncompressed lossy values to FILE (default: off)\n");
    printf("\t-h\t\t: Print this help\n");
    printf("\t-s\t\t: Print summary stats\n");
    //printf("\t-t [lines]\t: Number of lines to use as training set (0 for all, 1000000 default)\n");
    printf("\t-v\t\t: Enable verbose output\n");
}



int main(int argc, const char * argv[]) {
    
    uint32_t extract, i = 0, file_idx = 0, rc = 0;
    
    struct qv_options_t opts;
    
    char input_name[1024], output_name[1024], ref_name[1024];
    
    char* ptr;
    
    struct remote_file_info remote_info;
    
    struct compressor_info_t comp_info;
    
    pthread_t compressor_thread;
    //pthread_t network_thread;
    
    time_t begin_main;
    time_t end_main;
    
    file_available = 0;
    
    opts.training_size = 1000000;
    opts.verbose = 0;
    opts.stats = 0;
    opts.ratio = 1;
    opts.uncompressed = 0;
    opts.distortion = DISTORTION_MSE;
    
    extract = COMPRESSION;
    
    // No dependency, cross-platform command line parsing means no getopt
    // So we need to settle for less than optimal flexibility (no combining short opts, maybe that will be added later)
    i = 1;
    while (i < argc) {
        // Handle file names and reject any other untagged arguments
        if (argv[i][0] != '-') {
            switch (file_idx) {
                case 0:
                    strcpy(input_name,argv[i]);
                    file_idx = 1;
                    break;
                case 1:
                    strcpy(output_name,argv[i]);
                    file_idx = 2;
                    break;
                case 2:
                    strcpy(ref_name, argv[i]);
                    file_idx = 3;
                    break;
                default:
                    printf("Garbage argument \"%s\" detected.\n", argv[i]);
                    usage(argv[0]);
                    exit(1);
            }
            i += 1;
            continue;
        }
        
        // Flags for options
        switch(argv[i][1]) {
            case 'x':
                extract = DECOMPRESSION;
                i += 1;
                break;
            //case 'q':
                //extract = COMPRESSION;
                //i += 1;
                //break;
            case 'c':
                extract = COMPRESSION;
                opts.ratio = atof(argv[i+1]);
                opts.mode = MODE_RATIO;
                i += 2;
                break;
            case 'v':
                opts.verbose = 1;
                i += 1;
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
            case 's':
                opts.stats = 1;
                i += 1;
                break;
            //case 't':
                //opts.training_size = atoi(argv[i+1]);
                //i += 2;
                //break;
            case 'd':
                switch (argv[i+1][0]) {
                    case 'M':
                        opts.distortion = DISTORTION_MSE;
                        break;
                    case 'L':
                        opts.distortion = DISTORTION_LORENTZ;
                        break;
                    case 'A':
                        opts.distortion = DISTORTION_MANHATTAN;
                        break;
                    default:
                        printf("Distortion measure not supported, using MSE.\n");
                        break;
                }
                i += 2;
                break;
            default:
                printf("Unrecognized option -%c.\n", argv[i][1]);
                usage(argv[0]);
                exit(1);
        }
    }
    
    if (extract == DECOMPRESSION && file_idx != 3) {
        printf("Missing required filenames in extract mode.\n");
        usage(argv[0]);
        exit(1);
    }
    
    if (extract == COMPRESSION && file_idx != 2) {
        printf("Wrong required filenames in compress mode.\n");
        usage(argv[0]);
        exit(1);
    }
    
    if (opts.verbose) {
        if (extract) {
            printf("%s will be decoded to %s.\n", input_name, output_name);
        }
        else {
            printf("%s will be encoded as %s.\n", input_name, output_name);
            if (opts.mode == MODE_RATIO)
                printf("Ratio mode selected, targeting %f compression ratio\n", opts.ratio);
            else if (opts.mode == MODE_FIXED)
                printf("Fixed-rate mode selected, targeting %f bits per symbol\n", opts.ratio);
            else if (opts.mode == MODE_FIXED_MSE)
                printf("Fixed-MSE mode selected, targeting %f average MSE per context\n", opts.ratio);
            // @todo other modes?
        }
    }
    
    if (extract == COMPRESSION) {
        ptr = strtok((char*)output_name, "@");
        strcpy(remote_info.username, ptr);
        ptr = strtok(NULL, ":");
        strcpy(remote_info.host_name, ptr);
        ptr = strtok(NULL, "\0");
        strcpy(remote_info.filename, ptr);
    }
    
    if (extract == DECOMPRESSION) {
        ptr = strtok((char*)input_name, "@");
        strcpy(remote_info.username, ptr);
        ptr = strtok(NULL, ":");
        strcpy(remote_info.host_name, ptr);
        ptr = strtok(NULL, "\0");
        strcpy(remote_info.filename, ptr);
    }
    //FILE * fcomp = (extract == COMPRESSION)? fopen(output_name, "w"):  fopen(input_name, "r");
    
    
    comp_info.fsam = (extract == COMPRESSION)? fopen( input_name, "r"): fopen(output_name, "w");
    
    // Open the Ref file
    if (extract == DECOMPRESSION) {
        if (( comp_info.fref = fopen ( ref_name , "r" ) ) == NULL){
            fputs ("Chromosome (ref) File error\n",stderr); exit (1);
        }
    }
    
    comp_info.qv_opts = &opts;
    
    
    file_available = 0;
    
    time(&begin_main);
    
    if (extract == COMPRESSION){
        rc = pthread_create(&compressor_thread, NULL, compress , (void *)&comp_info);
        //file_available = 31;
        //rc = pthread_create(&network_thread, NULL, upload , (void *)&remote_info);
//        upload((void *)&remote_info);
    }
    else{
        //rc = pthread_create(&compressor_thread, NULL, decompress , (void *)&comp_info);
//        rc = pthread_create(&compressor_thread, NULL, download , (void *)&remote_info);
        decompress((void *)&comp_info);
    }
    
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    
    time(&end_main);
    
    
#ifdef _WIN32
    system("pause");
#endif
    
    printf("Total time elapsed: %ld seconds.\n",(end_main - begin_main));
    
    pthread_exit(NULL);
    
    return 1;
    
    
    
    
}

