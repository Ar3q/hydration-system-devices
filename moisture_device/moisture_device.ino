int sensorPin = 27;
int output;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  pinMode(sensorPin, INPUT);
}

void loop() {
  output = analogRead(sensorPin);
//  output = map(output, 0, 4095, 0, 100);
  Serial.print("Mositure: ");
  Serial.print(output);
  Serial.println("");
  delay(1000);
}
