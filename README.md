# CarPark (SmartParking System)
## Description
The SmartParking System is a project aimed at developing a comprehensive parking management solution for a facility. It involves utilizing various sensors, microcontrollers, and networking technologies to efficiently ma nage parking spaces and provide real-time insights into parking availability.

## Build
The substructure was built in cardboard. Double walls and a double floor were installed in the substructure so that the cables could be laid nicely.

In the next picture you can see the shell:
![ShellConstruction design sketch](./img/ShellConstruction.jpeg)
The car parks have been placed in extreme locations to make it easier for us to use the weight sensors.
The large building was used to hide the electronics. Here is the technical room:
![electrics design sketch](./img/electrics.jpeg)

at the end the building was covered with old pieces of paper to make it look a little tidier.
The result is shown here:
![building design sketch](./img/building.jpeg)

## Architecture 
In this architectural design sketch I have decided not to include all cables and elements such as pre-resistors, breadboards etc. This sketch is only intended to show the structure of the application. This sketch is only intended to show the structure of the application

![architectural design sketch](./img/Architecture.png)

## Libraries Used
- **SevSeg:** Library for controlling seven-segment displays.
- **SPI:** Arduino library for SPI communication.
- **Ethernet:** Library for connecting Arduino boards to the internet via Ethernet.
- **PubSubClient:** MQTT client library for Arduino.
- **ArduinoJson:** Library for handling JSON data in Arduino projects.

## Global Constants
- `default_value`: Default value for uninitialized variables.
- `debug`: Flag for enabling or disabling debugging output.

## Traffic Light
- `green_led`, `yellow_led`, `red_led`: Pins for controlling the traffic light LEDs.

## Ultrasonic Sensor
- `trigger_car_in`, `echo_car_in`, `trigger_car_out`, `echo_car_out`: Pins for ultrasonic sensors used to detect incoming and outgoing cars.
- `car_passes_distance`: Threshold distance for detecting a passing car.
- `offset`: Offset value used to account for measurement inaccuracies.
- `outlier_upper`, `outlier_lower`: Upper and lower bounds for outlier distance measurements.
- `max_parking_spaces`: Maximum number of parking spaces in the facility.

## Display
- `SevSeg`: Configuration for the seven-segment display.

## Ethernet
- `mac`: MAC address for Ethernet connection.
- `ip`: IP address assigned to the Arduino board.

## MQTT
- `mqttServerAddress`, `mqttServerPort`: MQTT broker details.
- `connectionString`: Unique identifier for MQTT connection.
- `publishString`: MQTT topic for publishing data.

## Force Sensor
- Pins for force sensors located at each parking place.

## Global Variables
- `counter`: Current number of cars in the parking area.
- `free_parking_places`: Current number of free parking places.
- `curr_led`: Current state of the traffic light.
- `last_distance_in`, `last_distance_out`: Previous distance measurements from ultrasonic sensors.
- `client`: Ethernet client for network communication.
- `mqttClient`: MQTT client for publishing data.
- `hasConnection`: Flag indicating Ethernet connection status.

## Setup Functions
- `setup_ultrasonic()`: Initialize ultrasonic sensor pins.
- `setupEthernet()`: Initialize Ethernet connection.
- `setDisplay()`: Configure and display data on the seven-segment display.

## Main Setup Function
- `setup()`: Perform initial setup including pin configurations, sensor initialization, Ethernet setup, and MQTT client configuration.

## Utility Functions
- `FlashLight()`: Control the state of the traffic light.
- `ProcessTrafficLight()`: Determine the appropriate traffic light state based on the number of cars.
- `ProcessDistance()`: Measure distance using ultrasonic sensors and update car count accordingly.
- `reconnect()`: Reconnect to MQTT broker if connection is lost.
- `ProcessMqtt()`: Publish parking data to MQTT broker.
- `process_parking_places()`: Check the status of force sensors to determine available parking spaces.

## Main Loop
- Continuously monitor parking spaces, update traffic light, process ultrasonic sensor data, and publish data to MQTT broker.

## Workflow
1. Initialize sensors, display, Ethernet connection, and MQTT client in the setup phase.
2. Continuously monitor parking spaces and update display accordingly.
3. Adjust the traffic light based on the number of cars in the parking area.
4. Process incoming and outgoing cars using ultrasonic sensors.
5. Publish parking data to the MQTT broker for remote monitoring.
6. Loop back to step 2 and repeat.
