#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <LittleFS.h>

//setup RGB outputs. project is using a common cathode RGB LED
int RED=D6;
int GREEN=D7;
int BLUE=D8;

//couple variables for storing current conditions
int weatherNow = 0;
String iconNow="";
String weatherDesc="";

//read file contents
String readFile(fs::FS &fs, const char * path){
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    //Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  //Serial.println(fileContent);
  return fileContent;
}
//write to file
void writeFile(fs::FS &fs, const char * path, String message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

String server1="api.openweathermap.org";
String latCode="";
String lonCode="";
String apiKey="";

unsigned long lastConnectionTime = 10 * 60 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postInterval = 3 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)

WiFiClient wifiClient;

void setup() {
  Serial.begin(115200);

  //initialize littlefs
  if(!LittleFS.begin()){  //try to initialize file system for stored values
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
    // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  server.begin();

  //setup outputs
  pinMode(RED,OUTPUT);
  pinMode(GREEN,OUTPUT);
  pinMode(BLUE,OUTPUT);

  //get IP address and flash led so end user can see IP to get portal
  String IPAdd = WiFi.localIP().toString();
  int p1 =IPAdd.indexOf(".");
  int p2 = IPAdd.indexOf(".",p1+1);
  int p3 = IPAdd.indexOf(".",p2+1); //p3 will be the starting place of the last part of IP
  int d1=IPAdd.substring(p3+1,p3+2).toInt();
  int d2=IPAdd.substring(p3+2,p3+3).toInt();
  int d3=IPAdd.substring(p3+3,p3+4).toInt();
  //flash Red for the first digit
  for (int x=1; x<=d1; x+=1){
    analogWrite(RED,500);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);    
  }
  delay(500);
  //flash Green for the first digit
  for (int x=1; x<=d2; x+=1){
    analogWrite(RED,0);
    analogWrite(GREEN,500);
    analogWrite(BLUE,0);
    delay(1000);
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);    
  }
  delay(500);
  //flash Blue for the first digit
  for (int x=1; x<=d3; x+=1){
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,500);
    delay(1000);
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);    
  }
  
  
}

void loop() {   //main program
  //load LAT, LON, and API on every loop
  String latCode=readFile(LittleFS,"/lat.txt");
  String lonCode=readFile(LittleFS,"/long.txt");
  String apiKey=readFile(LittleFS,"/api.txt");

  String errorMessage="";
  String payload="";

  WiFiClient client = server.available();   // Listen for incoming clients
   if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if(header.indexOf("GET /config.html?") >= 0){ //if the header contains /config.html
              int A= header.indexOf("SetLat=");  //find start of LAT setting
              int B= header.indexOf("&SetLon="); //find start of LON setting
              String SetLat = header.substring(A+7,B); //substring to get just the lat
              Serial.print("Lat = ");
              Serial.println(SetLat);
              writeFile(LittleFS, "/lat.txt", header.substring(A+7,B)); //write to lat.txt
              A = header.indexOf("&SetAPI=");  //find start of API
              String SetLon = header.substring(B+8,A); //substring the longitude
              Serial.print("Long = ");
              Serial.println(SetLon);
              writeFile(LittleFS, "/long.txt", SetLon);  //write to long.txt
              String SetAPI= header.substring(A+8,A+8+32); //get API key, this is assuming key is 32 characters
              Serial.print("API = ");
              Serial.println(SetAPI);
              writeFile(LittleFS, "/api.txt", SetAPI); //save to api.txt
            } else { Serial.println("No Data");}
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            //css was borrowed from another project don't need buttons but left css alone for now
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            //html content and form
            client.println("<body><h1>ESP Weather Station</h1>");
            //grab image icon from openweather
            client.println("<img src=\"http://openweathermap.org/img/wn/");
            client.println(iconNow);
            client.println(".png\" width=\"100\" height=\"100\">");
            client.print("<p>current: ");
            client.print(weatherNow);
            client.print("-");
            client.print(weatherDesc);
            client.print("(");
            client.print(iconNow);
            client.println(")</p>");
            client.println("<form action=\"/config.html\" method=\"get\">");
            client.println("<label for=\"SetLat\">Latitude:</label>");
            client.println("<input type=\"text\" id=\"SetLat\" name=\"SetLat\" value=\"");
            client.println(latCode); //load current value into box
            client.println("\"><br><br>");
            client.println("<label for=\"SetLon\">Longitude:</label>");
            client.println("<input type=\"text\" id=\"SetLon\" name=\"SetLon\" value=\"");
            client.println(lonCode); //load current value into box
            client.println("\"><br><br>");
            client.println("<label for=\"SetAPI\">OpenWeather API Key:</label>");
            client.println("<input type=\"text\" id=\"SetAPI\" name=\"SetAPI\" value=\"");
            client.println(apiKey); //load current value into box
            client.println("\"><br><br>");
            client.println("<input type=\"submit\" value=\"Submit\">");
            client.println("</form>");
            client.println("</body></html>");
            client.println();
            // Break out of the while loop
            break;
            } else { // if you got a newline, then clear currentLine
            currentLine = "";
            }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  //delay to prevent exceeding openweather poling requirement for free account
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println("making webcall");
        if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.useHTTP10(true);
    //Serial.print("http://"+server+"/data/2.5/forecast/hourly?q="+cityCode+"&APPID="+apiKey);
    //http.begin(wifiClient,"http://"+server+"/data/2.5/forecast/hourly?q="+cityCode+"&APPID="+apiKey);  //Specify request destination
    http.begin(wifiClient,"http://"+server1+"/data/2.5/onecall?lat="+latCode+"&lon="+lonCode+"&exclude=hourly,minutely,daily,alerts&appid="+apiKey);
    int httpCode = http.GET();                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      if (httpCode != 200){
        //Bad Response Code
        errorMessage = "Error response (" + String(httpCode) + "): " + payload;
        Serial.println(errorMessage);
        http.end();
        return;  
      } else {
          StaticJsonDocument<768> doc; //json parsing was created using the arduinojson tool
          DeserializationError error = deserializeJson(doc, http.getStream());
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
          }
          JsonObject current = doc["current"];
          JsonObject current_weather_0 = current["weather"][0];
          int current_weather_0_id = current_weather_0["id"]; // 803
          const char* current_weather_0_main = current_weather_0["main"]; // "Clouds"
          const char* current_weather_0_description = current_weather_0["description"]; // "broken clouds"
          const char* current_weather_0_icon = current_weather_0["icon"]; // "04d"
          iconNow=current_weather_0_icon;
          weatherNow=current_weather_0_id;
          weatherDesc=current_weather_0_description;
          Serial.print("current: ");
          Serial.print(weatherNow);
          Serial.print("-");
          Serial.print(weatherDesc);
          Serial.print("(");
          Serial.print(iconNow);
          Serial.println(")");
          http.end();   //Close connection
      }
    } else { 
      Serial.println("HTTP Failure"); 
      http.end();
    }
    }
  }
  //override weather for testing
  //weatherNow = 771;
  //iconNow = "50d";
  //for MOST conditions I used the icon to determinw what to display, origninally I was using the condition code but it was too confusing to understand that the flashing speed related to amount of rain fall.
  if (weatherNow ==771 || weatherNow == 781 ){
    //Tornado just flashes red on and off
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);
    analogWrite(RED,1024);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(1000);
  } else if ( weatherNow >= 611 && weatherNow <= 616) { //611-616 are snow + rain conditions
    BWPulse();
  } else if ( weatherNow == 511) { //511 is freezing rain
    BWPulse();
    //I found that I didn't want light on for "good" weather, I left code incase you want it
  //} else if ( iconNow == "01d") {
  //  analogWrite(RED,500);
  //  analogWrite(GREEN,500);
  //  analogWrite(BLUE,0); 
  //  delay(10000); 
  //} else if ( iconNow == "02d" || iconNow == "03d") {
  //  YGPulse();
  //} else if (iconNow == "04d") {
  //  analogWrite(RED,100);
  //  analogWrite(GREEN,100);
  //  analogWrite(BLUE,100);
  //  delay(1000);
  } else if (iconNow == "09d") { //09d is the rain icon
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,250); 
    delay(10000); 
  } else if (iconNow == "09n") { //09n is rain during the night I have the blue come on but with a bit less intensity
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,100); 
    delay(10000); 
  } else if (iconNow == "10d") { //10d and 10n are scattered rain so it flashes blue yellow
    BYPulse();
  } else if (iconNow == "10n") {
    BPulse();
  } else if (iconNow == "11d" || iconNow == "11n") {
    Thunder();
  } else if (iconNow == "13d" || iconNow == "13n"){
    Snow();
  } else if (iconNow == "50d" ) {
    weatherWarning();
  }
  else { //if icon is not configured turn off led
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0); 
    delay(1000); 
  }
}

void weatherWarning(){ //goes from a pale yellow to red/purple colour
  analogWrite(RED,150);
  analogWrite(GREEN,150);
  analogWrite(BLUE,0);
  delay(950);
  //loop from 150-50 step 25
  for (int x=150; x>=50; x-=25){
    analogWrite(RED,150);
    analogWrite(GREEN,x);
    analogWrite(BLUE,0);
    delay(50);
  }
  delay(200);
  //loop from 50-150 step 25
  for (int x=50; x<=150; x+=25){
    analogWrite(RED,150);
    analogWrite(GREEN,x);
    analogWrite(BLUE,0);
    delay(50);    
  }
}
void Snow() { //snow pusles white
    analogWrite(RED,250);
    analogWrite(GREEN,250);
    analogWrite(BLUE,250); 
    delay(950);
    //loop from 250-50 step 25
    for (int x=250;x>=50; x-=25){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,x); 
      delay(50);
    }
    delay(950);
    //loop from 50-250 step 25
    for (int x=50;x<=250; x+=25){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,x); 
      delay(50);
    }
}
void Thunder(){  //lightening holds blue for 10 sec then does 2 very quick flashes
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,500); 
    delay(10000);
    analogWrite(RED,50);
    analogWrite(GREEN,50);
    analogWrite(BLUE,500); 
    delay(50);
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,500); 
    delay(150);
    analogWrite(RED,50);
    analogWrite(GREEN,50);
    analogWrite(BLUE,500); 
    delay(50);
}
void BPulse(){ //for rain this just pulses a blue light
  analogWrite(RED,0);
  analogWrite(GREEN,0);
  analogWrite(BLUE,250);
  delay(950);
  //loop 0-250 step 25
  for (int x=0; x<=250; x+=25){
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,250-x);
    delay(50);    
  }
  delay(950);
  //loop 250-0 step 25
  for (int x=0; x<=250; x+=25){
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,x);
    delay(50);    
    
  }
}

void BYPulse(){  //blue/yellow pulsing
  analogWrite(RED,0);
  analogWrite(GREEN,0);
  analogWrite(BLUE,250);
  delay(950);
  //loop 0-250 step 25
  for (int x=0; x<=250; x+=25){
    analogWrite(RED,x);
    analogWrite(GREEN,x);
    analogWrite(BLUE,250-x);
    delay(50);    
  }
  delay(950);
  //loop 250-0 step 25
  for (int x=0; x<=250; x+=25){
    analogWrite(RED,250-x);
    analogWrite(GREEN,250-x);
    analogWrite(BLUE,x);
    delay(50);    
    
  }
}
void GPulse(){ //grey pulsing
    analogWrite(RED,100);
    analogWrite(GREEN,100);
    analogWrite(BLUE,100);
    delay(950);
    //loop go from 100 - 0 step 5, delay 50
    for (int x=100; x>=0;x-=5){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,x);
      delay(50);
    }
    delay(950);
    //loop go from 0 - 100 step 5, delay 50)
    for (int x=0; x<100; x+=5) {
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,x);
      delay(50);  
    }
}

void YGPulse(){ //this pulses yellow to grey
    analogWrite(RED,500);
    analogWrite(GREEN,500);
    analogWrite(BLUE,0);
    delay(950);
    //loop go from 500-250 step 25 delay 50
    for (int x=500; x>225; x-=25){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,50);
      delay(50);
      //Serial.println(x);
    }
    delay(950);
    //loop go from 250-500 step 25 delay 50
    for (int x=250; x<500; x+=25){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,50);
      delay(50);
      //Serial.println(x);
    }
    
}
void BWPulse(){  //pulsing from blue to bright white
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,500); //500
    delay(950);
    //loop to go from 0-500 step 50 delay 50
    for (int x=0; x<500; x+=50){
      analogWrite(RED,x);
      analogWrite(GREEN,x);
      analogWrite(BLUE,500);
      delay(50);
    }
    delay(950);
    //loop to go from 500-0 step 100 delay 50
    for (int x=600; x>1; x-=50){
      analogWrite(RED,x-100);
      analogWrite(GREEN,x-100);
      analogWrite(BLUE,500);
      delay(50);
    }
    
}



//https://openweathermap.org/weather-conditions
//01d / 01n - Clear Sky
          //01d = bright yellow
          //01n = off
//02d / 02n - Few Clouds
          //02d = yellow pulse
          //02n = off
//03d / 03n - Scattered Clouds
          //03d = grey
          //03n = off
//04d / 04n - Broken Clouds
          //04d = grey pulse black
          //04n = off
//09d / 09n - Shower Rain
          //09d = blue
          //09n = blue
//10d / 10n - Rain/sun
          //10d = Blue pulse yellow
          //10n = Blue pulse off
//11d / 11n - Thuderstorms
          //11d = Blue flash white
          //11n = Blue flash white
//13d / 13n - Snow
          //13d = white pulse
          //13n = white pulse
//50d / 50n - mist/fog
          //50d = Yellow pulse to red/purple
          //15n = off
//771 & 781 - Extreme weather, flash red
//611-616 - sleet/rain = blue white pulse
//511 - freezing rain = blue white pulse
