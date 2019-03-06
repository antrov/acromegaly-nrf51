# Acromegaly nRF51

Acromegaly nRF51 is an embedded controller for the adjustable height desk. Access to controller is available through BLE interface by the [iOS client app](https://github.com/antrov/acromegaly-ios)

This resposity is a part of the [Acromegaly project](https://github.com/antrov/acromegaly) - Open Source desk with adjustable height.

# Build

Code and structure of this repository bases on a BLE advertising template provided by Nordic. In sake of simplicity, folders for Keil uVision project and for other MCUs was removed.

## Prerequisites

* GNU Make 3.8+
* nRF5 SDK 11.0.0 (89a8197)
* nrfjprog 9.7.0+
* nRF51822 or nRF51422 MCU (ie. [nRF51 DK Board](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF51-DK))
* correct MCU [wiring](https://github.com/antrov/acromegaly)

## Makefile

Setup absolute path to nRF5 SDK 11 in `pca10028/s130/armgcc/Makefile`:

```Makefile
SDK_ROOT := /Users/johndoe/nRF5_SDK_11.0.0_89a8197
```

That its, project is ready to build and flash along with Softdevice:

```bash
cd pca10028/s130/armgcc/
make build
make flash_softdevice
make flash
```

Depending of applied connections to MCU and logger configuration, the new BLE advertiser should appear. 

In the opposite case, validate connection between nRF51 and Flash memory/UART/Segger according to the [Eagle files](https://github.com/antrov/acromegaly). 

Remind, if the Makefile has set `ASMFLAGS += -DNRF_LOG_USES_RTT=1`, then Segger SWDIO/SWCLK have to be connected. Otherwise the MCU would not boot.

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
Initiates movement with target position out of possible bounds. When movement is finished, value of extremum is as as current. This method is designed to reset position counter and minimize position detion error. 

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

## Contact
In case of new issues, concepts or just a will to say hello:

* antrof@gmail.com
* https://www.linkedin.com/in/handr/

## License 

Copyright 2019 Hubert Andrzejewski

Based on Nordic project and provided by this company license.

```
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE HEREBY
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; DAMAGE TO ANY DEVICE, LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE. SOME STATES DO NOT ALLOW 
THE EXCLUSION OF INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THE
ABOVE LIMITATIONS MAY NOT APPLY TO YOU.
```