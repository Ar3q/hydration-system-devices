#include <analogWrite.h>

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

int n1 = 26; //dir
int n2 = 27; //dir
int ena = 25; //controll

int n3 = 18; //dir
int n4 = 19; //dir
int enb = 5; //controll

const byte MAX_PWM_SPEED = 255;
const byte MIN_PWM_SPEED = 0;

bool liquidLevel = false;
int liquidSensorPin = 16;

int moisture[2];
int moistureThreshold[2] = { -1, -1};

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

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  String topicStr = String(topic);

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  String firstTopicElement = getElementOfTopic(topicStr, 0);

  Serial.print("1st: ");
  Serial.println(firstTopicElement);
  //  Serial.print("2nd: ");
  //  Serial.println(secondTopicElement);


  if (firstTopicElement == "devices") {
    String secondTopicElement = getElementOfTopic(topicStr, 1);
    String thirdTopicElement = getElementOfTopic(topicStr, 2);

    if (secondTopicElement == "pomps_device") {
      int pompPin = -1;

      // devices/pomps_device/(pomp1 || pomp2)
      if (thirdTopicElement == "pomp1") {
        pompPin = ena;
      } else if (thirdTopicElement == "pomp2") {
        pompPin = enb;
      }

      // devices/pomps_device/moisture_threshold/(moisture_device_1 || moisture_device_2)
      else if (thirdTopicElement == "moisture_threshold") {
        int moistureThresholdIndex = -1;
        String fourthTopicElement = getElementOfTopic(topicStr, 3);

        Serial.print("4th: ");
        Serial.println(fourthTopicElement);

        if (fourthTopicElement == "moisture_device_1") {
          moistureThresholdIndex = 0;
        } else if (fourthTopicElement == "moisture_device_2") {
          moistureThresholdIndex = 1;
        }


        /*
           threshold could be set to 0,
           toInt() would return 0 also when string doesn't start with '0', eg. 'a'
           so additional checking and setting to -1
        */
        int threshold = messageTemp.toInt();
        Serial.println("threshold: ");
        Serial.println(threshold);
        if (!threshold && messageTemp[0] != '0') {
          threshold = -1;
        }

        Serial.println("threshold: ");
        Serial.println(threshold);

        if (threshold >= 0) {
          moistureThreshold[moistureThresholdIndex] = threshold;
        } else {
          Serial.println("Not valid value");
          return;
        }

        Serial.println("moisture th: ");
        Serial.println(moistureThreshold[0]);
        Serial.println(moistureThreshold[1]);
      }
      
      // devices/pomps_device/liquid_level
      else if(thirdTopicElement == "liquid_level"){
        return;
      }

      else {
        Serial.println("Unknown pomp number or threshold of moisture");
        return;
      }

      Serial.print("pompPin: ");
      Serial.println(pompPin);

      if (pompPin != -1) {
        if (messageTemp == "on") {
          Serial.println("on");
          analogWrite(pompPin, MAX_PWM_SPEED);
        }
        else if (messageTemp == "off") {
          Serial.println("off");
          analogWrite(pompPin, MIN_PWM_SPEED);
        }
      }
    }
    // devices/(moisture_device_1 || moisture_device_2)/moisture
    else if (secondTopicElement.startsWith("moisture_device")) {
      int moistureIndex = -1;

      if (secondTopicElement == "moisture_device_1") {
        moistureIndex = 0;
      } else if (secondTopicElement == "moisture_device_2") {
        moistureIndex = 1;
      }

      String thirdTopicElement = getElementOfTopic(topicStr, 2);
      Serial.println("3rd:");
      Serial.println(thirdTopicElement);

      if (thirdTopicElement == "moisture") {
        moisture[moistureIndex] = messageTemp.toInt();
      } else {
        Serial.println("Unknown measurement");
        return;
      }

      Serial.println("moisture: ");
      Serial.println(moisture[0]);
      Serial.println(moisture[1]);
    }
  }


}

String getFirstElementOfTopic(String topic) {
  int indexOfSlash = topic.indexOf("/");
  return topic.substring(0, indexOfSlash);
}

String getElementOfTopicOtherThanTheFirstOne(String topic, int elementNumber) {
  int indexOfSlash = topic.indexOf("/");
  String subtopic = topic.substring(indexOfSlash + 1);
  int actualElement = 1;

  while (actualElement != elementNumber) {
    indexOfSlash = subtopic.indexOf("/");
    subtopic = subtopic.substring(indexOfSlash + 1);
    actualElement++;
  }

  int indexOfNextSlash = subtopic.indexOf("/");

  if (indexOfNextSlash) {
    subtopic = subtopic.substring(0, indexOfNextSlash);
  }

  return subtopic;
}

String getElementOfTopic(String topic, int elementNumber) {
  if (!elementNumber) {
    return getFirstElementOfTopic(topic);
  }

  return getElementOfTopicOtherThanTheFirstOne(topic, elementNumber);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("pompsDevice")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("devices/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void automaticWatering() {
  if (moistureThreshold[0] > 0) {
    automaticWateringBy(ena, 0);
  }
  if (moistureThreshold[1] > 0) {
    automaticWateringBy(enb, 0);
  }
}

void automaticWateringBy(int pompPin, int moistureIndex) {
  if (moisture[moistureIndex] > moistureThreshold[moistureIndex]) {
    Serial.println("on");
    analogWrite(pompPin, MAX_PWM_SPEED);
  } else {
    Serial.println("off");
    analogWrite(pompPin, MIN_PWM_SPEED);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);


  pinMode(n1, OUTPUT);
  pinMode(n2, OUTPUT);
  pinMode(ena, OUTPUT);

  pinMode(n3, OUTPUT);
  pinMode(n4, OUTPUT);
  pinMode(enb, OUTPUT);

  analogWrite(ena, MIN_PWM_SPEED);
  digitalWrite(n1, HIGH);
  digitalWrite(n2, LOW);

  analogWrite(enb, MIN_PWM_SPEED);
  digitalWrite(n3, LOW);
  digitalWrite(n4, HIGH);

  pinMode(liquidSensorPin, INPUT);
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

    liquidLevel = digitalRead(liquidSensorPin);
    Serial.print("liquidLevel: ");
    Serial.println(liquidLevel);

    char tempString[8];
    dtostrf(liquidLevel, 1, 0, tempString);
    client.publish("devices/pomps_device/liquid_level", tempString);
  }

  automaticWatering();
}
