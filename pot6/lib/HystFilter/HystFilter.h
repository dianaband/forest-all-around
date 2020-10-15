
/*
 * Class: HystFilter [Hysteresis filter].
 * Apply hysteresis to an input value and deliver a lower resolution, stabilised output value.
 *
 */


#ifndef HystFilter_h
#define HystFilter_h

#include <Arduino.h>


class HystFilter
{
  public:

    HystFilter( uint16_t inputValues, uint16_t outputValues, uint16_t margin  ) ;

    // constructor

    // inputValues:  the total number of discrete values delivered by the input.
    //               For example, a 10 bit ADC delivers 1024 values.
    //               8 bit ADC = 256, 9 bit ADC = 512, 10 bit ADC = 1024, 11 bit ADC = 2048, 12 bit ADC = 4096 etc.

    // outputValues: the number of discrete output values delivered. This governs the resolution of the function.
    //               For example a 6 bit resolution yields 64 values. This should ideally be no higher that the input resolution
    //               minus 3 bits. For example if the input resolution is 10 bits (1024), this should not exceed 7 bits (128)

    // margin:       margin sets the 'stickyness' of the hysteresis or the reluctance to leave the current state.
    //               It is measured in units of the the input level. As a general rule, this should be about 10% to 25% of the
    //               range of input values that map to 1 output value. For example, if the inputValues is 1024 and the outputValues
    //               is 128, then 8 input values map to 1 output value so the margin should be 2 (25% of 8 ).
    //               Probably a value of 2 is OK. For low resolutions or noisy environments, it can be higher. Don't make it too high
    //               or ranges overlap and some output values become unreachable.


    uint16_t getOutputLevel( uint16_t inputLevel )  ;

    // converts an input level (eg in the range 0 to 1023 to an aoutput value of 1 to 127 with hyteresis.

  private:
    uint16_t _inputValues ;
    uint16_t _outputValues ;
    uint16_t _margin ;
    uint16_t _currentOutputLevel ;
} ;

#endif
