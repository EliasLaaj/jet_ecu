///ESP32 file to control engine and produce data for instruments///

#include <max6675.h>
#include <RpmSensor.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <analogWrite.h>
#include <Arduino.h>

#define IGN_PIN 5 // Replace with your GPIO pin number
#define FUEL_PWM 6 
#define THROTTLE_PIN 7
#define OIL_PWM 8
#define STARTER_PWM 9
#define START_BUTTON 10
#define RPM_PIN 11
#define N1_SELECT 12

//EGT Sensing
Adafruit_MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

ServoESP32 servo1;//Fuel pump pwm
ServoESP32 servo2;//Starter pwm

const int IDLE = 25; // Lowest fuel pump percentage when running
const int thermoDO = 2; // Digital pin connected to MAX6675 SO
const int thermoCS = 3; // Digital pin connected to MAX6675 CS
const int thermoCLK = 4; // Digital pin connected to MAX6675 SCK

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
    moveServo(servo1, percentage);
  }
  if(egt >= 950 && egt < 1020){
    int lowerpercentage = map(percentage, IDLE, 100, IDLE, 70);
    moveServo(servo1, lowerpercentage);
  }
  if(egt >=1020){
    moveServo(servo1, IDLE);
  }
  else {
    moveServo(servo1, 0);
  }
}

bool STARTER_READ() {
  bool buttonState = digitalRead(START_BUTTON);
  delay(1000);// Delay for reading state again to confirm button pressed 
  if(digitalRead(START_BUTTON) == buttonstate){
    rpmSensor.update();
    if(rpmSensor.getRpmAverage() > 100){
      return true;
    }
    else(){
      return false;
    }
  }
  else(){
    return false;
    }
}


void moveServo(Servo &servo, int percent) {
  // Map the percentage value to the servo angle (0 to 180 degrees)
  int angle = map(percent, 0, 100, 0, 180);

  // Set the servo angle
  servo.write(angle);
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
  moveServo(servo1, 0); //Turn off fuel pump if not ignited
  digitalWrite(N1_SELECT, LOW); // Turn off generator and connect starter
  delay(200);
  moveServo(servo2, 20); // Slow down starter to cool down and went
  digitalWrite(IGN_PIN, LOW); // Turn off ignition
  analogWrite(OIL_PWM, 128); // 128 corresponds to 50% duty cycle of 255. Oilpump to 50%
  delay(10000);
  moveServo(servo2, 0); // Turn starter off
  delay(5000);
  analogWrite(OIL_PWM, 0); //Turn off oil pump

}

void START_ENGINE(){
  int n = 0
  analogWrite(OIL_PWM, 128); // 128 corresponds to 50% duty cycle of 255. Turn on oilpump to 50%
  delay(2000)
  moveServo(servo2, 25); //Servo2 is strater motor, number is percent of rpm on starter
  delay(3000);
  moveServo(servo2, 70);
  delay(3000)
  if(RequestRpm(RpmSlaveAdress) > 8000) {
    moveServo(servo2, 100);
    analogWrite(OIL_PWM, 185);
    digitalWrite(IGN_PIN, HIGH); // Turn on ignition
    dealy(500);
    moveServo(servo1, 15); //Turn on fuel pump at 15%
    delay(2000);
    if(EGT_READ() < 100){ // If start failed, egt not over 100c
      SHUTDOWN();
    }
    if(EGT_READ() <= 100){
      delay(2000)
      moveServo(servo1, 25); //Turn on fuel pump at 25%
      digitalWrite(IGN_PIN, LOW); // Turn off ignition
      while(RequestRpm(RpmSlaveAdress) > 32000){
        if(n > 9){
          SHUTDOWN();
        }
        if(EGT_READ() > 1000){
          SHUTDOWN();
        }
        delay(500)
        n++
      }
      moveServo(servo2, 0); // Turn starter off
      moveServo(servo1, 30); //Set fuel pump at 30%
      delay[2000]
      digitalWrite(N1_SELECT, HIGH); // Turn off starter and connect generator
    }
  else(){
    SHUTDOWN();
    }
  }
}

void OIL_CONTROL(){
  long rpm = RequestRpm(RpmSlaveAdress);
  if(EGT_READ() > 250){
    if(rpm < 3000){
      analogWrite(OIL_PWM, 0); //Turn off oil pump
    }
    else(){
      analogWrite(OIL_PWM, 102); //Set 40% pwm on oil pump
    }
  }
  else(){
    if rpm < 50000;{
      analogWrite(OIL_PWM, 153); //Set 60% pwm on oil pump
    }
    if(rpm >= 50000){
      analogWrite(OIL_PWM, 230); //Set 90% pwm on oil pump
    }
    else(){
      analogWrite(OIL_PWM, 153); //Set 60% pwm on oil pump
    }
  }

}

void setup(){
  pinMode(RELAY_PIN, OUTPUT); // Initialize the relay pin as an output
  pinMode(RPM_PIN, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(OIL_PWM, OUTPUT);
  pinMode(IGN_PIN, OUTPUT);
  rpmSensor.begin();//Starts rpm meter library
  servo1.attach(FUEL_PWM);  // Fuelpump pin
  servo2.attach(STARTER_PWM); // Starter motor pwm
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Setup done.");
  Serial.println();
}

void loop(){

  if(STARTER_READ() = true){
    if(EGT_READ() > 250){
      START_ENGINE();
    }

    else(){
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

///////