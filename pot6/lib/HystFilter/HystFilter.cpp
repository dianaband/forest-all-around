
#include "HystFilter.h"


HystFilter::HystFilter( uint16_t inputValues, uint16_t outputValues, uint16_t margin  ) :
  _inputValues( inputValues ),
  _outputValues(  outputValues ),
  _margin(  margin ),
  _currentOutputLevel( 0  )
{
}

uint16_t HystFilter::getOutputLevel( uint16_t inputLevel ) {

  // get lower and upper bounds for currentOutputLevel
  uint16_t lb = (float) ( (float) _inputValues / _outputValues ) * _currentOutputLevel;

  if ( _currentOutputLevel > 0 )
    lb -= _margin; // subtract margin

  uint16_t ub = ( ( (float) ( (float) _inputValues / _outputValues ) * ( _currentOutputLevel + 1 ) ) - 1 );

  if ( _currentOutputLevel < _outputValues )
    ub +=  _margin;  // add margin

  // now test if input is outside the outer margins for current output value
  // If so, caclulate new output level.
  if ( inputLevel < lb || inputLevel > ub ) {

    // determine new output level
    _currentOutputLevel =   (  ( (float) inputLevel * (float) _outputValues ) /  _inputValues );
  }
  return _currentOutputLevel;
}
