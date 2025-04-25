#include <RTClib.h>

/* PINOUT DECLARATION */
// Stepper Driver
#define dirPin 2
#define stepPin 3
#define enablePin 4

// RGB Led
#define blueLedPin 5
#define greenLedPin 6
#define redLedPin 7

// Magnet
#define magnetoPin 8

// HC-SR04
#define trigPin 11  
#define echoPin 12 

// Trigger
#define switchPin 9

RTC_DS3231 rtc;
float duration, distance;  
bool timerEnabled = true;

void setup() {
  // Serial initialization
  Serial.begin(9600);

  // Configure INPUT
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(echoPin, INPUT); 
  pinMode(magnetoPin, INPUT_PULLUP); 

  // Configure OUTPUT
  pinMode(blueLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(trigPin, OUTPUT); 
  
  // Write OUTPUT
  digitalWrite(blueLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);
  digitalWrite(enablePin, HIGH);

  distance = 100.0;

  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  Serial.println("<-V-v-V-v-V-v-V-v-V-v-V-v-V->");
  Serial.println("POWERED ON at:");
  printDateTime(rtc.now());

  enableTimer(true);
}

void loop() {

  // Setup RTC Date and Time
  if (Serial.available()) {
    String buffer = Serial.readStringUntil('\n');
    setDateTime(buffer);
  }

  if(rtc.alarmFired(1)){
    // GREEN LED
    digitalWrite(blueLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);

    // Obtain the distance
    digitalWrite(trigPin, LOW);  
    delayMicroseconds(2);  
    digitalWrite(trigPin, HIGH);  
    delayMicroseconds(10);  
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH); 
    distance = (duration*.0343)/2;
    //Serial.print("Distance: ");  
    //Serial.println(distance); 
  }
  

  // If Button pressed
  if (digitalRead(switchPin) == LOW){  
    bool longPress = false;
    int counter = 0;

    while(digitalRead(switchPin) == LOW && longPress == false){
      if(counter > 10){
        longPress = true;
      }
      counter++;
      delay(100);
    }

    if(longPress == true && timerEnabled == true){
      // BLUE LED
      digitalWrite(blueLedPin, HIGH);
      digitalWrite(greenLedPin, LOW);
      digitalWrite(redLedPin, LOW);
      enableTimer(false);
    }else if(longPress == true && timerEnabled == false){
      // LED OFF
      digitalWrite(blueLedPin, LOW);
      digitalWrite(greenLedPin, LOW);
      digitalWrite(redLedPin, HIGH);
      enableTimer(true);
    }else{
      findZeroPosition(0);
    }
    delay(1000);
  }

  if (distance <= 20.0  && rtc.alarmFired(1)){    
    // RED LED
    digitalWrite(blueLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);

    distance = 100.0;

    findZeroPosition(0);
  
    enableTimer(true);  
  }

  delay (100);  
}

// FUNCTION DESCRIPTION
void enableTimer (bool enable){
  DateTime alarm;
  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  if(enable){
    Serial.println("Timer enabled at");
    printDateTime(rtc.now());

    rtc.disableAlarm(1);
    rtc.clearAlarm(1);
    
    alarm = (rtc.now());

    if( rtc.now().hour() >= 18){
      Serial.println("Stop for today :-)");
      alarm = DateTime(alarm.year(), alarm.month(), alarm.day(), 10, 0, 0);
      alarm = DateTime(alarm + TimeSpan(1, 0, 0, 0));
    }else if (rtc.now().hour() < 10){
      Serial.println("Stop for now :-)");
      alarm = DateTime(alarm.year(), alarm.month(), alarm.day(), 10, 0, 0);
    }else{
      alarm = (rtc.now() + TimeSpan(0, 2, 0, 0));
    }
    
    Serial.println("Next trigger at:");
    printDateTime(alarm);

    rtc.setAlarm1(alarm, DS3231_A1_Hour);
    timerEnabled = true;
  } else {
    Serial.println("Timer disabled at");
    printDateTime(rtc.now());

    rtc.disableAlarm(1);
    rtc.clearAlarm(1);
    timerEnabled = false;
  }
}


void printDateTime(DateTime dt) {
  char dateBuffer[] = "   DD/MM/YYYY   ";
  char timeBuffer[] = "    hh:mm:ss    ";

  Serial.print(dt.toString(dateBuffer));
  Serial.print(" --- ");
  Serial.println(dt.toString(timeBuffer));
}

void setDateTime(String value) {
  int d, m, y, h, min, s;
  int n = sscanf(value.c_str(), "%d %d %d %d %d %d", &d, &m, &y, &h, &min, &s);
    
  if (n == 6) {
    rtc.adjust(DateTime(y, m, d, h, min, s));
  }

  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  Serial.println("Data and Time setted to");
  printDateTime(rtc.now());

}

int findZeroPosition(int offsetFromMagnet){
  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  int exitStrategy = 10000;

  Serial.println("Move motor CCW Rotation at:");
  printDateTime(rtc.now());
  
  digitalWrite(enablePin, LOW);
  digitalWrite(dirPin, LOW);

  Serial.println("Magnet Open");
  // Spin the stepper motor CCW until magnet is found
  while(digitalRead(magnetoPin) == HIGH && exitStrategy > 0){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(10000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(10000);
    exitStrategy-=20;
  }

  Serial.println("Magnet Close");
  // Spin the stepper motor CCW until magnet is lost
  while(digitalRead(magnetoPin) == LOW && exitStrategy > 0){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(10000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(10000);
    exitStrategy-=20;
  }

  //Serial.println(offsetFromMagnet-counter);  
  // Spin the stepper motor CCW with offset
  for(int counter = 0; counter < offsetFromMagnet && exitStrategy > 0; counter ++){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(10000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(10000);
    Serial.print("Offset: ");
    exitStrategy-=20000;  
  }

  if(exitStrategy > 0){
    Serial.println("Motor rotation completed at:");
    printDateTime(rtc.now());
  }else{
    Serial.println("Motor rotation detection FAILED at:");
    printDateTime(rtc.now());
  }

  digitalWrite(enablePin, HIGH);
}