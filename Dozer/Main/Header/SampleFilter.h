#ifndef SAMPLEFILTER_H_
#define SAMPLEFILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 2 Hz

* 0 Hz - 0.02 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 4.125805256439371 dB

* 0.06 Hz - 1 Hz
  gain = 0
  desired attenuation = -70 dB
  actual attenuation = -70.10105615620623 dB

*/

#define SAMPLEFILTER_TAP_NUM 113


#ifdef __cplusplus
 extern "C" {
#endif

typedef struct {
  double history[SAMPLEFILTER_TAP_NUM];
  unsigned int last_index;
} SampleFilter;

void SampleFilter_init(SampleFilter* f);
void SampleFilter_put(SampleFilter* f, double input);
double SampleFilter_get(SampleFilter* f);

#ifdef __cplusplus
}
#endif

#endif