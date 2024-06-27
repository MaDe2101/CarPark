// Other Libarys
#include "SevSeg.h"
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ++++ Const Values ++++
// General
const int default_value = -1;
const bool debug = false;

// Traffic Light
// pins
const int green_led = 22;
const int yellow_led = 23;
const int red_led = 24;

// Ultrasonic
// pins
const int trigger_car_in = 25;
const int echo_car_in = 26; 
const int trigger_car_out = 27;
const int echo_car_out = 28; 

const int car_passes_distance = 9;
const int offset = 1;
const int outlier_upper = 1000;
const int outlier_lower = 0;

const int max_parking_spaces = 6;

// Display
SevSeg sevseg;
const byte numDigits = 1;
const byte digitPins[] = {};
const byte segmentPins[] = {36, 35, 32, 33, 34, 37, 38, 39}; // pins
const bool resistorsOnSegments = true;
const byte hardwareConfig = COMMON_CATHODE;

// Ethernet
const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const IPAddress ip(192, 168, 0, 177);

// MQTT
const char mqttServerAddress[] = "test.mosquitto.org";
const int mqttServerPort = 1884;
const char connectionString = "S2310454007-S2310454013-car-park";
const char publishString = "S2310454007-S2310454013/outTopic";

// Force Sensor
const int parking_default_value = 1023;
const int parking_place_1 = A0;
const int parking_place_2 = A1;
const int parking_place_3 = A2;
const int parking_place_4 = A3;
const int parking_place_5 = A4;
const int parking_place_6 = A5;

// ---- Const Values ----

// ++++ Global Vars ++++
int counter = 0; // current amount of cars in the parking area
int free_parking_places = 6; // current amount of free parking places
int curr_led = default_value; // current led 
int last_distance_in = default_value;
int last_distance_out = default_value;
EthernetClient client;
PubSubClient mqttClient(client);
bool hasConnection = false;
// ---- Global Vars ----

void setup_ultrasonic(int echo, int trigger){
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);
  digitalWrite(trigger, LOW);
}

void setupEthernet(){
  IPAddress serverIP(216, 58, 213, 206); // IP-Adresse von google.com

  // not possible to check the link status but there is a led on the WS5100
  //https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.linkstatus/
  if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      Ethernet.begin(mac, ip); // try to congifure using IP address instead of DHCP:
    }

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield nicht vorhanden!");
    while (true);
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet-Kabel nicht angeschlossen!");
    while (true);
  }
}

void setDisplay(int number){
  if(number <= 9){
    sevseg.setNumber(number);
    sevseg.refreshDisplay(); 
  }
}

void setup() {  
  // important for debugging!
  Serial.begin(9600); // default baud rate
  while (!Serial) {}

  if(debug){  
    Serial.println("Start Setup!");
  }

  pinMode(A0, INPUT);

  // intit Leds
  pinMode(green_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(red_led, OUTPUT);

  // init distance sensor
  setup_ultrasonic(echo_car_in, trigger_car_in);
  setup_ultrasonic(echo_car_out, trigger_car_out);

  // display
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);
  setDisplay(0); 

  setupEthernet();

  mqttClient.setServer(mqttServerAddress, mqttServerPort);
  
  if(debug){  
    Serial.println("End Setup!");
  }
}

void FlashLight(const int led){
  if(curr_led != led) {
    digitalWrite(curr_led, LOW); 
    digitalWrite(led, HIGH); 
    curr_led = led;
  }
}

void ProcessTrafficLight(){
  if(counter <= 2) {
    FlashLight(green_led);
  }

  if((counter > 2) && (counter <= 4)) {
    FlashLight(yellow_led);
  }

  if((counter > 4)) {
    FlashLight(red_led);
  }
}

void ProcessDistance(int trigger, int echo, int &last_distance, void (*counterAction)(int &), const int car_passes_distance_ = car_passes_distance){
  long duration = 0;
  long distance = 0;

  digitalWrite(trigger, HIGH);
  digitalWrite(trigger, LOW);

  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2;

  if(distance >= outlier_upper || distance <= outlier_lower) {
    if(debug){  
      Serial.print("Distance is ");
      Serial.print(distance);
      Serial.println("Was outlier!");
    }

    return;
  }

  if(debug){  
    Serial.print("curr Distance = ");
    Serial.print(distance);
    Serial.print("cm; last Distance = ");
    Serial.print(last_distance);
    Serial.println("cm");
  }

  bool isInOffset = (distance > (last_distance - offset)) && (distance < (last_distance + offset)); 
  if((last_distance == default_value) || (!isInOffset)){
    if(debug){
      Serial.println("new distance was over old distance with offset");
    }
    if(distance < car_passes_distance_) {
      counterAction(counter);
      setDisplay(counter);
    }
  }
  
  last_distance = distance;
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (mqttClient.connect(connectionString)) {
      Serial.println("connected");     
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

void ProcessMqtt(){
  StaticJsonDocument<100> json;
  char msg[100];
  int msgSize; 

  if (!mqttClient.connected()) {
    reconnect();
  }

  json["counter"] = counter;  
  json["free_parking_places"] = free_parking_places;  
  json["max_parking_spaces"] = max_parking_spaces; 

  msgSize = serializeJson(json, msg, sizeof(msg));
  msg[msgSize] = '\0';

  mqttClient.publish(publishString, msg);
}

void process_parking_places() {
  int parkingPlaces[] = {parking_place_1, parking_place_2, parking_place_3, parking_place_4, parking_place_5, parking_place_6};
  int numParkingPlaces = sizeof(parkingPlaces) / sizeof(parkingPlaces[0]);
  int buffer = 0;

  for (int i = 0; i < numParkingPlaces; ++i) {
    if (analogRead(parkingPlaces[i]) != parking_default_value) {
      buffer++;
    }
  }

  free_parking_places = buffer;

  if(debug) {
    Serial.print("There are = ");
    Serial.print(free_parking_places);
    Serial.print(" Pakring spaces");
  }
}

void loop() {
  process_parking_places();

  if(hasConnection) {
    if (!mqttClient.connected()) {
      reconnect();
    }

    // Wait until the MQTT client is connected to the broker
    mqttClient.loop();
  }

  ProcessTrafficLight();
  ProcessDistance(trigger_car_in, echo_car_in, last_distance_in, [](int &counter) {counter++;});
  ProcessDistance(trigger_car_out, echo_car_out, last_distance_out, [](int &counter) {counter--;});

  // do here some fun with the six (hihi) weight teile do
  if(hasConnection){
    ProcessMqtt();
  }

  // hack for debugging
  if(counter > 6){
    counter = 0;
    setDisplay(counter);
  }
  
  if(debug) {
    Serial.print("Counter = ");
    Serial.println(counter);
  }

  delay(1000);
}
