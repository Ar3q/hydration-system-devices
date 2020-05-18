#include <analogWrite.h>

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "***";
const char* password = "***";

const char* mqtt_server = "xxx.xxx.xxx.xxx";
const int mqtt_port = xxxx;

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

int moisture[2] ;
int moistureThreshold[2] = {0, 0};

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
  String secondTopicElement = getElementOfTopic(topicStr, 1);

  Serial.print("1st: ");
  Serial.println(firstTopicElement);
  Serial.print("2nd: ");
  Serial.println(secondTopicElement);

  if (firstTopicElement == "pomps_device") {
    int pompPin = -1;

    if (secondTopicElement == "pomp1") {
      pompPin = ena;
    } else if (secondTopicElement == "pomp2") {
      pompPin = enb;
    }

    else if (secondTopicElement == "moistureThreshold") {
      int moistureThresholdIndex = -1;
      String thirdTopicElement = getElementOfTopic(topicStr, 2);

      Serial.print("3rd: ");
      Serial.println(thirdTopicElement);

      // returns 0 if no valid conversion could be performed
      int deviceNumber = thirdTopicElement.toInt();

      if (deviceNumber > 0 && deviceNumber <= 2) {
        moistureThresholdIndex = deviceNumber - 1;
      } else {
        Serial.println("Unknown threshold of moisture indenfier");
        return;
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
  } else if (firstTopicElement == "moisture_devices") {
    int moistureIndex = -1;

    // returns 0 if no valid conversion could be performed
    int deviceNumber = secondTopicElement.toInt();

    if (deviceNumber > 0 && deviceNumber <= 2) {
      moistureIndex = deviceNumber - 1;
    } else {
      Serial.println("Unknown device number");
      return;
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
      client.subscribe("pomps_device/#");
      client.subscribe("moisture_devices/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
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
  Serial.println("loop()");
  delay(500);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
    if (now - lastMsg > 10000) {
      lastMsg = now;
  
      liquidLevel = digitalRead(liquidSensorPin);
      Serial.print("liquidLevel: ");
      Serial.println(liquidLevel);
  
      char tempString[8];
      Serial.print("tempString char: ");
      Serial.println(tempString);
      dtostrf(liquidLevel, 1, 0, tempString);
      Serial.print("Message: ");
      Serial.println(tempString);
      client.publish("pomps_device/liquid_level", tempString);
    }

}
