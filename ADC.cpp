#include "ADC.h"

ADC::ADC()
{
    adc = new ADS7830();
}

/* Probably dont need this function, will see later
int ADC:read_stable_byte()
{
    while (true)
    {
        int value1 = 
    }
}
*/

float ADC::read_adc(static int channel const)
{
    int adcValue = adc->analogRead(channel);
    float voltage = (float)adcValue / 255.0 * 3.3; // may need to change voltage reference
    return voltage;
}