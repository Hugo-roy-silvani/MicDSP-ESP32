#pragma once
#include <stdint.h>
#include <math.h>
#include <stdio.h>

typedef enum {
    FILTER_LOW_PASS = 0,
    FILTER_HIGH_PASS,
    FILTER_BAND_PASS,
    FILTER_PEAKING,
    FILTER_LOW_SHELF,
    FILTER_HIGH_SHELF
} filter_type_t;

typedef struct {
    float b0, b1, b2, a1, a2;
    float w1, w2;
    float fc;
    float Q;
    float gain_db;
    filter_type_t type;
} eq_band_t;

typedef struct {
    eq_band_t low;
    eq_band_t mid;
    eq_band_t high;
} eq3band_t;

typedef enum { EQ_BAND_LOW = 0, EQ_BAND_MID, EQ_BAND_HIGH } eq_band_id_t;

const char* filter_type_to_str(filter_type_t type);
void print_filter_status(void);
float biquad_df2t_process_eq(eq_band_t *b, float x);
void update_filter_coefficients_eq(eq_band_t *band);
void eq_init(void);
float eq3band_process(float x);
const eq_band_t* eq_get_band(eq_band_id_t id);
void eq_get_all_bands(const eq_band_t **low, const eq_band_t **mid, const eq_band_t **high);

