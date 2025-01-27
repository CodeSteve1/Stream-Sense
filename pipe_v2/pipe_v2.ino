#define RELAY_PIN 4           // ESP32 pin GPIO25 connected to the IN pin of the relay
#define FLOW_SENSOR_PIN 13    // ESP32 pin GPIO13 connected to the flow rate sensor
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <esp_now.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128      // OLED display width, in pixels
#define SCREEN_HEIGHT 64      // OLED display height, in pixels

// Receiver MAC Address
uint8_t broadcastAddress[] = {0x40, 0x91, 0x51, 0xFD, 0xBE, 0x0C};

typedef struct struct_message {
  float a;
  float b;
  float c;
  float d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

char valve_status[10] = "ON";
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
volatile int pulseCount = 0;  // Variable to store the pulse count
float flowRate = 0.0;         // Variable to store the calculated flow rate
unsigned long previousMillis = 0;  // To track time for flow rate calculation

// Placeholder values for PH, TDS, and TBS
float ph = 7.5;
float tds = 300;
float tbs = 40;

// Interrupt Service Routine (ISR) for counting pulses
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Function to smooth readings using exponential moving average
float smoothReading(float currentValue, float newValue, float smoothingFactor) {
  return (smoothingFactor * newValue) + ((1 - smoothingFactor) * currentValue);
}

// Calculate score based on PH, TDS, and TBS
int calculateScore(float ph, float tds, float tbs) {
  int score = 0;

  // PH score (ideal range: 6.5 to 8.5, score decreases with deviation)
  float phScore = 100 - (abs(ph - 7.0) * 10);
  phScore = constrain(phScore, 1, 100);

  // TDS score (ideal range: 200 to 500)
  float tdsScore = 100 - ((abs(tds - 350) / 150.0) * 100);
  tdsScore = constrain(tdsScore, 1, 100);

  // TBS score (ideal range: 40 to 60)
  float tbsScore = 100 - ((abs(tbs - 50) / 10.0) * 100);
  tbsScore = constrain(tbsScore, 1, 100);

  // Combine scores with equal weight
  score = (int)((phScore + tdsScore + tbsScore) / 3);
  return constrain(score, 1, 100);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Simulate PH, TDS, and TBS fluctuation with smoothing
  float new_ph = 7.5 + (sin(currentMillis / 3000.0) * 0.2) + random(-5, 6) / 100.0;
  ph = smoothReading(ph, new_ph, 0.1);

  float new_tds = 300 + (int)(sin(currentMillis / 2000.0) * 20) + random(-3, 4);
  tds = smoothReading(tds, new_tds, 0.1);

  float new_tbs = 40 + (int)(cos(currentMillis / 1500.0) * 5) + random(-2, 3);
  tbs = smoothReading(tbs, new_tbs, 0.1);

  // Calculate score
  int score = calculateScore(ph, tds, tbs);

  // Calculate flow rate every second
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    flowRate = (pulseCount / 450.0) * 60.0;
    Serial.println(flowRate);


    pulseCount = 0;  // Reset pulse count
  }

  // Control the relay
  if (flowRate > 1.0 && score>=50) {
    digitalWrite(RELAY_PIN, HIGH);
    strcpy(valve_status, "ON");
    myData.a = flowRate;
    myData.b = ph;
    myData.c = tds;
    myData.d = tbs;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result != ESP_OK) {
      Serial.println("Error sending data");
        // Display logic
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Water Quality ");
  display.println("   ");
  display.print("   PH: ");
  display.println(ph, 2);
  display.print("   TDS: ");
  display.println((int)tds);
  display.print("   TBS: ");
  display.println((int)tbs);
  display.setFont(&FreeMono9pt7b);
  display.setCursor(10, 60);
  display.print("  SCR:");
  display.println(score);
  display.setCursor(80, 40);
  display.println(valve_status);
  display.display();
  display.setFont();
  delay(200);
  display.clearDisplay();
    }
  } else {
    digitalWrite(RELAY_PIN, LOW);
    strcpy(valve_status, "OFF");
        myData.a = flowRate;
    myData.b = 0.0;
    myData.c = 0.0;
    myData.d = 0.0;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result != ESP_OK) {
      Serial.println("Error sending data");

    }
      // Display logic
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Water Quality ");
  display.println("   ");
  display.print("   PH: ");
  display.println("0");
  display.print("   TDS: ");
  display.println("0");
  display.print("   TBS: ");
  display.println("0");
  display.setFont(&FreeMono9pt7b);
  display.setCursor(10, 60);
  display.print("  SCR:");
  display.println("NWR");
  display.setCursor(80, 40);
  display.println(valve_status);
  display.display();
  display.setFont();
  delay(200);
  display.clearDisplay();
  }


}
