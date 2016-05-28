# ESP8266 Temperature Monitor
Using the ESP8266 and a DHT22 or DHT11 we can log temperatures by MQTT or directly view the temperatures from the esp through a web browser.

SETUP:
This setup requires a MQTT Broker and a Web server ideally on the same machine
The web server is for the esp to point to for css and js files for the web interface

FOR the server
To setup simply copy the esp folder to your web server

The MQTT Broker is for the esp to log temperature and humidity data 
Setup of the broker will not be covered here

FOR the esp
Adjust the house_temp.ino file to give the device a name and set your wifi said and password

