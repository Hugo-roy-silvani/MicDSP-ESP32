#include "rms.h"

void rms_init(rms_filter_t *r, float fs, float tau_ms) {
    if (!r) return;
    if (tau_ms <= 0.0f) tau_ms = 50.0f;
    float tau = tau_ms * 0.001f;
    float a = 1.0f - expf(-1.0f / (fs * tau));
    if (a < 1e-6f)   a = 1e-6f;
    if (a > 0.999f)  a = 0.999f;
    r->alpha = a;
    r->rms_sq = 0.0f;
}
