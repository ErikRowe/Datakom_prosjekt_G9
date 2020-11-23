//#include <analogWrite.h> //Import the analogWrite library for ESP32 so that analogWrite works properly
#include <WiFi.h>//Imports the needed WiFi libraries
#include <WiFiMulti.h> //We need a second one for the ESP32 (these are included when you have the ESP32 libraries)
#include <SocketIoClient.h> //Import the Socket.io library, this also imports all the websockets
#include <SPI.h> //SPI is the communication protocol used by the RFID reader
#include <MFRC522.h> //The library for the RFID reader
#include <Servo.h> //Servo library

WiFiMulti WiFiMulti; //Declare an instane of the WiFiMulti library
SocketIoClient webSocket; //Decalre an instance of the Socket.io library
const char* ssid = "PolygondwanaLAN";
const char* password = "DrygolinOljedekkbeis";
const uint16_t port = 2520;
const char* host = "95.34.225.239";
int prev_LEDState = 0;
unsigned long time_now = 0; //Used to time the scanner function.
int interval = 1000;
int DoorState;

#define SS_PIN 21
#define RST_PIN 22
#define SERVO_PIN 25
#define LED_G 33 //define green LED pin
#define LED_R 32 //define red LED
#define THERMO_PIN 34 //Define pin for Thermistor.

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
Servo servo1;

void event(const char * payload, size_t length) { //Default event, what happens when you connect
  Serial.printf("got message: %s\n", payload);
}

//The scanners general behaviour such as looking for/reading cards.
void scanner() {
  long rfidKey;
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  } 

  if (millis() >= time_now + interval){
    /*
    //Show UID on serial monitor
    //Serial.println("UID tag :");
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
       //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
       //Serial.println(mfrc522.uid.uidByte[i], HEX);
       content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
       content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    Serial.print("UIDkey : ");
    content.toUpperCase();
    
    char str[13]; //Decalre a char array (needs to be char array to send to server)
    
    content.toCharArray(str,13);
    Serial.println(str);
    
    webSocket.emit("RFIDcontent", str); //Sends the RFID tag to the server
    */
      rfidKey =  mfrc522.uid.uidByte[0] << 24;
      rfidKey += mfrc522.uid.uidByte[1] << 16;
      rfidKey += mfrc522.uid.uidByte[2] <<  8;
      rfidKey += mfrc522.uid.uidByte[3];
      
      char str[11]; //Decalre a char array (needs to be char array to send to server)
 
      itoa(rfidKey, str, 11); //Use a special formatting function to get the char array as we want to, here we put the analogRead value from port 27 into the str variable
      Serial.print("ITOA TEST: ");
      Serial.println(str);

      
 
      webSocket.emit("RFIDcontent", str); //Here the data is sendt to the server and then the server sends it to the webpage
      //Str indicates the data that is sendt every timeintervall, you can change this to "250" and se 250 be graphed on the webpage
    time_now = millis();
  }
}

#define HALL_PIN 5

//Uses the hall effect sensor to sense if the door is closed or not
void doorState() {
  int sensorState = digitalRead(HALL_PIN);
  char str[10];
  
  if (sensorState != 0  && millis()> time_now + interval){
  
    itoa(1, str, 10);
    Serial.println("The door is open");
    webSocket.emit("doorState", str);
    time_now = millis();
  }  
}

void doorOpen (const char * DoorStateData, size_t length){
  Serial.printf("Doorstate: %s\n", DoorStateData);
  Serial.println(DoorStateData);
  
  //Data conversion
  String dataString(DoorStateData);
  DoorState = dataString.toInt();
  
  if (DoorState == 1){
    Serial.println("Opening door");
    servo1.write(180);
    digitalWrite(LED_G, HIGH);
  }
  else if (DoorState == 2){
    Serial.println("UNAUTHORIZED CARD");
    digitalWrite(LED_R, HIGH);
  }
  else {
    Serial.println("Closing door");
    servo1.write(0);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
  } 
}

//Voltage divider with thermistor as analog input. This data should be pushed onto the server.
float tempReader(int pin)
{
  float thermo_value, temp, thermo_voltage, thermo_resistance, resistor_voltage, ln;
  //Resistor in voltage divider is 10kOhm
  const int resistor = 10000;

  //Reads the analog input from the thermistor
  thermo_value = analogRead(pin);  
  
  //Scales the 12bit variable from the input to a voltage valure from 0-3.3V
  thermo_voltage = thermo_value*3.3/4096;
  
  //Calculates the voltage across the resistor
  resistor_voltage = 3.3-(thermo_voltage);
  
  //finds the resistance value of the thermistor
  thermo_resistance = (resistor_voltage/thermo_voltage)*resistor; 
  
  //Calculates the natural logarithm of the thermistor resistance divided by the resistor resistance.
  ln = log(thermo_resistance/resistor);
  
  //Calculates the temperature in kelvin the resistances. Assumes 300k as base temperature (T0).
  temp = 1.00/((1/300.00)+((1/3950.00)*ln));
  
  //Translates temp valure from kelvin to celcius.
  temp -= 273.15;  
  return temp;
}

void dataRequest(const char * DataRequestData, size_t length) {//This is the function that is called everytime the server asks for data from the ESP32
  Serial.printf("Datarequest Data: %s\n", DataRequestData);
  Serial.println(DataRequestData);

  //Data conversion
  String dataString(DataRequestData);
  int RequestState = dataString.toInt();

  Serial.print("This is the Datarequest Data in INT: ");
  Serial.println(RequestState);

  if(RequestState == 0) { //If the datarequest gives the variable 0, do this (default)

    char str[20];
    char str1[10]; //Decalre a char array (needs to be char array to send to server)
    char str2[10];
    itoa(tempReader(THERMO_PIN), str1, 10); //Use a special formatting function to get the char array as we want to, here we put the analogRead value from port 27 into the str variable
 //   Serial.print("ITOA TEST: ");
 //   Serial.println(str)   
    itoa(DoorState, str2, 10);

    strcpy(str, str1);
    strcat(str, str2);
    
    webSocket.emit("dataFromBoard", str);    
  }
}

void setup() {
  
    Serial.begin(9600); //Start the serial monitor  

    SPI.begin();          // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    pinMode(LED_G, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(SERVO_PIN, OUTPUT);
    pinMode(THERMO_PIN, INPUT);
    pinMode(HALL_PIN, INPUT);
    servo1.attach(SERVO_PIN);

    Serial.setDebugOutput(true); //Set debug to true (during ESP32 booting)

    Serial.println();
    Serial.println();
    Serial.println();

      for(uint8_t t = 4; t > 0; t--) { //More debugging
          Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
         // Serial.flush();
          delay(1000);
      }

    WiFiMulti.addAP(ssid, password); //Add a WiFi hotspot (addAP = add AccessPoint) (put your own network name and password in here)

    while(WiFiMulti.run() != WL_CONNECTED) { //Here we wait for a successfull WiFi connection untill we do anything else
      Serial.println("Not connected to wifi...");
        delay(100);
    }

    Serial.println("Connected to WiFi successfully!"); //When we have connected to a WiFi hotspot

    //Here we declare all the different events the ESP32 should react to if the server tells it to.
    //a socket.emit("identifier", data) with any of the identifieres as defined below will make the socket call the functions in the arguments below
    webSocket.on("clientConnected", event); //For example, the socket.io server on node.js calls client.emit("clientConnected", ID, IP) Then this ESP32 will react with calling the event function
    webSocket.on("dataRequest", dataRequest);
    webSocket.on("DoorControl", doorOpen);

    webSocket.begin(host, port); //This starts the connection to the server with the ip-address/domainname and a port (unencrypted)

    Serial.println("Place a card near the reader.");
    Serial.println();
}



void loop() {
  webSocket.loop(); //Keeps the WebSocket connection running 
  scanner();
  doorState();
  //DO NOT USE DELAY HERE, IT WILL INTERFER WITH WEBSOCKET OPERATIONS
  //TO MAKE TIMED EVENTS HERE USE THE millis() FUNCTION OR PUT TIMERS ON THE SERVER IN JAVASCRIPT
}
