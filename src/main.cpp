#include <DHT.h> // library voor de sensor
#include <WiFi.h>// library voor verbinding met het netwerk
#include <PubSubClient.h>// library voor het MQTT protocoll
#include <Wire.h>// library voor de verbinding met I2C
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>//library voor de OLED-display

#define DHTPIN 2 //pin waar de sensor is aangesloten
#define DHTTYPE DHT11 //de sensor is een dht11 sensot
DHT dht(DHTPIN, DHTTYPE);

//gegevens voor te verbinden met de hotspot en het ip adress waar de mqtt server op draait

const char* ssid = "brimspot";
const char* password = "datacomm123";
const char* mqtt_server = "192.168.43.199";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char temper[50]; //variabele voor de temperatuur
char hum[50];   //variabele voor de vochtigheid
int value = 0;

//definieeren van de oled display grote en de verbinding met de I2C dit is op pin 21(SDA) en 22(SCK)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//verbinden met het wifi netwerk de esp zoekt ernaar en probeert te verbinden.
void setup_wifi() {
  //laat het progress op het oled display zien
  delay(10);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 10);
  oled.println("verbinding maken met wifi...");
  oled.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 10);
  oled.println("WiFi Verbonden");
  oled.display();
}
//herconnecten met de MQTT server moest dit mislukt zijn
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); //dit is de baud-rate van de seriele poort
  dht.begin(); //start met lezen van de data
  
//maakt de setup van het OLED-dispay
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }

  delay(2000);
  setup_wifi();
  //maakt verbinding met de mqtt server op de poort 1883
  client.setServer(mqtt_server, 1883);//
}

void loop(){
  //kijkt of de verbinding met mqtt is gelukt zo niet voert het deze code uit
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    float temp = dht.readTemperature(); //zet de data van de temperatuur in een float
    float humidity = dht.readHumidity(); //zet de data van de vochtigheid in een float
    oled.clearDisplay(); //maakt het oled display blank
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(0, 10);
    oled.print("Temperatuur:");
    oled.print(temp);
    oled.print((char)247); //char 247 is het graden symbool
    oled.println("C");

    oled.print("Vochtigheid:");
    oled.print(humidity);
    oled.println((char)37); //char 37 is het procent symbool
    oled.display();

    //code voor het doorsturen van de temperatuur data via mqtt het published naar de mqtt broker/server op de raspberry pi met als onderwerp "esp32/temp"
    snprintf(temper, 50, "%.2f", temp);
    Serial.print("Temperatuur: ");
    Serial.println(temper);
    client.publish("esp32/temp", temper);

    //zelfde code als hierboven maar voor de vochtigheid onder het odnerwerp "esp32/hum
    snprintf(hum, 50, "%.2f", humidity);
    Serial.print("Vochtigheid: ");
    Serial.println(hum);
    client.publish("esp32/hum", hum);
  }
}