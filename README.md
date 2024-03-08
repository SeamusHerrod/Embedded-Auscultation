#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

DEVICE NAME: XIAO_ESP32S3


Instructions for Arduino IDE: 

launch IDE and connect XIAO ESP32S3 
- add board using dropdown menu near top left of the page (next to the verify, upload, and debug buttons)
    - if the board is not automatically detected, search for XIAO_ESP32S3
- make sure that the correct port is selected, this will be different for every machine, will most likely be something that starts with "dev/cu"

- attempt to verify the code, it will give you errors about missing libaries, install those it says are missing
  - will definitely need the WifiNINA libary

- once libaries are installed, click the upload button for the code to be compiled and loaded onto the chip,
- after the code is uploaded, the program will begin running
- 
