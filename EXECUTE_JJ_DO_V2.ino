//Code written by Jethro Jarrett, based off 'sample code' from https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237
//Updated May 2022. Contact at jethrotjarrett@gmail.com

#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

//-----------------------------------------------Configurable bits-----------------------------------------------------
#define READ_TEMP (24) //Current water temperature ℃, Or temperature sensor function

//Single-point calibration Mode=0, Two-point calibration Mode=1
#define TWO_POINT_CALIBRATION 0

//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (1181) //mv
#define CAL1_T (24)   //℃

//Two-point calibration needs to be filled CAL2_V and CAL2_T
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1300) //mv
#define CAL2_T (15)   //℃


//Some other bits
#define WINDOW_SIZE 5  //Window size - number of datapoints the running average incorporates
#define ADDITION 0   //If measured DO is slightly off, add this value to it
#define DELAY 1000         //How often you (roughly) want a measurement to be taken (in ms)
//----------------------------------------------That's everything!------------------------------------------------------------------

#define DO_PIN A1     //Probe input pin location
#define VREF 5000     //VREF (mv)
#define ADC_RES 1024  //ADC Resolution

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
String DObigS;

float DO;
float DESIRED = 1;     //DESIRED DO for now, this will change later based on potentiometer
float DObig;
float DObigF;
float SUMfloat;
float AVERAGED = 0;
float potValue = 0;  
float DESIREDBIG = 0; 

int count = 0;
int INDEX = 0;
int VALUE = 0;
int SUM = 0;
int READINGS[WINDOW_SIZE];


int potPin = A0;            //potentiometer input pin location
const int RELAY_PIN = 3;    //relay output pin location

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c)

{
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}



void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
}



void loop()
{
//-----------------------------------------Retrieving the DO from the probe, converting it to a float, then to mg/L
  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  DObigS = String(readDO(ADC_Voltage, Temperaturet));
  
  DObigF = DObigS.toFloat();               //convert to float so we can do maths 
  DObigF = abs(DObigF);
  DObig = DObigF + (ADDITION*1000);        //add addition (if required)
  DO = DObig/1000;                        //convert to mg/L

//-----------------------------------------Calculate desired DO from potentiometer 
      potValue = analogRead(potPin);                  //read value from potentiometer
      DESIREDBIG = (map(potValue, 0, 1023, 0, 50));   //use that to calculate the desired DO
      DESIRED = DESIREDBIG/10;

//-----------------------------------------Calculate rolling average
      SUM = SUM - READINGS[INDEX];       // Remove the oldest entry from the sum
      READINGS[INDEX] = DObig;           // Read the next sensor value
      SUM = SUM + READINGS[INDEX];       // Add the newest reading to the sum
      SUM = abs(SUM);                    // Keep SUM positive (sometimes it goes negative, I'm not sure why)

      SUMfloat = float(SUM);                           //Convert the string to a floating point number
      AVERAGED = SUMfloat / (WINDOW_SIZE * 1000);      // Divide the sum of the window by the window size for the result

      count=count+1;
      INDEX = (INDEX+1) % WINDOW_SIZE;



        

//-----------------------------------------Print data the serial moniter
    Serial.print(count); 
    Serial.print(", ");
    Serial.print(INDEX); 
    Serial.print(", ");
    Serial.print(WINDOW_SIZE); 
    Serial.print(", ");
    Serial.print(potValue); 
    Serial.print(", ");
    Serial.print(DESIRED); 
    Serial.print(" - ");
    Serial.print(ADC_Raw); 
    Serial.print(", ");
    Serial.print(ADC_Voltage);
    Serial.print(" - ");
    Serial.print(DO); 
    Serial.print(", ");
    Serial.print(DObig);
    Serial.print(", ");
    Serial.print(SUM);
    Serial.print(", ");
    Serial.print(AVERAGED);
    Serial.print(", ");
    delay(20);

//-----------------------------------------Printing data to LCD
     lcd.setCursor(2, 0); 
     lcd.print(DO);
     lcd.print("  ");
     lcd.print(AVERAGED);
     lcd.print("     ");

//-----------------------------------------Comparing DO to desired DO, and turning flow ON/OFF
      if (AVERAGED >= DESIRED && count>WINDOW_SIZE) {          //if the DO is greater than or equal to desired and isnt first couple of measures, turn flow off
        Serial.println("high");                       
        digitalWrite(RELAY_PIN, LOW); 
        
        lcd.setCursor(2, 1);
        lcd.print("Flow OFF ");
        lcd.print(DESIRED, 2); 
      }
      
      if (AVERAGED < DESIRED && count>WINDOW_SIZE) {          //if the DO is less than desired and isnt first couple of measures, turn flow on                               
        Serial.println("low");                       
        digitalWrite(RELAY_PIN, HIGH); 
        
        lcd.setCursor(2, 1);
        lcd.print("Flow ON  "); 
        lcd.print(DESIRED,2);    
      }

      if (count<=WINDOW_SIZE) {                               
        Serial.println("Calculating...");                     //if it si the first couple of measures (less than window size), wait...                   
        digitalWrite(RELAY_PIN, LOW); 
        
        lcd.setCursor(2, 1);
        lcd.print("Calculating   "); 
        lcd.print(DESIRED,2);    
      }

  delay(100);

//-----------------------------------------Flashing dot so you know LCD isnt frozen
    lcd.setCursor(0, 1);
    lcd.print("."); 

  delay(DELAY);
    
    lcd.setCursor(0, 1);
    lcd.print(" "); 

  }
