#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(8, 9);  //(rx,tx)
volatile int flow_frequency1;   // Measures flow sensor 1 pulses
volatile int flow_frequency2;

int count = 0;
int mcount = 0;
int acount = 0;

// Measures flow sensor 2 pulses
float vol1 = 0.0, l_minute1;
float vol2 = 0.0, l_minute2;
float fvol = 0.0;
float f1vol = 0.0;
float mvol1 = 0.0, ml_minute1;
float mvol2 = 0.0, ml_minute2;

unsigned char flowsensor1 = 2;  // Sensor 1 Input
unsigned char flowsensor2 = 3;  // Sensor 2 Input

const int switchPin = 6;  // For turning on and off the monitor mode
const int relay = 7;      //for relay-solenoid
const int beep = 5;
// I2C LCD address (change this if your address is different)
#define I2C_LCD_ADDR 0x27

LiquidCrystal_I2C lcd(I2C_LCD_ADDR, 16, 2);  // Initialize the I2C LCD

unsigned long currentTime;
unsigned long cloopTime;
unsigned long lastResetTime = 0;
const unsigned long volumeResetInterval = 86400000;  // 24 hours in milliseconds

bool monitorMode = false;  // Indicates whether the monitor mode is active or not

void flow1()  // Interrupt function for sensor 1
{
  flow_frequency1++;
}

void flow2()  // Interrupt function for sensor 2
{
  flow_frequency2++;
}

void setup() {
  pinMode(flowsensor1, INPUT);
  pinMode(flowsensor2, INPUT);
  pinMode(switchPin, INPUT_PULLUP);
  digitalWrite(flowsensor1, HIGH);  // Optional Internal Pull-Up
  digitalWrite(flowsensor2, HIGH);  // Optional Internal Pull-Up
  pinMode(relay, OUTPUT);
  pinMode(beep, OUTPUT);
  Serial.begin(9600);  // Commented out to remove Serial.print statements
  mySerial.begin(9600);

  // Set GSM module to use text mode for SMS

  lcd.init();  // initialize the lcd
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.print("Water Flow Meter");

  currentTime = millis();
  cloopTime = currentTime;
  lastResetTime = currentTime;  // Set the initial reset time to the current time

  attachInterrupt(digitalPinToInterrupt(flowsensor1), flow1, RISING);  // Setup Interrupt for sensor 1
  attachInterrupt(digitalPinToInterrupt(flowsensor2), flow2, RISING);  // Setup Interrupt for sensor 2
}

void loop() {
  currentTime = millis();

  int switchState = digitalRead(switchPin);  // Read the state of the switch
  if (switchState == LOW) {
    // Serial.println("Switch is pressed");  // Commented out to remove Serial.print statements
    monitorMode = true;
    // Activate monitor mode
  } else {
    monitorMode = false;  // Deactivate monitor mode
  }

  // If monitor mode is active, display "Monitoring Leakage"
  if (monitorMode) {
    lcd.clear();
    lcd.setCursor(0, 0);  // Set cursor to the first row
    lcd.print("Monitoring");
    lcd.setCursor(0, 1);  // Set cursor to the second row
    lcd.print("Leakage...");
    delay(300);
    ml_minute1 = (flow_frequency1 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
    ml_minute1 = ml_minute1 / 60;
    mvol1 = mvol1 + ml_minute1;

    ml_minute2 = (flow_frequency2 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
    ml_minute2 = ml_minute2 / 60;
    mvol2 = mvol2 + ml_minute2;

    fvol = mvol1 + mvol2;

    flow_frequency1 = 0;
    flow_frequency2 = 0;

    // Serial.println(mvol1);  // Commented out to remove Serial.print statements
    // Serial.println(mvol2);  // Commented out to remove Serial.print statements
    // Serial.println(fvol);   // Commented out to remove Serial.print statements

    // Check if fvol crosses 2 liters
    if (fvol >= 2.0) {
      lcd.clear();
      lcd.setCursor(0, 0);  // Set cursor to the first row
      lcd.print("Leak Detected");
      lcd.setCursor(0, 1);  // Set cursor to the second row
      lcd.print("Alarming");

      // code for sending noti regarding leak
      if (count < 1) {
        sendSMS("Leak detected - ring the GSM for 5 secs to stop water flow");
        count = 99;
      }

      delay(5000);
      checkIncomingCall();
    }

    checkIncomingCall();
    Serial.print(f1vol);
Serial.print(",");
Serial.println(fvol);

  } else {
    // Check if 24 hours have passed since the last volume reset
    if (currentTime - lastResetTime >= volumeResetInterval) {
      lastResetTime = currentTime;  // Update the last reset time
      // Reset the volume variables to zero
      vol1 = 0.0;
      vol2 = 0.0;
      f1vol = 0.0;
      count = 0;
      fvol = 0;
      mvol2 = 0;
      mvol1 = 0;
      acount = 0;
    }
      count = 0;
      fvol = 0;
      mvol2 = 0;
      mvol1 = 0;
      acount = 0;
    // Every second, calculate and print litres/hour for sensor 1
    if (currentTime >= (cloopTime + 1000)) {
      cloopTime = currentTime;  // Updates cloopTime

      bool hasFlow1 = (flow_frequency1 != 0);
      bool hasFlow2 = (flow_frequency2 != 0);

      lcd.clear();

      if (hasFlow1 && hasFlow2) {
        l_minute1 = (flow_frequency1 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
        l_minute1 = l_minute1 / 60;
        vol1 = vol1 + l_minute1;

        l_minute2 = (flow_frequency2 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
        l_minute2 = l_minute2 / 60;
        vol2 = vol2 + l_minute2;

        lcd.setCursor(0, 0);  // Set cursor to the first row
        lcd.print("Volume 1: ");
        lcd.print(vol1);
        lcd.print(" L");
        lcd.setCursor(0, 1);  // Set cursor to the second row
        lcd.print("Volume 2: ");
        lcd.print(vol2);
        lcd.print(" L");
        f1vol = vol1 + vol2;
        

  
      } else if (hasFlow1) {
        l_minute1 = (flow_frequency1 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
        l_minute1 = l_minute1 / 60;
        vol1 = vol1 + l_minute1;

        lcd.setCursor(0, 0);  // Set cursor to the first row
        lcd.print("S1 flow: ");
        lcd.print(l_minute1);
        lcd.print(" L/M");
        lcd.setCursor(0, 1);  // Set cursor to the second row
        lcd.print("Volume 1: ");
        lcd.print(vol1);
        lcd.print(" L");
        f1vol = vol1;
        
      } else if (hasFlow2) {
        l_minute2 = (flow_frequency2 / 7.045);  // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
        l_minute2 = l_minute2 / 60;
        vol2 = vol2 + l_minute2;

        lcd.setCursor(0, 0);  // Set cursor to the first row
        lcd.print("S2 flow: ");
        lcd.print(l_minute2);
        lcd.print(" L/M");
        lcd.setCursor(0, 1);  // Set cursor to the second row
        lcd.print("Volume 2: ");
        lcd.print(vol2);
        lcd.print(" L");
        f1vol = vol2;
        
      } else {
        // If there's no flow from both sensors, display Volume 1 and Volume 2
        lcd.setCursor(0, 0);  // Set cursor to the first row
        lcd.print("Volume 1: ");
        lcd.print(vol1);
        lcd.print(" L");
        lcd.setCursor(0, 1);  // Set cursor to the second row
        lcd.print("Volume 2: ");
        lcd.print(vol2);
        lcd.print(" L");
        f1vol = vol1 + vol2;
       
      }
Serial.print(f1vol);
Serial.print(",");
Serial.println(fvol);

      // Serial.println(f1vol);  // Commented out to remove Serial.print statements
      flow_frequency1 = 0;  // Reset Counter for sensor 1
      flow_frequency2 = 0;  // Reset Counter for sensor 2

      if (f1vol >= 5) {
        lcd.clear();
        lcd.setCursor(0, 0);  // Set cursor to the first row
        lcd.print("LIMIT REACHED!");
        lcd.setCursor(0, 1);  // Set cursor to the second row
        lcd.print("TOTAL VOL: ");
        lcd.print(f1vol);
        lcd.print(" L");
        alarm();
         acount=99;
      }
      
    }
  }
}

void updateSerial() {
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read());  // Forward what Serial received to Software Serial Port
  }
  while (mySerial.available()) {
    Serial.write(mySerial.read());  // Forward what Software Serial received to Serial Port
  }
}

void sendSMS(const String& message) {
  // Your SMS sending code goes here

  mySerial.println("AT");
  updateSerial();

  mySerial.println("AT+CMGF=1");
  updateSerial();

  mySerial.println("AT+CMGS=\"+917695887731\"");  // enter your phone number here (prefix country code)
  updateSerial();

  mySerial.print(message);  // Pass the SMS message as a parameter
  updateSerial();
  mySerial.write(26);

  delay(5000);
}

void checkIncomingCall() {
  if (mySerial.available()) {
    String response = mySerial.readStringUntil('\n');
    // Serial.println(response);  // Commented out to remove Serial.print statements

    // If the incoming call notification is detected
    if (response.indexOf("RING") != -1) {
      // You can add your custom logic here to handle the incoming call
      // Serial.println("Incoming call detected!");  // Commented out to remove Serial.print statements

      // Turn on the relay
      digitalWrite(relay, HIGH);

      // Send an SMS

      sendSMS("Water flow has been stopped. Check after going home.");

      // For example, you can hang up the call after a few seconds
      delay(2000);              // Wait for 2 seconds
      mySerial.println("ATH");  // Hang up the call
      // Serial.println("Call hung up.");  // Commented out to remove Serial.print statements
    }
  }
}

void alarm() {
  if(acount<1){

    sendSMS("water usage limit reached.");
    
  }
 
  digitalWrite(beep, HIGH);
  delay(1000);
  digitalWrite(beep, LOW);
  delay(5000);
}
