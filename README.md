# ESP8266 Temperature Monitor
Using the ESP8266 and a DHT22 or DHT11 we can log temperatures by MQTT and/or directly view the temperatures from the esp through a web browser.

The web interface is based on bootstrap but due to the limited space on the esp the css, js and fonts need to be hosted on a seperate server

## Setup:
This setup requires a MQTT Broker and a Web server ideally on the same machine.

The web server is used as a repository for the esp to point to serve css and js files for the web interface

**For the server:**

To setup simply copy the esp folder to your web server

The MQTT Broker is for the esp to log temperature and humidity data.
Setup of the broker will not be covered here

**For the esp:**

Adjust the house_temp.ino file to give the device a name and set your wifi said and password


Once set from a mDNS compatible device ESPNAME.local will load the web interface
