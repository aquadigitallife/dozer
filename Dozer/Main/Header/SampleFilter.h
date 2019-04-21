#ifndef SAMPLEFILTER_H_
#define SAMPLEFILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 10 Hz

* 0 Hz - 1 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 3.521449801870057 dB

* 1.6 Hz - 5 Hz
  gain = 0
  desired attenuation = -70 dB
  actual attenuation = -71.43344815934566 dB

*/

#define SAMPLEFILTER_TAP_NUM 33

typedef struct {
  double history[SAMPLEFILTER_TAP_NUM];
  unsigned int last_index;
} SampleFilter;

#ifdef __cplusplus
 extern "C" {
#endif

void SampleFilter_init(SampleFilter* f);
void SampleFilter_put(SampleFilter* f, double input);
double SampleFilter_get(SampleFilter* f);

#ifdef __cplusplus
}
#endif

#endif
