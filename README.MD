# Acromegaly nRF51

Simple system for remote control of desk height (UART+BLE)

## Setup
TODO

## Logging
Logging is supplied by utils library provided by Nordic SDK (utils/nrf_log). Project can be configured to use either UART and/or SeggerRTT. To make selection, set the proper flag in a Makefile. 

### JLink
*-DNRF_LOG_USES_RTT=1*
```bash
JLinkExe -device nRF51422 -speed 4000 -if SWD
JLinkRTTClient
```

### UART
*-DNRF_LOG_USES_UART=1*
```bash
screen /dev/tty.usbserial-A602JN7W 115200
```

## BLE Services

All BLE items has 4-letter-hex UUIDs alluding to their destination (for better reminding).

### Control Service - 0x1EAD - aka LEAD

Responsible for receiving control instructions, like target position, move direction or switch state.

#### Characteristic - 0x5010 - aka SOLO
```
CTRL_VALUE_MOTOR_MOVEMENT       0x01
CTRL_VALUE_MOTOR_POSITION       0x02
CTRL_VALUE_POWER_SWITCH         0x03
```

### Status Service - 0x5E1F - aka SELF

#### Characteristic - 0xFEED - aka... FEED

0,1 - Pos
2-3 - Target
4 - Direction
5 - Switch