#include <MPS.h>
#include "config.h"
#include <DHT.h>
#include <math.h>

#define DHTPIN 0
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


struct __attribute__((packed)) LedPayload {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

static_assert(sizeof(LedPayload) == 3,
              "LedPayload = 3 octets");

// ---- Payload humidité ----
struct __attribute__((packed)) HumidityPayload {
  float humidity;
};

static_assert(sizeof(HumidityPayload) == 4,
              "HumidityPayload = 4 octets");

static HumidityPayload humidity = {0.0f};

// ---- Payload temperature ----
struct __attribute__((packed)) TemperaturePayload {
  float temperature;
};

static_assert(sizeof(TemperaturePayload) == 4,
              "TemperaturePayload = 4 octets");

static TemperaturePayload temperature = {0.0f};

// ---- Structures de payloads ----
struct __attribute__((packed)) CounterPayload {
  uint32_t value;
};
static_assert(sizeof(CounterPayload) == 4, "CounterPayload = 4 octets");

// ---- Variables globales ----
static CounterPayload counter = {0};

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset");
constexpr mps::Subject kSubStatusTemperature = mps::makeSubject("status_" DEVICE_NAME "_temperature");
constexpr mps::Subject kSubStatusHumidity = mps::makeSubject("status_" DEVICE_NAME "_humidite");
constexpr mps::Subject kSubCmdLed = mps::makeSubject("cmd_" DEVICE_NAME "_led");

// ---- Callbacks ----
static void onSessionOpened() {
  Serial.println("Session opened!");

  MPS.publishValue(kSubStatusCounter, counter);
}

static void onCmdLed(const mps::FrameView& message, void*) {

  LedPayload led;

  // Vérification et lecture des 3 octets
  if (!mps::readPayloadAs(message, &led)) {
    Serial.println("Erreur : payload LED incorrect");
    return;
  }

  // Application de la couleur
  rgbLedWrite(2, led.red, led.green, led.blue);

  Serial.printf(
    "LED : R=%u G=%u B=%u\n",
    led.red,
    led.green,
    led.blue
  );
}

static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

static void onCmdResetCounter(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return; 

  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter);

  Serial.println("Compteur remis a zero");
}

// ---- Fonctions appelées par setup/loop ----
void publishHeartbeat() {
  static uint32_t lastHeartbeatTimeMs = 0;
  static const uint16_t kHbPeriod = 1000;

  if (millis() - lastHeartbeatTimeMs >= kHbPeriod) {
    lastHeartbeatTimeMs = millis();

    MPS.publishHeartbeat(kSubHb);
  }
}

// ---- Fonctions Arduino ----
void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT11 initialise !");
  MPS.setLogOutput(&Serial);

  MPS.on(kSubCmdResetCounter, onCmdResetCounter);
  MPS.on(kSubCmdLed, onCmdLed);

  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  MPS.on(kSubCmdResetCounter, onCmdResetCounter);

  static const uint8_t kUnsetMacAddress[6] = {0, 0, 0, 0, 0, 0};

  if (memcmp(kGatewayMacAddress, kUnsetMacAddress, 6) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  if (!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) {
    Serial.println("MPS: demarrage impossible");
  }
}

void loop() {

  MPS.loop();

  publishHeartbeat();
  publishDht11();

  // Envoi du compteur toutes les 2 secondes
  static uint32_t lastCounterTimeMs = 0;

  if (millis() - lastCounterTimeMs >= 2000) {
    lastCounterTimeMs = millis();

    counter.value++;
    MPS.publishValue(kSubStatusCounter, counter);
  }
}

void publishDht11() {
  static uint32_t lastDhtTimeMs = 0;

  if (millis() - lastDhtTimeMs >= 2000) {
    lastDhtTimeMs = millis();

    // Lecture des deux valeurs du DHT11
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Vérification de la température
    if (isnan(temp)) {
      Serial.println("Erreur lecture temperature DHT11 !");
    } else {
      temperature.temperature = temp;

      Serial.printf("Temperature : %.1f C\n", temp);

      MPS.publishValue(kSubStatusTemperature, temperature);
    }

    // Vérification de l'humidité
    if (isnan(hum)) {
      Serial.println("Erreur lecture humidite DHT11 !");
    } else {
      humidity.humidity = hum;

      Serial.printf("Humidite : %.1f %%\n", hum);

      MPS.publishValue(kSubStatusHumidity, humidity);
    }
  }
}
