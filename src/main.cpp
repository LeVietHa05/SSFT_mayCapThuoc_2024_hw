#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
//--------------------------------------------
#define dw digitalWrite
#define dr digitalRead

#define USE_SERIAL Serial
#define SERVER "pillport.net"
#define PORT 80

#define TOPIC1 "phoneNumber"
#define TOPIC2 "message"
#define TOPIC3 "pills"
#define TOPIC4 "/esp/pills"

#define CTHT1 33
#define CTHT2 32
#define CTHT3 35 // input only
#define CTHT4 34 // input only
#define CTHT5 39 // input only
#define CTHT6 36 // input only

#define SER1 25
#define SER2 26
#define SER3 27
#define SER4 14
#define SER5 12 // do not pull up
#define SER6 13

#define RUNG 4
#define COI 2
#define LED 15
// #define

#define RX1 22
#define TX1 23
#define RX2 16
#define TX2 17

//--------------------------------------------
SocketIOclient socketIO;
DFRobotDFPlayerMini myDFPlayer;
Servo servo[6];
int ctht[6] = {CTHT1, CTHT2, CTHT3, CTHT4, CTHT5, CTHT6};
int ser[6] = {SER1, SER2, SER3, SER4, SER5, SER6};
int rungIndex = 0;
unsigned long lastRung = 0;
//--------------------------------------------
void runServo(int id, int time);
//--------------------------------------------
void trigger(int time, int length, int pin)
{
  for (int i = 0; i < time; i++)
  {
    dw(pin, HIGH);
    delay(length);
    dw(pin, LOW);
    delay(length);
  }
}
//--------------------------------------------
void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  String temp = String((char *)payload);
  switch (type)
  {
  case sIOtype_DISCONNECT:
    USE_SERIAL.printf("[IOc] Disconnected!\n");
    break;
  case sIOtype_CONNECT:
    USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    break;
  case sIOtype_EVENT:
  {
    Serial.println("get event");
    String temp = String((char *)payload);
    Serial.println(temp);
    Serial.println(temp.length());

    // temp : "0987654321,1:1,2:1,3:1,4:0,5:0,6:0"
    if (temp.indexOf("/esp/pills") != -1)
    {
      trigger(3, 100, LED);
      trigger(3, 100, COI);
      temp = temp.substring(15);
      Serial.println("run the servo");
      for (int i = 0; i < 6; i++)
      {
        String temp1 = temp.substring(11 + i * 4, 11 + i * 4 + 3);
        runServo(i, temp1.substring(2, 3).toInt());
      }
    }
  }
  break;
  case sIOtype_ACK:
    USE_SERIAL.printf("[IOc] get ack: %u\n", length);
    break;
  case sIOtype_ERROR:
    USE_SERIAL.printf("[IOc] get error: %u\n", length);
    break;
  case sIOtype_BINARY_EVENT:
    USE_SERIAL.printf("[IOc] get binary: %u\n", length);
    break;
  case sIOtype_BINARY_ACK:
    USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
    break;
  }
}
//--------------------------------------------
void sendDataToServer(String topic, String msg)
{
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  array.add(topic);
  JsonObject data = array.createNestedObject();
  data["msg"] = msg;
  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);
}
//--------------------------------------------
void runTest()
{
}
//--------------------------------------------
void returnServoToOrigin()
{
  while (1)
  {
    for (int i = 0; i < 6; i++)
    {
      if (dr(ctht[i]) == LOW)
      {
        servo[i].write(90);
      }
    }
    if (dr(CTHT1) == LOW && dr(CTHT2) == LOW && dr(CTHT3) == LOW && dr(CTHT4) == LOW && dr(CTHT5) == LOW && dr(CTHT6) == LOW)
    {
      break;
    }
  }
}
//--------------------------------------------
void runServo(int id, int time)
{
  for (int i = 0; i < time; i++)
  {
    servo[id].write(180);
    delay(600);
    servo[id].write(90);
    delay(100);
    servo[id].write(0);
    delay(500);
    while (dr(ctht[id]) == HIGH) // wait for the pill to be taken
      ;
    servo[id].write(90);
  }
}
//--------------------------------------------
void setup()
{
  Serial.begin(115200); // UART for DFmini player
  Serial.print("runnign");

  // reposition the servo and init the pin
  for (int i = 0; i < 6; i++)
  {
    pinMode(ctht[i], INPUT_PULLUP);
    servo[i].attach(ser[i]);
    servo[i].write(30);
  }
  pinMode(RUNG, INPUT);
  pinMode(COI, OUTPUT);
  pinMode(LED, OUTPUT);

  returnServoToOrigin();

  trigger(1, 100, COI);

  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
  Serial.println("initialize the scanner");
  if (!myDFPlayer.begin(Serial1, /*isAck*/ true, /*doReset*/ true))
  { // Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    trigger(3, 1000, LED);
  }

  myDFPlayer.volume(10); // Set volume value. From 0 to 30
  WiFiManager wifiManager;
  wifiManager.autoConnect("PillPort");
  // WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    trigger(1, 100, LED);
  }

  Serial.println("WiFi connected");
  // server address, port and URL
  socketIO.begin(SERVER, PORT, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);
  trigger(3, 100, COI);
}
//--------------------------------------------
void loop()
{
  socketIO.loop();
  delay(10);
  while (Serial.available())
  {
    String temp = Serial.readStringUntil('\n');
    if (temp == "COI")
    {
      dw(COI, HIGH);
      delay(1000);
      dw(COI, LOW);
    }
    if (temp == "LED")
    {
      dw(LED, HIGH);
      delay(1000);
      dw(LED, LOW);
    }
  }
  while (Serial2.available()) // QR sensor
  {
    Serial.print("abc");
    String data = Serial2.readString();
    Serial.println(data);
    Serial.println(data.length());

    // check if data is valid
    if (data.length() != 35)
    {
      Serial.println("Invalid data. Length Check fail");
      trigger(3, 1000, COI);
      return;
    }
    // String data : "0987654321,1:1,2:1,3:1,4:0,5:0,6:0"
    // the 10 first characters are the phone number of patient
    //  the next 3 characters are the id of pill and number of pill
    // for example: 1:1 mean pill 1 take 1 pill
    // 4:0 mean pill 4 take 0 pill
    // write code take phone number and pill id and number of pill
    // then run the servo
    String phone = data.substring(0, 10);
    // send phone number to server for the record
    sendDataToServer(TOPIC3, data);
    // myDFPlayer.play(1); // play the sound
    trigger(3, 100, LED);
    trigger(3, 1000, COI);
    for (int i = 0; i < 6; i++)
    {
      String temp = data.substring(11 + i * 4, 11 + i * 4 + 3);
      // pills[1] = "1:1";
      // run servo i
      runServo(i, temp.substring(2, 3).toInt());
    }
  }

  // check if the rung button is trigger 10 times in 10 seconds
  if (dr(RUNG) == HIGH)
  {
    rungIndex++;
    if (rungIndex > 10)
    {
      dw(COI, HIGH);
    }
    delay(100);
  }
  // if not reset the rungIndex
  if (millis() - lastRung > 10000)
  {
    rungIndex = 0;
    lastRung = millis();
  }
}
