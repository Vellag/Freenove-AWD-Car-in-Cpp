#include "Buzzer.h"

Buzzer::Buzzer()
{
    pinMode(buzzer_pin, OUTPUT);
}

void Buzzer::set_state(bool state)
{
    digitalWrite(buzzer_pin, state ? true : false);
}

void close()
{
    pinMode(buzzer_pin, PM_OFF);
}