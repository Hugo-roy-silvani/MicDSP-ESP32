#include "expander.h"
#include "config.h"

void expander_init(expander_t *e, float fs, float threshold, float ratio,
                   float attack_ms, float release_ms, float hold_ms)
{
    if (!e) return;
    e->threshold = threshold;
    e->ratio = (ratio < 1.0f) ? 1.0f : ratio;
    e->attack_coeff  = expf(-1.0f / (fs * (attack_ms * 0.001f)));
    e->release_coeff = expf(-1.0f / (fs * (release_ms * 0.001f)));
    e->hold_time = hold_ms / 1000.0f;
    e->hold_counter = 0.0f;
    e->gain = 1.0f;
}

float expander_process(expander_t *e, float x, float level)
{
    float target_gain = 1.0f;

    // target gain
    if (level < e->threshold) {
       
        float under = e->threshold / fmaxf(level, 1e-9f);
        target_gain = powf(under, (1.0f / e->ratio) - 1.0f); 
        e->hold_counter = 0.0f; 
    } else {
        
        if (e->hold_counter < e->hold_time)
            e->hold_counter += 1.0f / I2S_SR; 
        else
            target_gain = 1.0f; 
    }

    // Attack / Release smoothing
    if (target_gain < e->gain)
        e->gain = e->attack_coeff  * (e->gain - target_gain) + target_gain;
    else
        e->gain = e->release_coeff * (e->gain - target_gain) + target_gain;

   
    float y = x * e->gain;

    return y;
}
