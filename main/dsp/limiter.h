#ifndef LIMITER_H
#define LIMITER_H

#include <math.h>

typedef struct {
    float threshold;   
    float attack;      // second
    float release;     // second
    float fs;          
    float gain;        
    float att_coeff;   
    float rel_coeff;   
} limiter_t;

void limiter_init(limiter_t *l, float fs, float threshold, float attack_ms, float release_ms);

float limiter_process(limiter_t *l, float x, float level);

#endif // LIMITER_H
