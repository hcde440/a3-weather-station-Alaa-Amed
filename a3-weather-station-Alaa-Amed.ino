// ********************************************* \\
//                                               \\ 
//                WEATHER STATION                \\
//               Working with MQTT               \\
//                                               \\
// ********************************************* \\

// This Program incorporates some sensors to build a weather station using the DHT22 and MPL115A2 sensors on a breadboard

//Requisite Libraries . . .
#include <ESP8266WiFi.h> 
#include <SPI.h>
#include <Wire.h>  // for I2C communications
#include <Adafruit_Sensor.h>  // the generic Adafruit sensor library used with both sensors
#include <DHT.h>   // temperature and humidity sensor library
#include <DHT_U.h> // unified DHT library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPL115A2.h> // Barometric pressure sensor library
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //

// pin connected to DH22 data line
#define DATA_PIN 12
// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);
// create MPL115A2 instance
Adafruit_MPL115A2 mpl115a2;


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// digital pin 5
#define BUTTON_PIN 2

// button state
bool current = false;
bool last = false;


#define wifi_ssid ""  
#define wifi_password "" 


#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Potentiometer connected to analog pin 0
#define POTPIN A0
// Variable to store potentiometer values 
int potValue = 0;

WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array
unsigned long currentMillis, timerOne, timerTwo, timerThree, timerfour; //we are using these to hold the values of our timers

void setup() {
  // set button pin as an input
  pinMode(BUTTON_PIN, INPUT);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  mpl115a2.begin();
  display.display();
  // start the serial connection
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
  timerOne = timerTwo = timerThree = millis();

  // wait for serial monitor to open
  while(! Serial);

  // initialize dht22
  dht.begin();
}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // Start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}  

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("Alaa/+"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/////LOOP/////
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  // Read data from sensor: potentiometer values every two seconds
  if (millis() - timerOne > 2000) {
    // Grab the current value of the potentiometer
    potValue = analogRead(POTPIN);

    sprintf(message, "{\"Pot_value\":\"%d%\"}", potValue); // %d is used for an int
    // Print the value to the serial monitor
    //Serial.println(potValue);
    //publish message  
    mqtt.publish("Alaa/Pot", message);
    timerOne = millis();
  }

  // Read data from sensor: DHT22 values every 4 seconds
  if (millis() - timerTwo > 4000) {
    sensors_event_t event;
    dht.temperature().getEvent(&event);

    float temp = event.temperature;
    float fahrenheit = (temp * 1.8) + 32;
    
    dht.humidity().getEvent(&event);
    float hum = event.relative_humidity;
     
    
    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_hum[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character

    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(temp, 5, 2, str_temp);
    //ditto
    dtostrf(hum, 5, 2, str_hum);
    // Publish message to the tem/hum topic 
    sprintf(message, "{\"temp\":\"%s\", \"hum\":\"%s\"}", str_temp, str_hum);
    mqtt.publish("Alaa/tempHum", message);
    timerTwo = millis();

    // Cast all float values into string so it can pe printed on the OLED module
    String temp1 = String(temp); 
    String hum1 = String(hum);
    // Clear the screen
    display.clearDisplay();
    // Normal 1:1 pixel scale
    display.setTextSize(1);
    // Start at top-left corner
    display.setCursor(0, 0);
    // Draw white text
    display.setTextColor(WHITE);
    // Display Humidity on OLED
    display.println("Humidity: " + hum1);
    // Display Temperature on OLED
    display.println("Temperature: " + temp1 + " C");
    // Update the screen-- without this command, nothing will be pushed/displayed 
    display.display();
    // wait 2 seconds (2000 milliseconds == 2 seconds)
    delay(2000);
    display.clearDisplay();
    //Serial.println("%");
  }

  // Read data from sensor: MPL115A2 values every 6 seconds
  if (millis() - timerThree > 6000) {;
    float pressure = 0; 
    pressure = mpl115a2.getPressure();
    sprintf(message, "{\"pressure\" : \"%d\"}", pressure); // %d is for int
    // Publish message to the pressure topic 
    mqtt.publish("Alaa/pressure", message);
    timerThree = millis();
  }
  // Read data from push button every 10 seconds
  if (millis() - timerfour > 10000) {
    if (digitalRead(BUTTON_PIN) == LOW)
        current = true;
        else
        current = false;
    sprintf(message, "{\"buttonState\" : \"%d\"}", current); // %d is used for a bool as well
    mqtt.publish("Alaa/switch", message);
    timerfour = millis();
    last = current;
  }
  
}//end Loop

/////CALLBACK/////
//The callback is where we attach a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "theTopic/LBIL") == 0) {
    Serial.println("A message from Batman . . .");
  }

  else if (strcmp(topic, "theTopic/tempHum") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  else if (strcmp(topic, "theTopic/switch") == 0) {
    Serial.println("The switch state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
