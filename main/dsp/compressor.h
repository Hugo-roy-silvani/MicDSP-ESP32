#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <math.h>

typedef struct {
    float threshold;    
    float ratio;        
    float makeup;       
    float attack_coeff; 
    float release_coeff;
    float gain;       
    float knee_db;
} compressor_t;

void compressor_init(compressor_t *c, float fs, float threshold, float ratio,
                     float makeup_db, float attack_ms, float release_ms, float knee);


float compressor_process(compressor_t *c, float x, float level);

#endif // COMPRESSOR_H
