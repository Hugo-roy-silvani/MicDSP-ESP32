#ifndef EXPANDER_H
#define EXPANDER_H

#include <math.h>

typedef struct {
    float threshold;    
    float ratio;         
    float attack_coeff;  
    float release_coeff; 
    float hold_time;     
    float hold_counter;  
    float gain;          
} expander_t;

void fft_init(expander_t *e, float fs, float threshold, float ratio,
                   float attack_ms, float release_ms, float hold_ms);

float expander_process(expander_t *e, float x, float level);

#endif // EXPANDER_H
