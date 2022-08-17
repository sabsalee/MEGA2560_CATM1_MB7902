#include "BG96.h" /* https://github.com/codezoo-ltd/CAT.M1_Arduino */
#include <Arduino.h>
#include <Time.h>       /* https://github.com/PaulStoffregen/Time */
#include <TimeAlarms.h> /* https://github.com/PaulStoffregen/TimeAlarms */
#include <SD.h>

#define DebugSerial Serial
#define M1Serial Serial1
#define PWR_PIN 2
#define STAT_PIN 3
#define DHT_PIN A0

/*
 * Be careful !!!
 * Keep the communication cycle with ThingSpeak.com for at least 3 minutes.
 */
// #define ALARM_CYCLE 3600 /* Seconds, 1hour */
#define ALARM_CYCLE   180    /*Seconds, 3min */

File myFile;

const int pwPin1 = 4; // MB7902 PwPin

long sensor;
float mm;

String WApiKey = "NX7NRN8OX0PVQBPW"; // Thing Speak Write API Key 16Character
String field1 = "height"; // Data Field for ThingSpeak

int _sendTag = 1;

BG96 BG96(M1Serial, DebugSerial, PWR_PIN, STAT_PIN);

void setup() {

  M1Serial.begin(115200);
  DebugSerial.begin(115200);

  /* SD Card Module Initialization */
  DebugSerial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    DebugSerial.println("initialization failed!");
    return;
  }
  DebugSerial.println("initialization done.");
  DebugSerial.println("SD Card Module Ready!!!")
  


  /* CAT.M1(BG96) Module Power On Sequence */
  if (BG96.isPwrON()) {
    DebugSerial.println("BG96 Power ON Status");
    if (BG96.pwrOFF()) {
      DebugSerial.println("BG96 Power Off Error");
    } else {
      DebugSerial.println("BG96 Power Off Success");
      DebugSerial.println("Module Power ON Sequence Start");
      if (BG96.pwrON()) {
        DebugSerial.println("BG96 Power ON Error");
      } else
        DebugSerial.println("BG96 Power ON Success");
    }
  } else {
    DebugSerial.println("BG96 Power OFF Status");
    if (BG96.pwrON()) {
      DebugSerial.println("BG96 Power ON Error");
    } else
      DebugSerial.println("BG96 Power ON Success");
  }

  /* BG96 Module Initialization */
  if (BG96.init()) {
    DebugSerial.println("BG96 Module Error!!!");
  }

  /* BG96 Module Power Saving Mode Disable */
  if (BG96.disablePSM()) {
    DebugSerial.println("BG96 PSM Disable Error!!!");
  }

  /* Network Regsistraiton Check */
  while (BG96.canConnect() != 0) {
    DebugSerial.println("Network not Ready !!!");
    delay(2000);
  }

  char szCCLK[32];
  int _year, _month, _day, _hour, _min, _sec;

  /* Get Time information from the Telco base station */
  if (BG96.getCCLK(szCCLK, sizeof(szCCLK)) == 0) {
    DebugSerial.println(szCCLK);
    sscanf(szCCLK, "%d/%d/%d,%d:%d:%d+*%d", &_year, &_month, &_day, &_hour,
           &_min, &_sec);

    /* Time Initialization */
    setTime(_hour, _min, _sec, _month, _day, _year);
  }

  /* Alarm Enable */
  Alarm.timerRepeat(ALARM_CYCLE, Repeats);

  DebugSerial.println("BG96 Module Ready!!!");
}

void Repeats() {
  DebugSerial.println("alarmed timer!");
  _sendTag = 1;
}

void loop() {

  if (_sendTag) {

    /* MB7092 Sensor Data Collect */
    sensor = pulseIn(pwPin1, HIGH);
    mm = sensor/5.5;
    DebugSerial.println(String(mm) + "mm");

    /* SD Card Write */
    myFile = SD.open("example.txt", FILE_WRITE);
    /* TODO - The code below may require a timestamp. */
    myFile.println(String(mm) + "mm");
    myFile.close();

    /* BG96 Send Data to ThingSpeak */
    char _IP[] =
        "api.thingspeak.com"; /* ThingSpeak Report Server api.thingspeak.com */
    unsigned long _PORT = 80;

    char recvBuffer[1024];
    int recvSize;
    String data = "GET /update";
    data += "?api_key=" + WApiKey + "&" + field1 + "=" + String(mm);
    data += " HTTP/1.1\r\n";

    data += "Host: api.thingspeak.com\r\n";
    data += "Connection: close\r\n\r\n";
    DebugSerial.println(data);

    if (BG96.actPDP() == 0) {
      DebugSerial.println("BG96 PDP Activation !!!");
    }
    /* TODO: Exception handling may be required
    to prevent system outages due to errors.*/

    if (BG96.socketCreate(1, _IP, _PORT) == 0)
      DebugSerial.println("TCP Socket Create!!!");

    /* Socket Send */
    if (BG96.socketSend(data.c_str()) == 0) {
      DebugSerial.print("[TCP Send] >>  ");
      DebugSerial.println(data);
    } else
      DebugSerial.println("Send Fail!!!");

    if (BG96.socketRecv(recvBuffer, sizeof(recvBuffer), &recvSize, 10000) ==
        0) {
      DebugSerial.print(recvBuffer);
      DebugSerial.println(" <<------ Read Data");
      DebugSerial.println(recvSize, DEC);
      DebugSerial.println("Socket Read!!!");
    }

    if (BG96.socketClose() == 0) {
      DebugSerial.println("Socket Close!!!");
    }

    delay(5000);

    if (BG96.deActPDP() == 0) {
      DebugSerial.println("BG96 PDP DeActivation!!!");
    }

    _sendTag = 0;
  }

  /* Time Test */
  DebugSerial.print(hour());
  DebugSerial.print(":");
  DebugSerial.print(minute());
  DebugSerial.print(":");
  DebugSerial.print(second());
  DebugSerial.println("");

  Alarm.delay(1000);
}
