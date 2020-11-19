//#include <analogWrite.h> //Import the analogWrite library for ESP32 so that analogWrite works properly
#include <WiFi.h>//Imports the needed WiFi libraries
#include <WiFiMulti.h> //We need a second one for the ESP32 (these are included when you have the ESP32 libraries)
#include <SocketIoClient.h> //Import the Socket.io library, this also imports all the websockets
#include <SPI.h> //SPI is the communication protocol used by the RFID reader
#include <MFRC522.h> //The library for the RFID reader
#include <Servo.h> //Servo library

WiFiMulti WiFiMulti; //Declare an instane of the WiFiMulti library
SocketIoClient webSocket; //Decalre an instance of the Socket.io library
const char* ssid = "Fn-2187";
const char* password = "hyperdeath";
const uint16_t port = 2520;
const char* host = "62.249.189.153";
int prev_LEDState = 0;
char RFIDcontent; 

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

//What we want the ESP32 to do if an unauthoirzed card is scanned
void unauthorizedCard(const char * unauthorizedData, size_t length) {
    Serial.println("Access denied");

    //Data conversion
    String dataString(unauthorizedData);
    int unauth = dataString.toInt();

    if (unauth == 1){
      digitalWrite(LED_R, HIGH); //YOU ARE UNAUTHORIZED, GIT
    }   
    else {
      digitalWrite(LED_R, LOW);
    }
}

/*
//The scanners general behaviour such as looking for/reading cards.
void scanner() {
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
    //Show UID on serial monitor
    Serial.println("UID tag :");
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
       Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
       Serial.println(mfrc522.uid.uidByte[i], HEX);
       content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
       content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    Serial.print("Message : ");
    content.toUpperCase();
    
    if (RFIDcontent = content.substring(1) == "0B 37 23 1B") //change here the UID of the card/cards that you want to give access
    {
      //Authorized card, tell the server
      char str[10]; //Decalre a char array (needs to be char array to send to server)
      itoa(RFIDcontent, str, 10); //Use a special formatting function to get the char array as we want to, here we put the analogRead value from port 27 into the str variable
      Serial.print("ITOA TEST: ");
      Serial.println(str);
      
      webSocket.emit("420", str); //Signals that the user is authorized and lit
      webSocket.emit("dataFromBoard", str); //Sends the RFID tag to the server
  }
        
   else   
   {
      //Unauthorized card, murder
      char str[10]; //Declare a char array (needs to be char array to send to server)
      itoa(RFIDcontent, str, 10); //Use a special formatting function to get the char array as we want to, here we put the analogRead value from port 27 into the str variable
      Serial.print("ITOA TEST: ");
      Serial.println(str);
      
      webSocket.emit("66", str); //Signals that the user is unauthorized
      webSocket.emit(RFIDcontent, str); //Sends the RFID tag to the server
      //Str indicates the data that is sendt every timeintervall, you can change this to "250" and se 250 be graphed on the webpage      
    }
}
*/
void doorOpen (const char * DoorStateData, size_t length){
  Serial.printf("Doorstate: %s\n", DoorStateData);
  Serial.println(DoorStateData);
  
  //Data conversion
  String dataString(DoorStateData);
  int DoorState = dataString.toInt();
  
  if (DoorState == 1){
    servo1.write(180);
    digitalWrite(LED_G, HIGH);
  }
  else {
    servo1.write(0);
    digitalWrite(LED_G. LOW);
  } 
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
    
    char str[10]; //Decalre a char array (needs to be char array to send to server)
    itoa(analogRead(34), str, 10); //Use a special formatting function to get the char array as we want to, here we put the analogRead value from port 27 into the str variable
    Serial.print("ITOA TEST: ");
    Serial.println(str);
    
    webSocket.emit("dataFromBoard", str); //Here the data is sendt to the server and then the server sends it to the webpage
    //Str indicates the data that is sendt every timeintervall, you can change this to "250" and se 250 be graphed on the webpage
  }
}

void setup() {
  
    Serial.begin(9600); //Start the serial monitor  
/*    pinMode(18,OUTPUT);
    pinMode(19,OUTPUT);
    pinMode(32,OUTPUT);
    pinMode(33,OUTPUT); */

    SPI.begin();          // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    pinMode(LED_G, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(SERVO_PIN, OUTPUT);
    pinMode(THERMO_PIN, INPUT);
    servo1.attach(SERVO_PIN);

    Serial.setDebugOutput(true); //Set debug to true (during ESP32 booting)

    Serial.println();
    Serial.println();
    Serial.println();

      for(uint8_t t = 4; t > 0; t--) { //More debugging
          Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
          Serial.flush();
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
    webSocket.on("door", doorOpen);
 //   webSocket.on("rfid_scanner", scanner);

    webSocket.begin(host, port); //This starts the connection to the server with the ip-address/domainname and a port (unencrypted)

    Serial.println("Place a card near the reader.");
    Serial.println();
}

//Drive the car forwards or backwards (THIS IS JUST AN EXAMPLE AND NOT WHAT YOU HAVE TO USE IT FOR)
void Drive(bool Direction){

  if(Direction) {
    //Turn motors one direction
  } else {
    //Turn it the other direction
  }
  
}

void loop() {
  webSocket.loop(); //Keeps the WebSocket connection running 
  //DO NOT USE DELAY HERE, IT WILL INTERFER WITH WEBSOCKET OPERATIONS
  //TO MAKE TIMED EVENTS HERE USE THE millis() FUNCTION OR PUT TIMERS ON THE SERVER IN JAVASCRIPT
}
