// Libraries
#include "SevSeg.h"
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Constants
const int DEFAULT_VALUE = -1;
const bool DEBUG_MODE = false;

// Traffic Light
const int GREEN_LED = 22;
const int YELLOW_LED = 23;
const int RED_LED = 24;

// Ultrasonic
const int TRIGGER_CAR_IN = 25;
const int ECHO_CAR_IN = 26; 
const int TRIGGER_CAR_OUT = 27;
const int ECHO_CAR_OUT = 28; 

const int CAR_PASSES_DISTANCE = 9;
const int OFFSET = 1;
const int OUTLIER_UPPER = 1000;
const int OUTLIER_LOWER = 0;

const int MAX_PARKING_SPACES = 6;

// Display
SevSeg sevseg;
const byte NUM_DIGITS = 1;
const byte DIGIT_PINS[] = {};
const byte SEGMENT_PINS[] = {36, 35, 32, 33, 34, 37, 38, 39}; // Pins
const bool RESISTORS_ON_SEGMENTS = true;
const byte HARDWARE_CONFIG = COMMON_CATHODE;

// Ethernet
const byte MAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress IP(192, 168, 0, 177);

// MQTT
const char MQTT_SERVER_ADDRESS[] = "test.mosquitto.org";
const int MQTT_SERVER_PORT = 1883;
const char CONNECTION_STRING[] = "S2310454007-S2310454013-car-park";
const char PUBLISH_STRING[] = "S2310454007-S2310454013/outTopic";

// Force Sensor
const int PARKING_DEFAULT_VALUE = 1023;
const int PARKING_PLACES[] = {A0, A1, A2, A3, A4, A5};

// Global Variables
int counter = 0; // Current amount of cars in the parking area
int cars_driven_in = 0;
int cars_driven_out = 0;
int freeParkingPlaces = 6; // Current amount of free parking places
int currLed = DEFAULT_VALUE; // Current LED 
int lastDistanceIn = DEFAULT_VALUE;
int lastDistanceOut = DEFAULT_VALUE;
EthernetClient client;
PubSubClient mqttClient(client);
bool hasConnection = false;
int parkingPlaceResult[] = {0, 0, 0, 0, 0, 0};

// register function
void setDisplay(int number);
void setupUltrasonic(int echo, int trigger);
void setupEthernet();
void setup();
void flashLight(const int led);
void processTrafficLight();
bool validateCounter();
void processDistance(int trigger, int echo, int &lastDistance, void (*counterAction)(int &), const int carPassesDistance = CAR_PASSES_DISTANCE);
void reconnect();
void processMqtt();
void loop();

void setupUltrasonic(int echo, int trigger) {
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);
  digitalWrite(trigger, LOW);
}

void setupEthernet() {
  if (Ethernet.begin(MAC) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(MAC, IP);
  } else {
    hasConnection = true;
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield not present!");
    while (true);
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable not connected!");
    delay(1000);
    return;
  }
}

void setup() {  
  Serial.begin(9600); // Default baud rate
  while (!Serial) {}

  Serial.println("Start Setup!");

  pinMode(A0, INPUT);

  // Init LEDs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Init distance sensors
  setupUltrasonic(ECHO_CAR_IN, TRIGGER_CAR_IN);
  setupUltrasonic(ECHO_CAR_OUT, TRIGGER_CAR_OUT);

  // Init display
  sevseg.begin(HARDWARE_CONFIG, NUM_DIGITS, DIGIT_PINS, SEGMENT_PINS, RESISTORS_ON_SEGMENTS);
  sevseg.setBrightness(90);
  setDisplay(0); // Show 0 on display

  setupEthernet();

  mqttClient.setServer(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT);

  Serial.println("End Setup!");
}

void setDisplay(int number) {
  if (number <= 9) {
    sevseg.setNumber(number);
    sevseg.refreshDisplay(); 
  }
}

void flashLight(const int led) {
  if (currLed != led) {
    digitalWrite(currLed, LOW); 
    digitalWrite(led, HIGH); 
    currLed = led;
  }
}

void processTrafficLight() {
  if (counter <= 2) {
    flashLight(GREEN_LED);
  }

  else if (counter <= 4) {
    flashLight(YELLOW_LED);
  }

  else {
    flashLight(RED_LED);
  }
}

bool validateCounter(){
  bool result = false;
  if(counter < 0) {
    counter = 0;
    result = true;
  }

  if(counter > MAX_PARKING_SPACES){
    counter = MAX_PARKING_SPACES;
    result = true;
  }

  return result;
}

void processDistance(int trigger, int echo, int &lastDistance, void (*counterAction)(int &), const int carPassesDistance = CAR_PASSES_DISTANCE) {
  long duration = 0;
  long distance = 0;

  digitalWrite(trigger, HIGH);
  digitalWrite(trigger, LOW);

  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2;

  if (distance >= OUTLIER_UPPER || distance <= OUTLIER_LOWER) {
    Serial.print("Distance is ");
    Serial.print(distance);
    Serial.println(" was outlier!");
    return;
  }

  Serial.print("Current Distance = ");
  Serial.print(distance);
  Serial.print(" cm; Last Distance = ");
  Serial.print(lastDistance);
  Serial.println(" cm");
  
  bool isInOffset = (distance > (lastDistance - OFFSET)) && (distance < (lastDistance + OFFSET)); 
  if (lastDistance == DEFAULT_VALUE || !isInOffset) {
    Serial.println("New distance was over old distance with offset");
    if (distance < carPassesDistance) {
      counterAction(counter);
      setDisplay(counter);
    }
  }
  
  lastDistance = distance;
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (mqttClient.connect(CONNECTION_STRING)) {
      Serial.println("connected");     
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void processMqtt() {
  StaticJsonDocument<100> json;
  char msg[100];
  int msgSize; 

  if (!mqttClient.connected()) {
    reconnect();
  }

  JsonArray array;
  for (int i = 0; i < MAX_PARKING_SPACES; ++i) {
    array.add(parkingPlaceResult[i]);
  }


  //json["cars_in_parking_center"] = counter;  
  //json["free_parking_places"] = freeParkingPlaces; 
  json["CarsEntered"] = cars_driven_in;
  json["CarsLeft"] = cars_driven_out;
  json["parking_spaces"] = array; 

  msgSize = serializeJson(json, msg, sizeof(msg));
  msg[msgSize] = '\0';

  mqttClient.publish(PUBLISH_STRING, msg);
}

void processParkingPlaces() {
  int buffer = 0;

  for (int i = 0; i < MAX_PARKING_SPACES; ++i) {
    Serial.println("##########################");
    Serial.print(PARKING_PLACES[i]);
    Serial.print(": ");
    Serial.print(analogRead(PARKING_PLACES[i]));
    Serial.print(" = ");
    Serial.print(PARKING_DEFAULT_VALUE);
    Serial.println("##########################");

    parkingPlaceResult[i] = analogRead(PARKING_PLACES[i]) == PARKING_DEFAULT_VALUE;
    if (parkingPlaceResult[i]) {
      buffer++;
    }
  }

  freeParkingPlaces = buffer;
  Serial.print("There are = ");
  Serial.print(freeParkingPlaces);
  Serial.print(" Parking spaces");
  delay(2000);
}

void loop() {
  processParkingPlaces();

  if (hasConnection) {
    if (!mqttClient.connected()) {
      reconnect();
    }

    mqttClient.loop();
  }

  processTrafficLight();
  processDistance(TRIGGER_CAR_IN, ECHO_CAR_IN, lastDistanceIn, [](int &counter) {counter++; cars_driven_in++; if(validateCounter()) {cars_driven_in--; counter--;}});

  processDistance(TRIGGER_CAR_OUT, ECHO_CAR_OUT, lastDistanceOut, [](int &counter) {counter--; cars_driven_out++; if(validateCounter()) {cars_driven_out--; counter++;}});

  //processDistance(TRIGGER_CAR_IN, ECHO_CAR_IN, lastDistanceIn, [](int &counter) {counter++; cars_driven_in++; if(validateCounter() {cars_driven_in--; counter--;})});
  //processDistance(TRIGGER_CAR_OUT, ECHO_CAR_OUT, lastDistanceOut, [](int &counter) {counter--; cars_driven_out++; if(validateCounter() {cars_driven_out--; counter++;}});

  if (hasConnection) {
    processMqtt();
  }

  Serial.print("Counter = ");
  Serial.println(counter);

  delay(1000);
}
