void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
  if (Serial.available()) {
    // Serial(PC) to Serial2(LTE-Module)
    int inByte = Serial.read();
    Serial2.write(inByte);
  }
  if (Serial2.available()) {
    // Serial2(LTE-Module) to Serial(PC)
    int inByte = Serial2.read();
    Serial.write(inByte);
  }
}