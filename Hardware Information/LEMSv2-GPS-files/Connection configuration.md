## Required Items
  - LEMSv2 and Sparkfun ATSAMD21 DEV board
  - wire or dev cables
  - bread board
  - any GPS module compatable with arduino

## How to connect

  - connect the TX pin on GPS module to pin A2 on SAMD21 board
  - connect VCC on GPS module to 3.3V pin on SAMD21 board (voltage regulator needs to be sized) and incorperated here the code is already written for a voltage regulator to be plugged into D4 pin..
  - connect GND to GND
  
## How to start
  - plug in USB cord
  - press the reset button twice on the board
  - upload GPS_standalone_Revisions_V3.ino
