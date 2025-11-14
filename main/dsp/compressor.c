#include "compressor.h"

void compressor_init(compressor_t *c, float fs, float threshold, float ratio,
                     float makeup_db, float attack_ms, float release_ms, float knee)
{
    if (!c) return;
    c->threshold = threshold; 
    c->ratio = (ratio < 1.0f) ? 1.0f : ratio;
    c->makeup = powf(10.0f, makeup_db / 20.0f);
    float tau_a = attack_ms / 1000.0f;
    float tau_r = release_ms / 1000.0f;
    c->attack_coeff  = expf(-1.0f / (fs * tau_a));
    c->release_coeff = expf(-1.0f / (fs * tau_r));
    c->gain = 1.0f;
    c->knee_db = knee;
}

float compressor_process(compressor_t *c, float x, float level)
{
    
    float level_db = 20.0f * log10f(fmaxf(level, 1e-9f));
    float thr_db   = 20.0f * log10f(fmaxf(c->threshold, 1e-9f));
    float knee     = c->knee_db;

    float target_gain_db = 0.0f;

    // Zone 1 : under the knee → no compression
    if (level_db <= thr_db - (knee * 0.5f)) {
        target_gain_db = 0.0f;
    }

    // Zone 2 : above the knee → standard compression 
    else if (level_db >= thr_db + (knee * 0.5f)) {
        float over_db = level_db - thr_db;
        target_gain_db = (1.0f - 1.0f / c->ratio) * (-over_db);
    }

    // Zone 3 : inside the knee → soft compression 
    else {
        float delta = level_db - (thr_db - knee * 0.5f);
        float soft = delta * delta / (2.0f * knee); 
        target_gain_db = -(1.0f - 1.0f / c->ratio) * soft;
    }

    // linear gain
    float target_gain = powf(10.0f, target_gain_db / 20.0f);

    // Attack / Release smoothing
    if (target_gain < c->gain)
        c->gain = c->attack_coeff  * (c->gain - target_gain) + target_gain;
    else
        c->gain = c->release_coeff * (c->gain - target_gain) + target_gain;

    // Apply gain and make-up
    float y = x * c->gain * c->makeup;

    // Hard clip 
    if (y > 1.0f) y = 1.0f;
    if (y < -1.0f) y = -1.0f;

    return y;
}
