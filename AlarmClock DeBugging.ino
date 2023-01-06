#include <TM1637Display.h>
#include <RTClib.h>
#include <Wire.h>

#define CLK 3
#define DIO 2

TM1637Display display(CLK, DIO);
RTC_DS3231 rtc;

const int setAlarmButton = 5;
const int setHour = 6;
const int setMin = 7;
const int turnOffAlarmButton = 8;
const int setTimeButton = 9;
const int nightModeButton = 10;
const int alarmLight = 11;
const int resetPrevAlarmTime = 12;
const int ambientLightDimmer = A3;
const int ambientLightButton = 13;
int alarmTime, prevAlarmTime, CurrentTime;
bool settingAlarm, activeAlarm, settingTime, turnOffLight, lightDimmable, nightMode = false; //Used to keep track of time and also to active loops - setting time loop and setting alarm loop.
int alarmHour, alarmMin = 0; //Used for setting the time of the alarm.
int timerForAlarmFunction, prevTimerForAlarmFunction = 0; //Varibales used to control setting the alarm button (allow for 1 button to have 2 functions).
int setTimeFunctionTimer, prevSetTimeFunctionTimer = 0; //Varibales to control adjusting the time button (allow for 1 button to have 2 functions).
int adjustedHour, adjustedMinutes, adjustedTime; //Varibales for when clock time is adjusted.
int changeHour, changeMinute = 0; //Used to change the clock time.
int ambientLightTimer, prevAmbientLightTimer = 0; //USed to make statemachine for ambientLight;
int nightModeTimer, prevNightModeTimer = 0;

void setup() {
  Serial.begin(57600);
  display.clear();
  display.setBrightness(7);

  pinMode(setAlarmButton, INPUT_PULLUP);
  pinMode(setHour, INPUT_PULLUP);
  pinMode(setMin, INPUT_PULLUP);
  pinMode(turnOffAlarmButton, INPUT_PULLUP);
  pinMode(setTimeButton, INPUT_PULLUP);
  pinMode(alarmLight, OUTPUT);
  pinMode(resetPrevAlarmTime, INPUT_PULLUP);
  pinMode(ambientLightButton, INPUT_PULLUP);
  pinMode(nightModeButton, INPUT_PULLUP);

  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if(rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
DateTime now = rtc.now();
DateTime future(now + TimeSpan(0, changeHour, changeMinute, 0));
DateTime futureAlarm(now + TimeSpan(0, changeHour, (changeMinute + 30), 0)); //Adds 30 mins to current or adjusted time, used to turn alarm light on 30 minutes before so it can progressively brighten.
adjustedTime = (future.hour()*100) + future.minute(); //Time to be displayed on the clock using the 2 above varibales.
CurrentTime = (now.hour()*100)+now.minute();//Varibale for time by the RTC - used for reference while coding.
static unsigned long notAlarmFunctionTimer = millis(); //Prevent the settingAlarm if statement from re-running. allows 1 button to start and end alarm function.
///////////////DISPLAYING THE TIME/////////
display.showNumberDecEx(adjustedTime, 0b11100000, true);
/////////////////////SETTING THE ALARM//////////////////////
int setAlarmButtonState = digitalRead(setAlarmButton);
if(setAlarmButtonState == LOW && millis() - notAlarmFunctionTimer > 500)
{
  settingAlarm = true;
  display.showNumberDecEx(alarmTime, 0b11100000, true);
  timerForAlarmFunction++;
  while(settingAlarm == true)
  {
    settingAlarmFunction();
  }
  notAlarmFunctionTimer = millis();
}

int resetPrevAlarmTimeState = digitalRead(resetPrevAlarmTime);
if(resetPrevAlarmTimeState == LOW)
{
  alarmTime = prevAlarmTime;
  activeAlarm = true;
  Serial.print("Previous Alarm Time: ");
  Serial.println(prevAlarmTime);
}
//////////////////SETTING THE TIME/////////////////////
int setTimeButtonState = digitalRead(setTimeButton);
static unsigned long notAdjustingClockTimer = millis(); // Keep track of time outside of setting time loop.

  if(setTimeButtonState == LOW && millis() - notAdjustingClockTimer > 500)
  {
  blinkClock(); //Function created to blink display, and display the adjustedTime.
  settingTime = true; // allows while loop/setTimefunction to run indefinitely until returned to false.
  setTimeFunctionTimer++; 
  while(settingTime == true) // Run setTimeFunction indefinitely.
    {
    setTimeFunction();
    }
  blinkClockAdjusted(); // adjusted function with newTimeSet from function.
  notAdjustingClockTimer = millis();
  } 

////////////////ALARM TURNING ON/////////////
int futureAlarmTime = (futureAlarm.hour()*100)+futureAlarm.minute();
int turnOffAlarmButtonState = digitalRead(turnOffAlarmButton);
if(futureAlarmTime == alarmTime && now.second() == 0 && activeAlarm == true) //If the time is equal to alarm time and the active alarmFunction is true - run this function.
{
  if(turnOffAlarmButtonState == LOW)
  {
    turnOffLight = true;
  }
  alarmFunction();
}

if(alarmTime == CurrentTime && now.second() == 0)
{
  digitalWrite(alarmLight, HIGH);
}
///////////ALARM TURNING OFF////////////

if(turnOffAlarmButtonState == LOW) // Used to turn off the alarm light .
{
  digitalWrite(alarmLight, LOW);
}

/////////TURN ON AMBIENT LIGHT///////////
static unsigned long notDimLightTimer = millis();
int readDimmerValue = analogRead(ambientLightDimmer);
int ambientLightButtonState = digitalRead(ambientLightButton);
if(ambientLightButtonState == LOW && millis() - notDimLightTimer > 500)
{
  ambientLightTimer++;
  lightDimmable = true;
  digitalWrite(alarmLight, HIGH);
  Serial.print("light Dimmabel: ");
  Serial.println(lightDimmable);
  while(lightDimmable == true)
  {
    dimLightFunction();
  } 
  notDimLightTimer = millis();
}

////////TURN ON/OFF NIGHT MODE//////////
int nightModeButtonState = digitalRead(nightModeButton);
if(nightModeButtonState == LOW)
{
  nightMode = !nightMode;
  if(nightMode == true)
  {
    display.setBrightness(1);
  }
  else 
  {
    display.setBrightness(7);
  }
  delay(200);
}

  Serial.print("Real Time: ");
  char buf1[] = "hh:mm:ss";
  Serial.println(now.toString(buf1));
  Serial.print("Alarm Time: ");
  Serial.println(alarmTime);

}

void blinkClock()
{
  display.setBrightness(0,false);//Following seriues of functions are used to blink the display (1st - set display off on next command),
  display.showNumberDecEx(adjustedTime, 0b11100000, true);//display turns off on this command,
  delay(20);//delay for 20 milliseconds for a prominent blink,
  display.setBrightness(7,true);//display turns on and brightness is set to 7 on next command,
  display.showNumberDecEx(adjustedTime, 0b11100000, true);//display turns on and brightness is up to 7 on this command.
};

void blinkClockAdjusted() //adjusted for the setting time if statement (at the end).
{
  display.setBrightness(0,false); // same functions as at the top of the loop.
  display.showNumberDecEx(adjustedTime, 0b11100000, true);
  delay(20);
  display.setBrightness(7,true);
  display.showNumberDecEx(adjustedTime, 0b11100000, true);
};

void settingAlarmFunction()
{
  static unsigned long alarmFunctionTimer = millis(); //To prevent function from recycling at end after clicking set button.
  if(timerForAlarmFunction != prevTimerForAlarmFunction) //TimerForAlarmFunction was incremented by 1 when the original button was clicked, therefore this statement will always run.
  {
    alarmFunctionTimer = millis(); //Reset the alarmFunctionTimer varibale, allowing accurate counting.
    prevTimerForAlarmFunction++; //Readjust this varibale therefore it is = timerForAlarmFunction.
  }
  int setAlarmButtonState = digitalRead(setAlarmButton);
  int setHourState = digitalRead(setHour);
  int setMinState = digitalRead(setMin);
  if(setHourState == LOW)
  {
    alarmHour++; //The placeholder for the alarm hour.
    if(alarmHour == 24)
    {
      alarmHour = 0;
    }
    display.showNumberDecEx(alarmHour, 0b11100000, true, 2, 0);
    delay(200);
  }
  if(setMinState == LOW)
  {
    alarmMin++; //place holder for alarm minute.
    if(alarmMin == 60)
    {
      alarmMin = 0;
      alarmHour++;
      if(alarmHour == 24)
      {
        alarmHour = 0;
      }
      display.showNumberDecEx(alarmHour, 0b11100000, true, 2, 0);
    }
    display.showNumberDecEx(alarmMin, 0b11100000, true, 2, 2);
    delay(200);
  }

  if(setAlarmButtonState == LOW && millis() - alarmFunctionTimer > 500)
  {
    settingAlarm = false; //Close out of this function.
    alarmTime = (alarmHour*100)+alarmMin;
    activeAlarm = true; //allows for the alarm to actually run, prevents the alarm from being set to 00:00 after running one time.
  }

};

void alarmFunction()
{
  for(int i = 1; i < 1800; i++)
  {
    int turnOffAlarmButtonState = digitalRead(turnOffAlarmButton);
    DateTime now = rtc.now();
    DateTime future(now + TimeSpan(0, changeHour, changeMinute, 0));
    int alarmBrightness = map(i, 0, 1800, 0, 255);
    adjustedTime = (future.hour()*100) + future.minute();
    display.showNumberDecEx(adjustedTime, 0b11100000, true);
    analogWrite(alarmLight, alarmBrightness);
    if(turnOffAlarmButtonState == LOW)
    {
      digitalWrite(alarmLight, LOW);
      break;
    }
    Serial.print("brightness: ");
    Serial.println(i);
    delay(1000);
  }
  prevAlarmTime = alarmTime;
  alarmTime = 0000;
  alarmHour = 0;
  alarmMin = 0;
  activeAlarm = false;
};

void setTimeFunction()
{
  DateTime now = rtc.now();
  DateTime future(now + TimeSpan(0, changeHour, changeMinute, 0));
  static unsigned long settingClockTimer = millis(); //Used to keep track og time in this loop - prevent immediate exit with button click.
  int setHourState = digitalRead(setHour); //Varibale to read state of hour setting button.
  int setMinState = digitalRead(setMin); //Varibale to read state of minute setting button.
  int setTimeButtonState = digitalRead(setTimeButton);
  if(setTimeFunctionTimer != prevSetTimeFunctionTimer) //same concept as the alarmTimeFunction.
  {
    settingClockTimer = millis();
    prevSetTimeFunctionTimer++;
  }
  if(setHourState == LOW)
  {
    changeHour++; //Amount to increment the hour by.
    DateTime future(now + TimeSpan(0, changeHour, changeMinute, 0));
    adjustedHour = future.hour();
    adjustedTime = (future.hour() * 100) + future.minute();
    display.showNumberDecEx(adjustedTime, 0b11100000, true);
    delay(200);
  }
  
  if(setMinState == LOW)
  {
    changeMinute++; //Keeps track of the change in the minutes.
    DateTime future(now + TimeSpan(0, changeHour, changeMinute, 0));
    adjustedMinutes = future.minute();
    adjustedTime = (future.hour() * 100) + future.minute();
    display.showNumberDecEx(adjustedTime, 0b11100000, true);
    delay(200);
  }

  if(setTimeButtonState == LOW && millis() - settingClockTimer > 500)
  {
    
    settingTime = false; //Close out of the loop.
    adjustedTime = (future.hour() * 100) + future.minute();
  }
  
};

void dimLightFunction()
{
  static unsigned long dimLightFunctionTimer = millis();
  int readDimmerValue = analogRead(ambientLightDimmer);
  int ambientLightButtonState = digitalRead(ambientLightButton);
  int lightDimmed = map(readDimmerValue, 0, 1023, 0, 255);
  if(ambientLightTimer != prevAmbientLightTimer)
  {
    dimLightFunctionTimer = millis();
    prevAmbientLightTimer++;
  }
  analogWrite(alarmLight, lightDimmed);
  Serial.print("Dim Light: ");
  Serial.println(readDimmerValue);
    if(ambientLightButtonState == LOW && millis() - dimLightFunctionTimer > 500)
    {
      lightDimmable = false;
      digitalWrite(alarmLight, LOW);
    }
  Serial.println("Loop activated");
};
