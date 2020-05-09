# ESP32-BLE [![Build Status](https://travis-ci.com/nacansino/esp32-ble.svg?branch=master)](https://travis-ci.com/nacansino/esp32-ble)

Simple demo of [Bluetooth Low Energy (BLE)](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy) functionality in ESP32 chip from EspressIf.

In this repository, an ESP32 chip serves as a low-power terminal device that can be configured to receive sensor information through its peripherals and control actuators.

For demonstration purposes, only a simple analog temperature sensor, LED, and switch are connected.

This is a part of a [bigger project](https://github.com/nacansino/Raspberry-Pi-as-Bluetooth-Hub) which transfers data to and from a cloud service provider.

## Tech Stack
| Component |  Stack    |
|-----------|-----------|
| Framework | [Arduino](https://arduino.cc/) |
| IDE       | [PlatformIO](https://platformio.org/)   |
| Board     | [ESP-WROOM-32](https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z)|
| CI Framework | [Travis-CI](https://travis-ci.com/nacansino/esp32-ble.svg?branch=master)|

## Circuit Diagram
-- Todo

## Bluetooth Custom Service
The concept of Attribute Protocol (ATT) and Generic Attribute Profile (GATT) will not be discussed here, but is available on the [Bluetooth Special Interest Group (SIG) portal](https://www.bluetooth.com/specifications/). As a standard, the SIG established profiles for common BLE applications such as health monitors and fitness machine. These profiles dictate the identifiers or UUIDs (Universally Unique IDs) for each message (e.g. blood pressure, glucose level). These specifications are assigned as standard, but is not required to make BLE work.
In this demonstration, however, we will create a custom service with our custom data format. UUIDs are formatted as 128-bit generated using this [free generator](https://www.uuidgenerator.net/).

## Our Custom Service
[This](https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/) is an easy-to-understand explanation of how BLE and GATT works.

Basically, we use the term *profile* to identify our application. A profile has one or many *services*. A *service* can have one or multiple *characteristics*, which is the smallest unit in our hierarchy. 
The *characteristics* hold the *value* and *descriptor*. Its *properties* describe whether this characteristic can be read, written on, notify, indicate, etc. The complete list is on the article I linked above.

[gatt-hierarchy](https://i2.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/06/GATT-BLE-ESP32.png?w=750&ssl=1)
Image taken from randomnerdtutorials.com

There is only one custom service in this demonstration. It holds four characteristics with different properties.
### Service
UUID of Service: **4fafc201-1fb5-459e-8fcc-c5c9c331914b**
### Characteristics
| UUID   | Specification |  Properties    | 
|----------|-----------|-------|
| 229a8d41-fde4-44ff-8dad-feecdc379e92 | Analog Temperature Reading | `READ`
| d8520577-81ed-478c-a3ad-a810d65c064a | Binary Switch | `READ`
| 638cc58f-0c58-4f7a-ab38-0df7aff5e1f3 | LED | `READ` `WRITE`
| be6b91f2-86ef-4426-807e-2a2e6e67e29d | Text in String | `READ` `WRITE`

## Flowchart
-- Todo