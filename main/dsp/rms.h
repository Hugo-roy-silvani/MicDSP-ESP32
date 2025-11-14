#ifndef RMS_H
#define RMS_H
#include <math.h>

typedef struct {
    float rms_sq;
    float alpha;
} rms_filter_t;

void  rms_init(rms_filter_t *r, float fs, float tau_ms);

static inline float rms_process(rms_filter_t *r, float x) {
    r->rms_sq = (1.0f - r->alpha) * r->rms_sq + r->alpha * (x * x);
    return sqrtf(r->rms_sq);
}

static inline float rms_get_dbfs(const rms_filter_t *r) {
    float rms = (r->rms_sq > 0.0f) ? sqrtf(r->rms_sq) : 0.0f;
    if (rms < 1e-12f) rms = 1e-12f;
    return 20.0f * log10f(rms);
}
#endif
