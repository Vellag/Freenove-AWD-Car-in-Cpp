#include <wiringPi.h>
#include <ADCDevice.hpp>

class ADC
{
private:
    ADCDevice *adc;
public:
    ADC();
    float read_adc(static int channel const);
};