#include "limiter.h"
#include <math.h>

void limiter_init(limiter_t *l, float fs, float threshold, float attack_ms, float release_ms)
{
    if (!l) return;
    if (threshold <= 0.0f || threshold > 1.0f) threshold = 0.9f;
    if (attack_ms <= 0.0f) attack_ms = 5.0f;
    if (release_ms <= 0.0f) release_ms = 100.0f;

    l->fs = fs;
    l->threshold = threshold;

    float attack_t = attack_ms / 1000.0f;
    float release_t = release_ms / 1000.0f;

    l->att_coeff = expf(-1.0f / (fs * attack_t));
    l->rel_coeff = expf(-1.0f / (fs * release_t));

    l->gain = 1.0f;
}

float limiter_process(limiter_t *l, float x, float level)
{
    if (!l) return x;

    float desired_gain = 1.0f;

    if (level > l->threshold)
        desired_gain = l->threshold / (level + 1e-9f);

    if (desired_gain < l->gain)
        l->gain = l->att_coeff * (l->gain - desired_gain) + desired_gain;  // attack
    else
        l->gain = l->rel_coeff * (l->gain - desired_gain) + desired_gain;  // release

    float y = x * l->gain;

    return y;
}
