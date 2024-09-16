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
#define SERVER "pillport.vn"
#define PORT 80

#define TOPIC1 "phoneNumber"
#define TOPIC2 "message"
#define TOPIC3 "pills"

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

#define RX2 16
#define TX2 17

//--------------------------------------------
SocketIOclient socketIO;
// DFRobotDFPlayerMini myDFPlayer;
Servo servo[6];
int ctht[6] = {CTHT1, CTHT2, CTHT3, CTHT4, CTHT5, CTHT6};
int ser[6] = {SER1, SER2, SER3, SER4, SER5, SER6};
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
    servo[id].write(70);
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
  // for (int i = 0; i < 6; i++)
  // {
  //   pinMode(ctht[i], INPUT_PULLUP);
  //   servo[i].attach(ser[i]);
  //   servo[i].write(70);
  // }

  // returnServoToOrigin();

  // pinMode(RUNG, INPUT);
  // pinMode(COI, OUTPUT);
  pinMode(LED, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("initialize the scanner");
  // if (!myDFPlayer.begin(Serial, /*isAck*/ true, /*doReset*/ true))
  // { // Use softwareSerial to communicate with mp3.
  //   Serial.println(F("Unable to begin:"));
  //   Serial.println(F("1.Please recheck the connection!"));
  //   Serial.println(F("2.Please insert the SD card!"));
  //   for (int i = 0; i < 3; i++)
  //   {
  //     dw(LED, !dr(LED));
  //     delay(1000);
  //   }
  // }
  // myDFPlayer.volume(10); // Set volume value. From 0 to 30
  //   WiFiManager wifiManager;
  //   wifiManager.autoConnect("PillPort");
  //   // WiFi.begin(ssid, pass);
  //   while (WiFi.status() != WL_CONNECTED)
  //   {
  //     delay(500);
  //     Serial.print(".");
  //   }
  //   Serial.println("WiFi connected");
  //   // server address, port and URL
  //   socketIO.begin(SERVER, PORT, "/socket.io/?EIO=4");
  //
  //   // event handler
  //   socketIO.onEvent(socketIOEvent);
}
//--------------------------------------------
void loop()
{
  delay(10);
  while (Serial2.available()) // QR sensor
  {
    Serial.print("abc");
    String data = Serial2.readString();
    Serial.println(data);
    dw(LED, !dr(LED));

    // check if data is valid
    if (data.length() != 34)
    {
      Serial.println("Invalid data. Length Check fail");
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
    for (int i = 0; i < 6; i++)
    {
      String temp = data.substring(11 + i * 4, 11 + i * 4 + 3);
      // pills[1] = "1:1";
      // run servo i
      runServo(i, temp.substring(2, 3).toInt());
    }
  }
}
