#include "iir_filter.h"
#include "esp_log.h"
#include <math.h>
#include "config.h"

static const char *TAG_FILT = "FILTER";

static eq3band_t eq;

const char* filter_type_to_str(filter_type_t type)
{
    switch (type) {
        case FILTER_LOW_PASS:  return "Low-Pass";
        case FILTER_HIGH_PASS: return "High-Pass";
        case FILTER_BAND_PASS: return "Band-Pass";
        default:               return "Unknown";
    }
}

bool is_biquad_stable(float a1, float a2)
{
    
    if (fabsf(a2) >= 1.0f) return false;
    if (a1 <= -1.0f - a2) return false;
    if (a1 >= 1.0f - a2) return false;
    return true;
}

void update_filter_coefficients_eq(eq_band_t *band)
{
    float fs = (float)I2S_SR;
    if (band->fc < 20.0f)         band->fc = 20.0f;
    if (band->fc > 0.45f * fs)    band->fc = 0.45f * fs;
    if (band->Q  < 0.3f)          band->Q  = 0.3f;
    if (band->Q  > 10.0f)         band->Q  = 10.0f;
    if (band->gain_db > 8.0f) band->gain_db = 8.0f;
    if (band->gain_db < -8.0f) band->gain_db = -8.0f;

    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    const float w0    = 2.0f * (float)M_PI * band->fc / fs;
    const float cosw0 = cosf(w0);
    const float sinw0 = sinf(w0);
    const float alpha = sinw0 / (2.0f * band->Q);
    const float A     = powf(10.0f, band->gain_db / 40.0f);  // linéaire
    
    switch (band->type) {
        case FILTER_LOW_PASS:
            b0 = (1.0f - cosw0) * 0.5f;
            b1 = 1.0f - cosw0;
            b2 = (1.0f - cosw0) * 0.5f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosw0;
            a2 = 1.0f - alpha;
            break;

        case FILTER_HIGH_PASS:
            b0 = (1.0f + cosw0) * 0.5f;
            b1 = -(1.0f + cosw0);
            b2 = (1.0f + cosw0) * 0.5f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosw0;
            a2 = 1.0f - alpha;
            break;

        case FILTER_BAND_PASS:
            b0 = alpha;
            b1 = 0.0f;
            b2 = -alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosw0;
            a2 = 1.0f - alpha;
            break;

        case FILTER_PEAKING:
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cosw0;
            b2 = 1.0f - alpha * A;
            a0 = 1.0f + alpha / A;
            a1 = -2.0f * cosw0;
            a2 = 1.0f - alpha / A;
            break;

        case FILTER_LOW_SHELF:
        {
            float sqrtA = sqrtf(A);
            b0 =    A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
            b1 =  2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
            b2 =    A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
            a0 =        (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            a1 =   -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
            a2 =        (A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
            break;
        }

        case FILTER_HIGH_SHELF:
        {
            float sqrtA = sqrtf(A);
            b0 =    A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
            b2 =    A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
            a0 =        (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            a1 =    2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
            a2 =        (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
            break;
        }
        default:
            return;
    }

    const float eps = 1e-12f;
    if (fabsf(a0) < eps) a0 = (a0 >= 0.0f) ? eps : -eps;

    b0 /= a0;  b1 /= a0;  b2 /= a0;
    a1 /= a0;  a2 /= a0;

    if (!is_biquad_stable(a1, a2)) {
        ESP_LOGW(TAG, "Unstable filter rejected! fc=%.1fHz Q=%.2f a1=%.3f a2=%.3f",band->fc, band->Q, a1, a2);
        return;
    }

    band->b0 = b0;  
    band->b1 = b1;  
    band->b2 = b2;
    band->a1 = a1;  
    band->a2 = a2;

    band->w1 = 0.0f; 
    band->w2 = 0.0f;
}

static inline const eq_band_t* band_ptr(eq_band_id_t id)
{
    switch (id) {
        case EQ_BAND_LOW:  return &eq.low;
        case EQ_BAND_MID:  return &eq.mid;
        case EQ_BAND_HIGH: return &eq.high;
        default:           return &eq.low; 
    }
}

const eq_band_t* eq_get_band(eq_band_id_t id)
{
    return band_ptr(id);
}

void eq_get_all_bands(const eq_band_t **low,
                      const eq_band_t **mid,
                      const eq_band_t **high)
{
    if (low)  *low  = &eq.low;
    if (mid)  *mid  = &eq.mid;
    if (high) *high = &eq.high;
}

void print_filter_status(void)
{
    ESP_LOGI(TAG_FILT,
        "Current filter:\n"
        "  b0=%.7f\n"
        "  b1=%.7f\n"
        "  b2=%.7f\n"
        "  a1=%.7f\n"
        "  a2=%.7f\n"
        "  filter type : %s\n"
        "  Q = %.3f\n",
        filt.b0, filt.b1, filt.b2, filt.a1, filt.a2, filter_type_to_str(filt.type), filt.q);
}

void eq_init(void)
{
    eq.low.type = FILTER_LOW_SHELF;
    eq.low.fc = 100.0f;
    eq.low.Q = 0.707f;
    eq.low.gain_db = 0.0f;

    eq.mid.type = FILTER_PEAKING;
    eq.mid.fc = 1200.0f;
    eq.mid.Q = 1.0f;
    eq.mid.gain_db = 0.0f;

    eq.high.type = FILTER_HIGH_SHELF;
    eq.high.fc = 8000.0f;
    eq.high.Q = 0.707f;
    eq.high.gain_db = 0.0f;

    update_filter_coefficients_eq(&eq.low);
    update_filter_coefficients_eq(&eq.mid);
    update_filter_coefficients_eq(&eq.high);
}

float biquad_df2t_process_eq(eq_band_t *b, float x)
{
    float y = b->b0 * x + b->w1;
    float w1_next = b->b1 * x + b->w2 - b->a1 * y;
    float w2_next = b->b2 * x - b->a2 * y;

    b->w1 = w1_next;
    b->w2 = w2_next;

    return y;
}

float eq3band_process(float x)
{
    float y  = biquad_df2t_process_eq(&eq.low, x);
    y = biquad_df2t_process_eq(&eq.mid,  y);   
    y = biquad_df2t_process_eq(&eq.high, y);

    return y;
}
