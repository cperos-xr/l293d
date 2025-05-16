#define BAUD_CONSOLE 115200   // USB‐Serial
#define BAUD_MOTOR    9600    // UART on pins 0/1
#define MAX_INPUT    128      // input buffer size

void send_motor_packet(int8_t motors[8]) {
  uint8_t packet[10];
  packet[0] = 0xAA;
  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    packet[i+1] = (uint8_t)motors[i];
    checksum ^= packet[i+1];
  }
  packet[9] = checksum;
  Serial1.write(packet, sizeof(packet));
}

void setup() {
  Serial.begin(BAUD_CONSOLE);
  while (!Serial) ;            // wait for USB‐Serial on Leonardo/Micro
  Serial1.begin(BAUD_MOTOR);   // UART on pins 0 (RX1), 1 (TX1)

  Serial.println(F("Motor packet sender ready."));
  Serial.println(F("Type 8 numbers from -128 to 127 separated by spaces, then ENTER:"));
}

void loop() {
  static char line[MAX_INPUT];
  if (!Serial.available()) return;

  // --- read one line from USB console ---
  size_t len = Serial.readBytesUntil('\n', line, sizeof(line)-1);
  line[len] = '\0';

  // --- parse 8 signed values ---
  int8_t motors[8];
  char* tok = strtok(line, " ");
  int count = 0;
  while (tok && count < 8) {
    int v = atoi(tok);
    if (v < -128) v = -128;
    if (v >  127) v =  127;
    motors[count++] = (int8_t)v;
    tok = strtok(NULL, " ");
  }
  if (count < 8) {
    Serial.println(F("❌ Please enter exactly 8 values."));
    return;
  }

  // --- send packet over UART1 ---
  send_motor_packet(motors);

  // --- log what we sent ---
  Serial.print(F("✅ Packet sent: "));
  for (int i = 0; i < 8; i++) {
    Serial.print(motors[i]);
    Serial.print(' ');
  }
  Serial.println();

  // --- wait up to 1 s for a newline‐terminated response ---
  char echoBuf[128];
  int pos = 0;
  unsigned long start = micros();
  while (micros() - start < 1000000UL) {
    if (Serial1.available()) {
      char c = Serial1.read();
      if (c == '\n' || pos >= (int)sizeof(echoBuf)-1) break;
      echoBuf[pos++] = c;
    }
  }
  echoBuf[pos] = '\0';

  // --- echo back to console ---
  if (pos > 0) {
    Serial.print(F("🔁 Slave says: "));
    Serial.println(echoBuf);
  } else {
    Serial.println(F("⚠️  No response from slave."));
  }
}
