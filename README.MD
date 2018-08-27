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
```
CTRL_COMMAND_FORCE_STOP 0xAA
CTRL_COMMAND_SET_TARGET_POS 0x60
```

## Status Service - 0x5E1F - aka SELF
### Characteristic - 0xFEED - aka... FEED

Allows to read and to subscribe for current controller state. Value of this characteristic is updated:
- on device initiliazed (after boot/restart)
- on new peer connected
- on internal status changed but with not guarantee of being most recent state

#### Values
Particular values are transmitted in little endian order.
|Bits|Value|
:-: |:-
**0 - 3** | `int32` currentPosition (µm)
**4 - 7** | `int32` targetPosition (µm) 
**8** | `uint8` movementState (Enum)
