#include <MPS.h>
#include "config.h"
#include <DHT.h>
#include <math.h>

static constexpr uint8_t kDhtPin = 0;
static constexpr uint8_t kDhtType = DHT11;
static constexpr uint8_t kRgbLedPin = 2;
static constexpr uint16_t kHeartbeatPeriodMs = 1000;
static constexpr uint32_t kPublishPeriodMs = 2000;
static constexpr uint32_t kBaudRate = 115200;
static constexpr size_t kLedPayloadSize = 3;
static constexpr size_t kLedEnabledPayloadSize = 1;
static constexpr size_t kLedStatusPayloadSize = 4;
static constexpr size_t kCounterPayloadSize = 4;
static constexpr size_t kMeasurementPayloadSize = 4;
static constexpr uint8_t kLedEnabled = 1;
static constexpr uint8_t kLedDisabled = 0;

DHT dht(kDhtPin, kDhtType);

struct __attribute__((packed)) LedPayload {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

static_assert(sizeof(LedPayload) == kLedPayloadSize,
              "LedPayload = 3 octets");

struct __attribute__((packed)) LedStatusPayload {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t enabled;
};

static_assert(sizeof(LedStatusPayload) == kLedStatusPayloadSize,
              "LedStatusPayload = 4 octets");

struct __attribute__((packed)) LedEnabledPayload {
  uint8_t enabled;
};

static_assert(sizeof(LedEnabledPayload) == kLedEnabledPayloadSize,
              "LedEnabledPayload = 1 octet");

// ---- Payload humidité ----
struct __attribute__((packed)) HumidityPayload {
  float humidity;
};

static_assert(sizeof(HumidityPayload) == kMeasurementPayloadSize,
              "HumidityPayload = 4 octets");

static HumidityPayload humidity = {0.0f};

// ---- Payload temperature ----
struct __attribute__((packed)) TemperaturePayload {
  float temperature;
};

static_assert(sizeof(TemperaturePayload) == kMeasurementPayloadSize,
              "TemperaturePayload = 4 octets");

static TemperaturePayload temperature = {0.0f};

// ---- Structures de payloads ----
struct __attribute__((packed)) CounterPayload {
  uint32_t value;
};
static_assert(sizeof(CounterPayload) == kCounterPayloadSize, "CounterPayload = 4 octets");

// ---- Variables globales ----
static CounterPayload counter = {0};

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset");
constexpr mps::Subject kSubStatusTemperature = mps::makeSubject("status_" DEVICE_NAME "_temperature");
constexpr mps::Subject kSubStatusHumidity = mps::makeSubject("status_" DEVICE_NAME "_humidite");
constexpr mps::Subject kSubSetLedColor = mps::makeSubject("set_" DEVICE_NAME "_led_color");
constexpr mps::Subject kSubSetLedEnabled = mps::makeSubject("set_" DEVICE_NAME "_led_enabled");
constexpr mps::Subject kSubStatusLed = mps::makeSubject("status_" DEVICE_NAME "_led");

static LedStatusPayload ledState = {0, 0, 0, kLedDisabled};

// Publie le compteur, les mesures connues et l’état de la LED après l’ouverture de session.
static void publishCurrentState() {
  MPS.publishValue(kSubStatusCounter, counter);
  MPS.publishValue(kSubStatusTemperature, temperature);
  MPS.publishValue(kSubStatusHumidity, humidity);
  MPS.publishValue(kSubStatusLed, ledState);
}

// ---- Callbacks ----
static void onSessionOpened() {
  Serial.println("Session opened!");
  publishCurrentState();
}

// Applique la couleur mémorisée et confirme l’état effectif par un status_.
static void applyLedState() {
  const uint8_t red = ledState.enabled == kLedEnabled ? ledState.red : kLedDisabled;
  const uint8_t green = ledState.enabled == kLedEnabled ? ledState.green : kLedDisabled;
  const uint8_t blue = ledState.enabled == kLedEnabled ? ledState.blue : kLedDisabled;
  rgbLedWrite(kRgbLedPin, red, green, blue);
  MPS.publishValue(kSubStatusLed, ledState);
}

// Reçoit une nouvelle couleur RGB, ignore les formats incorrects et confirme son application.
static void onSetLedColor(const mps::FrameView& message, void*) {
  LedPayload color;
  if (!mps::readPayloadAs(message, &color)) return;
  ledState.red = color.red;
  ledState.green = color.green;
  ledState.blue = color.blue;
  ledState.enabled = kLedEnabled;
  applyLedState();
}

// Reçoit l’état marche/arrêt de la LED et confirme l’état appliqué.
static void onSetLedEnabled(const mps::FrameView& message, void*) {
  LedEnabledPayload command;
  if (!mps::readPayloadAs(message, &command)) return;
  if (command.enabled != kLedEnabled && command.enabled != kLedDisabled) return;
  ledState.enabled = command.enabled;
  applyLedState();
}

static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

static void onCmdResetCounter(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return;

  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter);

}

// ---- Fonctions appelées par setup/loop ----
void publishHeartbeat() {
  static uint32_t lastHeartbeatTimeMs = 0;
  if (millis() - lastHeartbeatTimeMs >= kHeartbeatPeriodMs) {
    lastHeartbeatTimeMs = millis();

    MPS.publishHeartbeat(kSubHb);
  }
}

// ---- Fonctions Arduino ----
void setup() {
  Serial.begin(kBaudRate);
  dht.begin();
  MPS.setLogOutput(&Serial);

  MPS.on(kSubCmdResetCounter, onCmdResetCounter);
  MPS.on(kSubSetLedColor, onSetLedColor);
  MPS.on(kSubSetLedEnabled, onSetLedEnabled);

  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  rgbLedWrite(kRgbLedPin, kLedDisabled, kLedDisabled, kLedDisabled);

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

  if (millis() - lastCounterTimeMs >= kPublishPeriodMs) {
    lastCounterTimeMs = millis();

    counter.value++;
    MPS.publishValue(kSubStatusCounter, counter);
  }
}

void publishDht11() {
  static uint32_t lastDhtTimeMs = 0;

  if (millis() - lastDhtTimeMs >= kPublishPeriodMs) {
    lastDhtTimeMs = millis();

    // Lecture des deux valeurs du DHT11
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Vérification de la température
    if (!isnan(temp)) {
      temperature.temperature = temp;
      MPS.publishValue(kSubStatusTemperature, temperature);
    }

    // Vérification de l'humidité
    if (!isnan(hum)) {
      humidity.humidity = hum;
      MPS.publishValue(kSubStatusHumidity, humidity);
    }
  }
}
