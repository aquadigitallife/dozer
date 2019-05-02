#include "SampleFilter.h"

static const double filter_taps[SAMPLEFILTER_TAP_NUM] = {
  0.0011250095965782457,
  0.0040286164068818004,
  0.009353894935487977,
  0.016314710102849447,
  0.02224381466199979,
  0.022984192398794522,
  0.01461500301973259,
  -0.003956227324613201,
  -0.028782413722235965,
  -0.050567330370688425,
  -0.05716711154617488,
  -0.03844355065688017,
  0.00845253340521779,
  0.07598456657163402,
  0.1472986084593124,
  0.20160790469726206,
  0.2218952050313174,
  0.20160790469726206,
  0.1472986084593124,
  0.07598456657163402,
  0.00845253340521779,
  -0.03844355065688017,
  -0.05716711154617488,
  -0.050567330370688425,
  -0.028782413722235965,
  -0.003956227324613201,
  0.01461500301973259,
  0.022984192398794522,
  0.02224381466199979,
  0.016314710102849447,
  0.009353894935487977,
  0.0040286164068818004,
  0.0011250095965782457
};

void SampleFilter_init(SampleFilter* f) {
  int i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

void SampleFilter_put(SampleFilter* f, double input) {
  f->history[f->last_index++] = input;
  if(f->last_index == SAMPLEFILTER_TAP_NUM)
    f->last_index = 0;
}

double SampleFilter_get(SampleFilter* f) {
  double acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : SAMPLEFILTER_TAP_NUM-1;
    acc += f->history[index] * filter_taps[i];
  };
  return acc;
}
