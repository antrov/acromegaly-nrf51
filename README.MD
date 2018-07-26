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