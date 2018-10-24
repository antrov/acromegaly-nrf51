**Acromegaly nRF51**
===

Simple system for remote control of desk height (UART+BLE)

# Setup
TODO

# Logging
Logging is supplied by utils library provided by Nordic SDK (utils/nrf_log). Project can be configured to use either UART and/or SeggerRTT. To make selection, set the proper flag in a Makefile. 

## JLink
*-DNRF_LOG_USES_RTT=1*
```bash
JLinkExe -device nRF51422 -speed 4000 -if SWD
JLinkRTTClient
```

## UART
*-DNRF_LOG_USES_UART=1*
```bash
screen /dev/tty.usbserial-A602JN7W 115200
```

# BLE Services

All BLE items has 4-letter-hex UUIDs alluding to their destination (for better reminding).

## Control Service - 0x1EAD - aka LEAD

Responsible for receiving control instructions, like target position, move direction or switch state.

### Characteristic - 0x5010 - aka SOLO
Position values should be send in little endian order (0xDD04 -> 1245 mm)

#### Stop - 0xAA
Command for stop current movement
|Bytes|Value|
:-: |:-
**0** | `uint8` 0xAA

#### Go to target position - 0x60
Command for stop current movement
|Bytes|Value|
:-: |:-
**0** | `uint8` 0x60
**1 - 2** | `uint16` targetPosition (mm)

#### Go to extremum position - 0x88
Initiates movement with target position out of possible bounds. When 
|Bytes|Value|
:-: |:-
**0** | `uint8` 0x88
**1** | `uint8` extremumType (Enum)

where extremumType is:  
>`0xDD` Bottom extremum  
>`0xFF` Upper extremum

## Status Service - 0x5E1F - aka SELF
### Characteristic - 0xFEED - aka... FEED

Allows to read and to subscribe for current controller state. Value of this characteristic is updated:
- on device initiliazed (after boot/restart)
- on new peer connected
- on internal status changed but with not guarantee of being most recent state

#### Values
Particular values are transmitted in little endian order.
|Bytes|Value|
:-: |:-
**0 - 3** | `int32` currentPosition (µm)
**4 - 7** | `int32` targetPosition (µm) 
**8** | `uint8` movementState (Enum)

where movementState enum is:  
>`0xA1` None movement  
>`0x92` Moves down  
>`0xCB` Moves up