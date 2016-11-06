/*
 * House temperature monitor
 * Author: Joshua Parker
 * 
 * Make sure to edit the ssid, password, ESPNAME and mqtt_*** values
 * before flashing to your esp8266
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP8266WebServer.h>

#define TEMPERATURE_IN_F false // changing to true gives temperature values in fahrenheit
#define MQTT_AUTH        true  // changing to false disables authentication for mqtt (make sure to set correct username and password when enabled)

const char* ssid = "NETWORK-SSID";
const char* password = "PASSWORD";

#define ESPNAME "ESP8266"
#define topic_path "demo/"    // when changing topic path keep the / at the end of the topic name 

#define mqtt_server "SERVER IP / NAME"
#define mqtt_port 1883                  // only needs to be changed if using custom port
#define mqtt_username "MQTT_USERNAME"   // only needs to be changed if MQTT_AUTH is true
#define mqtt_password "MQTT_PASSWORD"   // here also

#define humidity_topic topic_path ESPNAME "/humidity"
#define temperature_topic topic_path ESPNAME "/temperature"
#define heatindex_topic topic_path ESPNAME "/heatindex"

#define DHTPIN        0      // Pin the DHT Module is on
#define DHTTYPE       DHT22  // Define the DHT type DHT11 or DHT22

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
ADC_MODE(ADC_VCC); // for voltage checking

// for attached temperature "Modules" / sensors
class Module {
  // These maintain the current state
  float temperature;
  float humidity;
  float heatIndex;
  String roomName;
  
  private:
    String htmlElement(String headingSize, String label, String val, String measurement){
      return "<" + headingSize + "><small>" + label + ": </small>" + val + measurement + "</" + headingSize + ">";
    }
  
    String makeHtmlString() {
      return "<h1>" + roomName + "</h1>" 
           + htmlElement("h1", "Temperature", String(temperature, 2), (TEMPERATURE_IN_F? "&degF": "&degC"))
           + htmlElement("h3", "Humidity", String(humidity, 2), "%")
           + htmlElement("h3", "Heat Index", String(heatIndex, 2), (TEMPERATURE_IN_F? "&degF": "&degC"));
    }

  public:
    Module() {
      temperature, humidity = 0.0;
      heatIndex = 0.0;
      roomName = String(ESPNAME);
      roomName.replace("s-", "'s ");
      roomName.replace("-", " ");
    }

  String toString(){
    return makeHtmlString();
  }

  void setTemperature(float t){
    temperature = t;
  }
    
  void setHumidity(float h){
    humidity = h;
  }
    
  void setHeatIndex(float hi){
    heatIndex = hi;
  }

  String getTemperature(){
    return String(temperature);
  }
  
  String getHumidity(){
    return String(humidity);
  }
  
  String getHeatIndex(){
    return String(heatIndex);
  }
};

class WebPage {
  // REUSABLE HTML ELEMENTS
  String htmlTitle = "";
  String serverURL = "http://some.address/esp/";                  // change to location of your web server containing required css and js files
  String cssBootStrap = serverURL + "css/bootstrap.min.css";
  String cssScrollingNav = serverURL + "css/scrolling-nav.css";
  String jsHtml5shiv = "html5shiv.js";
  String jsRespond = "respond.min.js";
  String nl = "\r\n";
  String htmlStart = "<!DOCTYPE HTML>\r\n<html lang=\"en\">";
  String htmlEnd = nl + "</html>";
  String js[4] = { "jquery.js", "bootstrap.min.js", "jquery.easing.min.js", "scrolling-nav.js" };
  String webString;
  String roomName;
  
  private:
    String makeSectionHtml(String id, String content){
      return "<section id=\"" + id + "\" class=\"" + id + "-section\">"
             + "<div class=\"container\">"
             + "<div class=\"row\">"
             + "<div class=\"col-lg-12\">"
             + content
             + "</div>"
             + "</div>"
             + "</div>"
             + "</section>";
    }
    
    // make method for html <script></script> to reduce mem usage
    String makeScriptHtml(String url, String filename){
      return nl + "<script src=\"" + url + "js/" + filename + "\"></script>";
    }
    
    String makeJSLoad(String id){
      return "$('#" + id + "').load('" + id + "');";
    }
    
    String makeJSCall(String id, String delayFor){
      return nl + "$(document).ready(function() {"
           + makeJSLoad(id)
           + "setInterval(function() {"
           + makeJSLoad(id)
           + "}, " + delayFor + ");"
           + "});";
    }
    
    String makeWebPage(String htmlSection1, String htmlSection2){
      String html = htmlStart
                  + nl + "<meta charset=\"utf-8\">"
                  + nl + "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">"
                  + nl + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  + nl + "<meta name=\"description\" content=\"\">"
                  + nl + "<meta name=\"author\" content=\"\">"
                  + nl + "<title>" + htmlTitle + "</title>"
                  + nl + "<link href=\"" + cssBootStrap + "\" rel=\"stylesheet\">"
                  + nl + "<link href=\"" + cssScrollingNav + "\" rel=\"stylesheet\">"
                  + nl + "<!--[if lt IE 9]>"
                  + makeScriptHtml(serverURL, jsHtml5shiv)
                  + makeScriptHtml(serverURL, jsRespond)
                  + nl + "<![endif]-->"
                  + makeScriptHtml(serverURL, js[0])
                  + nl + "<script type=\"text/javascript\">// <![CDATA["
                  + nl + "$.ajaxSetup({cache:true});"
                  + makeJSCall("sensor", "120000")
                  + makeJSCall("sys", "30000")
                  + nl + "// ]]>"
                  + nl + "</script>"
                  + nl + "</head>"
                  + nl + "<body id=\"page-top\" data-spy=\"scroll\" data-target=\".navbar-fixed-top\">"
                  + nl + "<nav class=\"navbar navbar-default navbar-fixed-top\" role=\"navigation\">"
                  + nl + "<div class=\"container\">"
                  + nl + "<div class=\"navbar-header page-scroll\">"
                  + nl + "<button type=\"button\" class=\"navbar-toggle\" data-toggle=\"collapse\" data-target=\".navbar-ex1-collapse\">"
                  + nl + "<span class=\"sr-only\">Toggle navigation</span>"
                  + nl + "<span class=\"icon-bar\"></span>"
                  + nl + "</button>"
                  + nl + "<a class=\"navbar-brand page-scroll\" href=\"#page-top\">" + roomName + "</a>"
                  + nl + "</div>"
                  + nl + "<div class=\"collapse navbar-collapse navbar-ex1-collapse\">"
                  + nl + "<ul class=\"nav navbar-nav\">"
                  + nl + "<li class=\"hidden\">"
                  + nl + "<a class=\"page-scroll\" href=\"#page-top\"></a>"
                  + nl + "</li>"
                  + nl + "<li>"
                  + nl + "<a class=\"page-scroll\" href=\"#system\">System</a>"
                  + nl + "</li>"
                  + nl + "</ul>"
                  + nl + "</div>"
                  + nl + "</div>"
                  + nl + "</nav>";
      html += makeSectionHtml("intro", htmlSection1);
      html += makeSectionHtml("system", htmlSection2);
      for (int i = 1; i < (sizeof(js)/sizeof(String)); i++){ // using fixed value, for some reason sizeof() caused crash
        html += makeScriptHtml(serverURL, js[i]);
      }
      html += nl + "</body>"
            + htmlEnd;
      return html;
    }
    
  public:
    WebPage(){
      webString = "";
      roomName = String(ESPNAME);
      roomName.replace("s-", "'s ");
      roomName.replace("-", " ");
      htmlTitle = roomName + " Temperature";
    }
  
    void makeWebString(String sysPage){
      if(webString == ""){
        webString = makeWebPage("<div id=\"sensor\"><h2><small>loading...</small></h2></div>", sysPage);
      }
    }
  
    String toString(){
      return webString;
    }
  
    String failToString(){
      return htmlStart + "404 page not found" + htmlEnd;
    }
  
    String voltageToString(){
      return "<h3><small>Voltage: </small>" + String(ESP.getVcc() / 1000.00, 2) + "v</h3>";
    }
};

WebPage web;
Module weather;

// main paige
void handleRoot() {
  send_data(200, web.toString());
}

// is shown when user tries an invalid path
void handleNotFound() {
  send_data(404, web.failToString());
}

// update the system status
void handleSystem() {
  send_data(200, web.voltageToString());
}

// handle the temperature sensor data
void handleWeatherSensor(){
  send_data(200, weather.toString());
}

// send the data
void send_data(int type, String message) {
  server.send(type, "text/html", message);
  delay(100);
}

// reads the DHT sensor and saves the new values
void readDHT(){
  delay(200);
  float t = dht.readTemperature(TEMPERATURE_IN_F);
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  weather.setTemperature(t);
  weather.setHumidity(h);
  weather.setHeatIndex(dht.computeHeatIndex(t, h, TEMPERATURE_IN_F));
  Serial.println(String(t,2) + "*C");
  delay(2000);
}

// pass topic and message to publish an mqtt message
void publicMessage(char topic[], String message){
  client.publish(topic, message.c_str(), true);
}

// reconnect with mqtt server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (!MQTT_AUTH && client.connect(ESPNAME)) {
      Serial.println("connected");
    } else if (MQTT_AUTH && client.connect(ESPNAME, mqtt_username, mqtt_password)) {
      Serial.println("connected using auth");
    } else {
      Serial.println("failed, rc=");
      Serial.print(client.state());
      Serial.println(" wil try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long lastMsg = 0;

void sensorData(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  String oldTemperature = weather.getTemperature();
  String oldHumidity = weather.getHumidity();
  String oldHeatIndex = weather.getHeatIndex();
  
  if (now - lastMsg > 1000) {
    lastMsg = now;
    
    readDHT();
    if (oldTemperature != weather.getTemperature())
      publicMessage(temperature_topic, weather.getTemperature());

    if (oldHumidity != weather.getHumidity())
      publicMessage(humidity_topic, weather.getHumidity());
    
    if (oldHeatIndex != weather.getHeatIndex())
      publicMessage(heatindex_topic, weather.getHeatIndex());
    
    delay(3000);
  }
}

void setup() {
  Serial.begin(115200);
  // Connect to WiFi network and make sure it's in station mode only
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Set up mDNS responder
  if (!MDNS.begin(ESPNAME)) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("\n\rmDNS responder started");

  Serial.println("");
  Serial.println("ESP SMART CLIENT");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  
  readDHT();
  
  String sys = "<h1>System Information</h1>\r\n<h3><small>Connected to: </small>"+ String(ssid) +"</h3>"
             + "\r\n<h3><small>IP Address: </small>" + String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]) + "</h3>"
             + "\r\n<h3><small>MAC Address: </small>"+ WiFi.macAddress() +"</h3>"
             + "\r\n<div id=\"sys\"></div>\r\n<br /><br /><br />\r\n";
  web.makeWebString(sys);
  
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.on("/sensor", handleWeatherSensor);
  server.on("/sys", handleSystem);
  
  // start mqtt
  client.setServer(mqtt_server, mqtt_port);

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");
    
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  server.handleClient();
  sensorData();
}
