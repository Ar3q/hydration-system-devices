#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "***";
const char* password = "***";

const char* mqtt_server = "***.***.***.***";
const int mqtt_port = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int sensorPin = A0;
int output;

const char* topic = "devices/moisture_device_1/moisture";
//const char* topic = "devices/moisture_device_2/moisture";

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("moisture_device_1")) {
//    if (client.connect("moisture_device_2")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  delay(5000);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;

    output = analogRead(sensorPin);
    Serial.print("Mositure: ");
    Serial.print(output);

    char tempString[8];
    dtostrf(output, 1, 0, tempString);
    
    client.publish(topic, tempString);
  }
}
