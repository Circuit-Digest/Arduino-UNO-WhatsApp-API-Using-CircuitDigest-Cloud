#include <WiFiS3.h>
#include <WiFiSSLClient.h>

#define TRIG_PIN 9
#define ECHO_PIN 10
#define DIST_LIMIT 10
#define COOLDOWN_MS 15000

const char* ssid     = "YOUR_WiFi_SSID";
const char* password = "YOUR_WiFi_PASSWORD";
const char* apiKey   = "YOUR_API_KEY";

const char* host = "www.circuitdigest.cloud";
const int httpsPort = 443;

WiFiSSLClient client;

unsigned long lastSentTime = 0;

long measureDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;

  return distance;
}

void sendAlert(long distance)
{
  if (!client.connect(host, httpsPort)) {
    Serial.println("TLS failed");
    return;
  }

  String distanceStr = String(distance) + "cm";

  String payload =
    "{"
      "\"phone_number\":\"919876543210\","
      "\"template_id\":\"threshold_violation_alert\","
      "\"variables\":{"
        "\"device_name\":\"UNO R4 Ultrasonic Node\","
        "\"parameter\":\"Distance\","
        "\"measured_value\":\"" + distanceStr + "\","
        "\"limit\":\"10cm\","
        "\"location\":\"Entrance\""
      "}"
    "}";

  client.println("POST /api/v1/whatsapp/send HTTP/1.1");
  client.println("Host: www.circuitdigest.cloud");
  client.println("Authorization: " + String(apiKey));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.println(payload);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  Serial.println(client.readString());
  client.stop();
}

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
}

void loop()
{
  long distance = measureDistance();
  unsigned long now = millis();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 &&
      distance <= DIST_LIMIT &&
      now - lastSentTime > COOLDOWN_MS)
  {
    Serial.println("Threshold reached → Sending WhatsApp");
    sendAlert(distance);
    lastSentTime = now;
  }

  delay(500);
}