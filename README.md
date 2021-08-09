# ESP-Weather-Station
ESP Weather Station
## Overview
I wanted a simple weather station that can be visible across a room and give you a general idea of if it's raining or nice outside.
When looking for examples I found that many existing versions used old and depreciated code (i.e. arduninoJson v5), and that openweather api caused inconsistant performance due to memory issues.
This project uses WiFiManager to create an AP to configure your wifi, then uses a webform to allow you to enter your geolocation and an API key from openweather.
## Hardware
* ESP8266 board (I used a nodeMCU board from LoLin)
* Common Cathode RGB LED
* 3x 220Ohm Resistors (these are just the ones I had lying around feel free to use different ones as you feel neccessary they are just for a load to prevent the LPD from burning out)
* 3d Printed Parts (STL attached)
## Known Issues
Because this project does not use an aSync webserver the settings page may be unresponsive, it will load eventually it just needs to wait for the current loop cycle to complete.
## Wiring
Note: the 3d model provided is very tight for this project so I recommend soldering the resistor and LED to the board and not using wires between everything.
Connect the LED through the resistor to Red=D6, Green=D7, Blue=D8.
Connect the common cathode to ground.
## Operation
After you load the code onto the ESP it will create an access point called AutoConnectAP (see documentation here https://github.com/tzapu/WiFiManager, you will need to download this repo).
When the ESP connects to the wifi the LED will flash to indicate what the IP address is (it will flash for the last byte red is digit 1, green is digit 2, blue is digit 3). You will need to know your subnet mask. For example if your router is on subnet 192.168.0.0 and the IP address is 192.168.0.123 you will see 1 red, flash, 2 green and 3 blue.
Navigate to this IP address and you will see the config page (image in video directory).
If this is your first boot all fields will be blank and you will need to enter a lat/lon and API key. If you already have this configured you will see a weather icon for the curent weather and the code the API is returning.
## How the program works
This is a rough overview of the process incase troubleshooting is needed.
* each loop the system looks for a webclient.
* If a client is found the loop will investigate if the header contains the get command from the settings.
* Settings will be loaded to a file using LittleFS
* After settings are updated as needed the webpage will be displayed with the form
* If no client is found (or after the page has been displayed) the API call to openweather is made
* ArduinoJSON Assistant (https://arduinojson.org/v6/assistant/) was used to create the code to parse the stream (it's important to use the data stream instead of loading into memory because the available mem on an ESP isn't enough to load the whole stream into memory)
* After the weather is known (based on the icon code from open weather) the program will decide what flashing to display (listed below)
## What the flashing means
* Note that there are no lights for nice weather (I found it distracting so commented out)
* 01d / 01n - Clear Sky (01d = bright yellow, 01n = off)
* 02d / 02n - Few Clouds (02d = yellow pulse, 02n = off)
* 03d / 03n - Scattered Clouds (03d = grey, 03n = off)
* 04d / 04n - Broken Clouds (04d = grey pulse black, 04n = off)
* 09d / 09n - Shower Rain (09d = blue, 09n = blue)
* 10d / 10n - Rain/sun (10d = Blue pulse yellow, 10n = Blue pulse off)
* 11d / 11n - Thuderstorms (11d = Blue flash white, 11n = Blue flash white)
* 13d / 13n - Snow (13d = white pulse, 13n = white pulse)
* 50d / 50n - mist/fog (50d = Yellow pulsing red/purple, 15n = off)
* 771 & 781 - Extreme weather, flash red
* 611-616 - sleet/rain = blue white pulse
* 511 - freezing rain = blue white pulse
