///ESP32 file to control engine and produce data for instruments///

#include <max6675.h>// by adafruit
#include <Wire.h>
#include <analogWrite.h>//by erropix
#include <Arduino.h>

#define IGN_PIN 5 // Replace with your GPIO pin number
#define FUEL_PWM 6 
#define THROTTLE_PIN 7
#define OIL_PWM 8
#define STARTER_PWM 9
#define START_BUTTON 10
#define RPM_PIN 11
#define N1_SELECT 12
#define ESC1 1//Fuel pump pwm pin
#define ESC2 2//Starter pwm pin

const int IDLE = 25; // Lowest fuel pump percentage when running
const int thermoDO = 2; // Digital pin connected to MAX6675 SO
const int thermoCS = 3; // Digital pin connected to MAX6675 CS
const int thermoCLK = 4; // Digital pin connected to MAX6675 SCK

//EGT Sensing
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const int RpmSlaveAdress = 9;

int EGT_READ() {
  float celsius = thermocouple.readCelsius();
  int intCelsius = int(celsius);
  return(intCelsius);
}

void THROTTLE_READ() {
  int rawValue = analogRead(THROTTLE_PIN);
  int percentage = map(rawValue, 0, 1023, IDLE, 100);
  int egt = EGT_READ();
  if(egt > 300 && egt < 950){
    ServoWrite(ESC1, percentage);
  }
  if(egt >= 950 && egt < 1020){
    int lowerpercentage = map(percentage, IDLE, 100, IDLE, 70);
    ServoWrite(ESC1, lowerpercentage);
  }
  if(egt >=1020){
    ServoWrite(ESC1, IDLE);
  }
  else {
    ServoWrite(ESC1, 0);
  }
}

void ServoWrite(int servo, int percent){
  int degree = map(percent, 0, 100, 0, 180);
  analogServo(servo, degree);
}

void PwmWrite(int pin, int percent){
  int pulseWidth = map(percent, 0, 100, 0, 255);
  analogWrite(pin, pulseWidth);
}

bool STARTER_READ() {
  bool buttonState = digitalRead(START_BUTTON);
  delay(1000);// Delay for reading state again to confirm button pressed 
  if(digitalRead(START_BUTTON) == buttonState){
    if(RequestRpm(RpmSlaveAdress) > 100){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
    }
}


long RequestRpm(int RpmSlaveAddress) {
  long receivedData = 0;
  Wire.requestFrom(RpmSlaveAddress, sizeof(receivedData));
  
  while(Wire.available() < sizeof(receivedData)) {
    // Wait for the data to be available
  }
  
  Wire.readBytes((uint8_t*)&receivedData, sizeof(receivedData));
  return receivedData;
}


void SHUTDOWN(){
  ServoWrite(1, 0); //Turn off fuel pump if not ignited
  digitalWrite(N1_SELECT, LOW); // Turn off generator and connect starter
  delay(200);
  ServoWrite(ESC2, 20); // Slow down starter to cool down and went
  digitalWrite(IGN_PIN, LOW); // Turn off ignition
  PwmWrite(OIL_PWM, 50); // 128 corresponds to 50% duty cycle of 255. Oilpump to 50%
  delay(10000);
  ServoWrite(ESC2, 0); // Turn starter off
  delay(5000);
  PwmWrite(OIL_PWM, 0); //Turn off oil pump

}

void START_ENGINE() {
  int n = 0;
  PwmWrite(OIL_PWM, 50); //Turn on oilpump to 50%
  delay(2000);
  ServoWrite(ESC2, 20); //Servo2 is strater motor, number is percent of rpm on starter
  delay(3000);
  ServoWrite(ESC2, 70);
  delay(3000);
  if(RequestRpm(RpmSlaveAdress) > 8000) {
    ServoWrite(ESC2, 100);
    PwmWrite(OIL_PWM, 75);
    digitalWrite(IGN_PIN, HIGH); // Turn on ignition
    delay(500);
    ServoWrite(ESC1, 15); //Turn on fuel pump at 15%
    delay(2000);
    if(EGT_READ() < 100){ // If start failed, egt not over 100c
      SHUTDOWN();
    }
    if(EGT_READ() <= 100){
      delay(2000);
      ServoWrite(ESC1, 25); //Turn on fuel pump at 25%
      digitalWrite(IGN_PIN, LOW); // Turn off ignition
      while(RequestRpm(RpmSlaveAdress) > 32000){
        if(n > 9){
          SHUTDOWN();
        }
        if(EGT_READ() > 1000){
          SHUTDOWN();
        }
        delay(500);
        n++;
      }
      ServoWrite(ESC2, 0); // Turn starter off
      ServoWrite(ESC1, 30); //Set fuel pump at 30%
      delay(2000);
      digitalWrite(N1_SELECT, HIGH); // Turn off starter and connect generator
    }
  else{
    SHUTDOWN();
    }
  }
}

void OIL_CONTROL(){
  long rpm = RequestRpm(RpmSlaveAdress);
  if(EGT_READ() > 250){
    if(rpm < 3000){
      PwmWrite(OIL_PWM, 0); //Turn off oil pump
    }
    else{
      PwmWrite(OIL_PWM, 40); //Set 40% pwm on oil pump
    }
  }
  else{
    if(rpm < 50000){
      PwmWrite(OIL_PWM, 60); //Set 60% pwm on oil pump
    }
    if(rpm >= 50000){
      PwmWrite(OIL_PWM, 90); //Set 90% pwm on oil pump
    }
    else{
      PwmWrite(OIL_PWM, 60); //Set 60% pwm on oil pump
    }
  }

}

void setup(){
  pinMode(N1_SELECT, OUTPUT); // Initialize the relay pin as an output
  pinMode(RPM_PIN, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(OIL_PWM, OUTPUT);
  pinMode(IGN_PIN, OUTPUT);
  pinMode(ESC1, OUTPUT);
  pinMode(ESC2, OUTPUT);
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Setup done.");
  Serial.println();
}

void loop(){

  if(STARTER_READ() == true){
    if(EGT_READ() > 250){
      START_ENGINE();
    }

    else{
      SHUTDOWN();
    }
  }
  THROTTLE_READ();
  OIL_CONTROL();

  //RPM SENS
  long rpm = RequestRpm(RpmSlaveAdress);

  //EGT READ
  int egt = EGT_READ();
}