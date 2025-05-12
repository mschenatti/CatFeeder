#include <RTClib.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

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

// ESP8266
#define feedNotificationPin 10

// HC-SR04
#define trigPin 11  
#define echoPin 12 

// Trigger
#define switchPin 9

RTC_DS3231 rtc;
SSD1306AsciiWire oled;
DateTime feedAlarm;
DateTime dayAlarm;

int alarm_index = 0;
int alarm_counter = 0;

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
  pinMode(feedNotificationPin, OUTPUT);
  
  // Write OUTPUT
  // BLUE LED
  digitalWrite(blueLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);

  digitalWrite(enablePin, HIGH);
  digitalWrite(feedNotificationPin, LOW);

  distance = 100.0;

  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  Serial.println("<-V-v-V-v-V-v-V-v-V-v-V-v-V->");
  Serial.println("POWERED ON at:");
  printDateTime(rtc.now());
  setFeedAlarm(true);

  //Display init
  oled.begin(&Adafruit128x64, 0x3C);
  oled.clear();
  oled.displayRemap(false);          // rotate text 180°
  
  printOledInfo();
  printAlarmInfo();
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

  if(rtc.alarmFired(2)){
    alarm_index = 0;
    alarm_counter = 0;
    oled.clear();
    printOledInfo();
    printAlarmInfo();
    setDayAlarm();
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
      setFeedAlarm(false);
    }else if(longPress == true && timerEnabled == false){
      setFeedAlarm(true);
    }else{
      feed(0);
    }
    delay(1000);
  }

  if (distance <= 20.0  && rtc.alarmFired(1)){    
    // BLUE LED
    digitalWrite(blueLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);

    distance = 100.0;

    feed(0);
  
    setFeedAlarm(true); 
  }

  delay (100);  
}

// FUNCTION DESCRIPTION
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

  //setFeedAlarm(true);
  setDayAlarm();

  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  Serial.println("Data and Time setted to");
  printDateTime(rtc.now());

}

void setFeedAlarm (bool enable){
  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  if(enable){
    // BLUE LED
    digitalWrite(blueLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);

    Serial.println("Timer enabled at");
    printDateTime(rtc.now());

    rtc.disableAlarm(1);
    rtc.clearAlarm(1);
    
    feedAlarm = (rtc.now());

    if( rtc.now().hour() >= 18){
      Serial.println("Stop for today :-)");
      feedAlarm = DateTime(feedAlarm.year(), feedAlarm.month(), feedAlarm.day(), 10, 0, 0);
      feedAlarm = DateTime(feedAlarm + TimeSpan(1, 0, 0, 0));
    }else if (rtc.now().hour() < 10){
      Serial.println("Stop for now :-)");
      feedAlarm = DateTime(feedAlarm.year(), feedAlarm.month(), feedAlarm.day(), 10, 0, 0);
    }else{
      feedAlarm = (rtc.now() + TimeSpan(0, 2, 0, 0));
    }
    
    Serial.println("Next trigger at:");
    printDateTime(feedAlarm);

    rtc.setAlarm1(feedAlarm, DS3231_A1_Hour);
    timerEnabled = true;
  } else {
    // RED LED
    digitalWrite(blueLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);

    Serial.println("Timer disabled at");
    printDateTime(rtc.now());

    rtc.disableAlarm(1);
    rtc.clearAlarm(1);

    feedAlarm = DateTime(0,0,0,0,0,0);
    rtc.setAlarm1(feedAlarm, DS3231_A1_Day);

    timerEnabled = false;
  }

  printAlarmInfo();
}

void setDayAlarm(){
  rtc.disableAlarm(2);
  rtc.clearAlarm(2);
  dayAlarm = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), 0, 0, 0);
  dayAlarm = DateTime(dayAlarm + TimeSpan(1, 0, 0, 0));
  rtc.setAlarm2(dayAlarm, DS3231_A2_Day);
}

int feed(int offsetFromMagnet){
  Serial.println("<-O-o-O-o-O-o-O-o-O-o-O-o-O->");
  int exitStrategy = 10000;
  //int exitStrategy = 100;

  alarm_index = alarm_counter%6 + 1;
  alarm_counter++;

  printOledInfo();
  
  Serial.println("Move motor CCW Rotation at:");
  printDateTime(rtc.now());
  
  digitalWrite(enablePin, LOW);
  digitalWrite(dirPin, LOW);

  // Send notification to ESP8266
  digitalWrite(feedNotificationPin, HIGH);
  delay(200);
  digitalWrite(feedNotificationPin, LOW);

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

void printOledInfo(){
  char timeBuffer[] = "hh:mm ";

  // LIST
  if(alarm_counter != 0){
    oled.setFont(Callibri10);
    oled.SSD1306Ascii::setCursor(0,alarm_index);
    oled.print(alarm_counter);
    oled.print("-");
    oled.print(rtc.now().toString(timeBuffer));
  }else{
    // HEADER
    oled.setFont(Callibri10);
    oled.SSD1306Ascii::setCursor(0,0);
    oled.print(" TRG:");
  }

  // COUNTER
  oled.setFont(CalBlk36);
  oled.SSD1306Ascii::setCursor(70,3);
  oled.print(alarm_counter);
  oled.print(" ");
}

void printAlarmInfo(){
  // ALARM
  oled.setFont(Callibri15); 
   oled.SSD1306Ascii::setCursor(50,0);

  if(feedAlarm.day() == 0){
    oled.print("-Disarmed-");    
  }else{
    oled.print("Next: ");
    char alarmBuffer[] = "hh:mm";
    oled.print(feedAlarm.toString(alarmBuffer));

  }
}

