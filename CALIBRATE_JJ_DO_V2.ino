//Callibration code for the Gravity DO Probe
//Make sure the serial moniter is set to 115200 otherwise you will get random string 

#include <Arduino.h>

#define VREF    5000//VREF(mv)
#define ADC_RES 1024//ADC Resolution

uint32_t raw;

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    raw=analogRead(A1);
    Serial.println("raw:\t"+String(raw)+"\tVoltage(mv)"+String(raw*VREF/ADC_RES));
    delay(1000);
}
