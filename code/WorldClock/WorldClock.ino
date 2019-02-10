#include "CommandShell.h"
#include "dateTimeValidator.h"
CommandShell CommandLine;

#include <RTClock.h>
#include <TimeLib.h>

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#include <Timezone.h>   // https://github.com/JChristensen/Timezone

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule edt = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule est = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone easternTime(edt, est);

// US Pacific Time Zone (San Francisco)
TimeChangeRule pdt = {"PDT", Second, Sun, Mar, 2, -420};    // Daylight time = UTC - 7 hours
TimeChangeRule pst = {"PST", First, Sun, Nov, 2, -480};     // Standard time = UTC - 8 hours
Timezone pacificTime(pdt, pst);

// Indian Time Zone (Bangalore)
TimeChangeRule idt = {"IST", Second, Sun, Mar, 2, +330};    // Daylight time = UTC + 5.5 hours
TimeChangeRule ist = {"IST", First, Sun, Nov, 2, +330};     // Standard time = UTC + 5.5 hours
Timezone indianTime(idt, ist);

Adafruit_7segment easternMatrix = Adafruit_7segment();
Adafruit_7segment pacificMatrix = Adafruit_7segment();
Adafruit_7segment indianMatrix = Adafruit_7segment();

RTClock rtclock (RTCSEL_LSE);

int setDateFunc(char * args[], char num_args);
int setTimeFunc(char * args[], char num_args);
int printTimeFunc(char * args[], char num_args);

/* UART command set array, customized commands may add here */
commandshell_cmd_struct_t uart_cmd_set[] =
{
  {
    "setDate", "\tsetDate [day] [month] [year]", setDateFunc      }
  ,
  {
    "setTime", "\tsetTime [hours] [minutes] [seconds]", setTimeFunc      }
  ,
  {
    "printTime", "\tprintTime", printTimeFunc      }
  ,
  {
    0,0,0      }
};

int setDateFunc(char * args[], char num_args) {
  if(3 != num_args) {
    Serial.println(F("Insufficient arguments!"));
    return 1;
  }

  uint8_t dayNum = atoi(args[0]);
  uint8_t monthNum = atoi(args[1]);
  uint16_t yearNum = atoi(args[2]);

  uint8_t tmp = validateDate(yearNum, monthNum, dayNum);

  if(tmp == 2) {
    Serial.println(F("Invalid year!"));
    return 2;
  } else if(tmp == 3) {
    Serial.println(F("Invalid month!"));
    return 3;
  } else if(tmp == 4) {
    Serial.println(F("Invalid day!"));
    return 4;
  }
  
  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = easternTime.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  newTime.Year = CalendarYrToTm(yearNum);
  newTime.Month = monthNum;
  newTime.Day = dayNum;
  
  tmp_t = makeTime(newTime);
  tmp_t = easternTime.toUTC(tmp_t);
  rtclock.setTime(tmp_t);
  setTime(tmp_t);

  Serial.print(F("Setting date to "));
  Serial.print(dayNum);
  Serial.print('/');
  Serial.print(monthNum);
  Serial.print('/');
  Serial.println(yearNum);
  return 0;  
}

int setTimeFunc(char * args[], char num_args) {
  if(3 != num_args) {
    Serial.println(F("Insufficient arguments!"));
    return 1;
  }

  int hourNum = atoi(args[0]);
  int minNum = atoi(args[1]);
  int secNum = atoi(args[2]);

  uint8_t tmp = validateTime(hourNum, minNum, secNum);

  if(tmp == 2) {
    Serial.println(F("Invalid hours!"));
    return 2;
  } else if(tmp == 3) {
    Serial.println(F("Invalid minutes!"));
    return 3;
  } else if(tmp == 4) {
    Serial.println(F("Invalid seconds!"));
    return 4;
  }

  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = easternTime.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  newTime.Hour = hourNum;
  newTime.Minute = minNum;
  newTime.Second = secNum;
  tmp_t = makeTime(newTime);
  tmp_t = easternTime.toUTC(tmp_t);
  rtclock.setTime(tmp_t);
  setTime(tmp_t);

  Serial.print(F("Setting time to "));
  Serial.print(hourNum);
  Serial.print(F(":"));
  Serial.print(minNum);
  Serial.print(F(":"));
  Serial.println(secNum);
  return 0;  
}

int printTimeFunc(char * args[], char num_args) {
  Serial.print(F("The current time is:"));

  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = easternTime.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  Serial.print(tmYearToCalendar(newTime.Year), DEC);
  Serial.print('/');
  Serial.print(newTime.Month, DEC);
  Serial.print('/');
  Serial.print(newTime.Day, DEC);
  Serial.print(' ');
  Serial.print(newTime.Hour, DEC);
  Serial.print(':');
  Serial.print(newTime.Minute, DEC);
  Serial.print(':');
  Serial.print(newTime.Second, DEC);
  Serial.println();
  return 0;
}

time_t getHwTime(void) {
  return rtclock.getTime();
}

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Starting"));

  setTime(getHwTime());
  setSyncProvider(getHwTime);
  setSyncInterval(600);

  Wire.begin();
  Wire.setClock(10000);
  easternMatrix.begin(0x70);
  pacificMatrix.begin(0x71);
  indianMatrix.begin(0x72);

  CommandLine.commandTable = uart_cmd_set;
  CommandLine.init(&Serial);
}

void loop(void) {
  uint8_t minuteOnes, minuteTens, hourOnes, hourTens;
  tmElements_t nowTime;
  time_t tmp_t, eastern_t, pacific_t, indian_t;
  
  tmp_t = now();
  eastern_t = easternTime.toLocal(tmp_t);
  pacific_t = pacificTime.toLocal(tmp_t);
  indian_t = indianTime.toLocal(tmp_t);

  CommandLine.runService();

  breakTime(eastern_t, nowTime);
  minuteOnes = nowTime.Minute % 10;
  minuteTens = nowTime.Minute / 10;
  hourOnes = nowTime.Hour % 10;
  hourTens = nowTime.Hour / 10;
  if (hourTens != 0) {
    easternMatrix.writeDigitNum(0, hourTens, false);
  } else {
    easternMatrix.writeDigitRaw(0, 0x00);
  }
  easternMatrix.writeDigitNum(1, hourOnes, true);
  easternMatrix.writeDigitNum(2, minuteTens, false);
  easternMatrix.writeDigitNum(3, minuteOnes, false);
  easternMatrix.writeDisplay();

  breakTime(pacific_t, nowTime);
  minuteOnes = nowTime.Minute % 10;
  minuteTens = nowTime.Minute / 10;
  hourOnes = nowTime.Hour % 10;
  hourTens = nowTime.Hour / 10;
  if (hourTens != 0) {
    pacificMatrix.writeDigitNum(0, hourTens, false);
  } else {
    pacificMatrix.writeDigitRaw(0, 0x00);
  }
  pacificMatrix.writeDigitNum(1, hourOnes, true);
  pacificMatrix.writeDigitNum(2, minuteTens, false);
  pacificMatrix.writeDigitNum(3, minuteOnes, false);
  pacificMatrix.writeDisplay();
  
  breakTime(indian_t, nowTime);
  minuteOnes = nowTime.Minute % 10;
  minuteTens = nowTime.Minute / 10;
  hourOnes = nowTime.Hour % 10;
  hourTens = nowTime.Hour / 10;
  if (hourTens != 0) {
    indianMatrix.writeDigitNum(0, hourTens, false);
  } else {
    indianMatrix.writeDigitRaw(0, 0x00);
  }
  indianMatrix.writeDigitNum(1, hourOnes, true);
  indianMatrix.writeDigitNum(2, minuteTens, false);
  indianMatrix.writeDigitNum(3, minuteOnes, false);
  indianMatrix.writeDisplay();
}
