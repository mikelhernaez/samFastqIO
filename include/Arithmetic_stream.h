//
//  Arithmetic_stream.h
//  XC_s2fastqIO
//
//  Created by Mikel Hernaez on 11/4/14.
//  Copyright (c) 2014 Mikel Hernaez. All rights reserved.
//

#ifndef XC_s2fastqIO_Arithmetic_stream_h
#define XC_s2fastqIO_Arithmetic_stream_h

#define IO_STREAM_BUF_LEN 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct stream_stats_t {
    uint32_t *counts;
    uint32_t alphabetCard;
    uint32_t step;
    uint32_t n;
    uint32_t rescale;
} *stream_stats;

typedef struct ppm0_stream_stats_t{
    
    int32_t *alphabet;
    uint32_t *counts;
    int32_t *alphaMap;
    uint8_t *alphaExist;
    uint32_t alphabetCard;
    uint32_t step;
    uint32_t n;
    uint32_t rescale;
    
}*ppm0_stream_stats;

typedef struct io_stream_t {

    FILE *fp;
    
    uint8_t *buf;
    uint32_t bufPos;
    uint8_t bitPos;
    uint64_t written;
    
} *io_stream;


typedef struct Arithmetic_stream_t {
    int32_t scale3;
    
    uint32_t l;     // lower limit
    uint32_t u;     // upper limit
    uint32_t t;     // the tag
    
    uint32_t m;     // size of the arithmetic word
    uint32_t r;     // Rescaling condition
    
    io_stream ios;  // input/output stream of encoded bits
}*Arithmetic_stream;


// Function Prototypes
struct io_stream_t *alloc_io_stream(FILE *fp, uint8_t in);
void free_os_stream(struct io_stream_t *os);
uint8_t stream_read_bit(struct io_stream_t *is);
uint32_t stream_read_bits(struct io_stream_t *is, uint8_t len);
void stream_write_bit(struct io_stream_t *os, uint8_t bit);
void stream_finish_byte(struct io_stream_t *os);
void stream_write_buffer(struct io_stream_t *os);

Arithmetic_stream alloc_arithmetic_stream(uint32_t m, uint8_t direction);
void arithmetic_encoder_step(Arithmetic_stream a, uint32_t cumCountX_1, uint32_t cumCountX, uint32_t n);
uint64_t encoder_last_step(Arithmetic_stream a);
void arithmetic_decoder_step(Arithmetic_stream a, uint32_t cumCountX,  uint32_t cumCountX_1, uint32_t n);
uint64_t arithmetic_get_symbole_range(Arithmetic_stream a, uint32_t n);
#endif