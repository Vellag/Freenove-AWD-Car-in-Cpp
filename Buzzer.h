#include <wiringPi.h>

class Buzzer
{
    private:
        static int buzzer_pin const = 17; // GPIO pin for the buzzer
    public:
        Buzzer();
        void set_state(bool state);
        void close();
};
