#include <Arduino.h>
#include <M5Unified.h>
#include <VL53L0X.h>
#include <Wire.h>

namespace {

constexpr uint8_t kTofAddress = 0x29;
constexpr int kPortASdaPin = 9;
constexpr int kPortASclPin = 10;
constexpr uint32_t kStatusPrintIntervalMs = 500;
constexpr size_t kBatchSize = 10;
constexpr size_t kHistorySize = 120;
constexpr uint32_t kDisplayRefreshBatchCount = 10;

constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t kColorBg = color565(0, 0, 0);
constexpr uint16_t kColorGrid = color565(48, 48, 48);
constexpr uint16_t kColorText = color565(240, 240, 240);
constexpr uint16_t kColorDistance = color565(0, 220, 120);
constexpr uint16_t kColorTimeout = color565(220, 60, 60);

struct Sample {
  uint32_t timestampMs = 0;
  uint16_t distanceMm = 0;
  bool valid = false;
  bool timeout = false;
};

VL53L0X gSensor;
TwoWire gTofWire = TwoWire(0);

bool gSensorReady = false;
bool gBatchSerialEnabled = true;
bool gDisplayEnabled = true;
uint32_t gTimingBudgetUs = 20000;

uint32_t gLastStatusPrintMs = 0;
uint32_t gLastLoopUs = 0;

uint32_t gLoopCount = 0;
uint32_t gSampleCount = 0;
uint32_t gLastLoopCount = 0;
uint32_t gLastSampleCount = 0;

Sample gLatestSample;
std::array<Sample, kBatchSize> gBatchSamples = {};
size_t gBatchCount = 0;
std::array<Sample, kHistorySize> gHistorySamples = {};
size_t gHistoryHead = 0;
size_t gHistoryCount = 0;
uint32_t gDisplayDirtySamples = 0;

bool probeAddress(TwoWire &wire, uint8_t address) {
  wire.beginTransmission(address);
  return wire.endTransmission() == 0;
}

bool beginSensor() {
  gTofWire.end();
  gTofWire.begin(kPortASdaPin, kPortASclPin, 400000U);
  gTofWire.setTimeOut(20);

  if (!probeAddress(gTofWire, kTofAddress)) {
    return false;
  }

  gSensor.setBus(&gTofWire);
  gSensor.setTimeout(100);
  if (!gSensor.init()) {
    return false;
  }

  gSensor.setMeasurementTimingBudget(gTimingBudgetUs);
  gSensor.startContinuous();
  gSensorReady = true;
  return true;
}

bool initSensor() {
  gSensorReady = beginSensor();
  return gSensorReady;
}

void applyTimingBudget(uint32_t timingBudgetUs) {
  gTimingBudgetUs = timingBudgetUs;
  if (!gSensorReady) {
    return;
  }
  gSensor.stopContinuous();
  gSensor.setMeasurementTimingBudget(gTimingBudgetUs);
  gSensor.startContinuous();
}

Sample acquireSample() {
  Sample sample;
  sample.timestampMs = millis();

  if (!gSensorReady) {
    return sample;
  }

  sample.distanceMm = gSensor.readRangeContinuousMillimeters();
  sample.timeout = gSensor.timeoutOccurred();
  sample.valid = !sample.timeout;
  ++gSampleCount;
  return sample;
}

void appendHistory(const Sample &sample) {
  const size_t index = (gHistoryHead + gHistoryCount) % kHistorySize;
  gHistorySamples[index] = sample;
  if (gHistoryCount < kHistorySize) {
    ++gHistoryCount;
  } else {
    gHistoryHead = (gHistoryHead + 1) % kHistorySize;
  }
}

void appendBatch(const Sample &sample) {
  if (gBatchCount < kBatchSize) {
    gBatchSamples[gBatchCount++] = sample;
  }
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h : help");
  Serial.println("  1 : timing budget 20ms");
  Serial.println("  2 : timing budget 33ms");
  Serial.println("  5 : timing budget 50ms");
  Serial.println("  0 : timing budget 100ms");
  Serial.println("  l : toggle batch serial output");
  Serial.println("  d : toggle display graph");
  Serial.println("  p : re-probe sensor");
  Serial.println("  r : print latest sample");
  Serial.println();
}

void printSampleCsvHeader() {
  Serial.println("millis,distance_mm,valid,timeout");
}

void printSampleCsv(const Sample &sample) {
  Serial.print(sample.timestampMs);
  Serial.print(',');
  Serial.print(sample.distanceMm);
  Serial.print(',');
  Serial.print(sample.valid ? "1" : "0");
  Serial.print(',');
  Serial.println(sample.timeout ? "1" : "0");
}

void flushBatchToSerial() {
  if (!gBatchSerialEnabled || gBatchCount == 0) {
    gBatchCount = 0;
    return;
  }

  Serial.print("batch budgetUs=");
  Serial.print(gTimingBudgetUs);
  Serial.print(" count=");
  Serial.print(gBatchCount);
  Serial.print(" samples=");

  for (size_t i = 0; i < gBatchCount; ++i) {
    const Sample &sample = gBatchSamples[i];
    if (i > 0) {
      Serial.print('|');
    }
    Serial.print(sample.timestampMs);
    Serial.print(':');
    Serial.print(sample.distanceMm);
    Serial.print(':');
    Serial.print(sample.valid ? 'Y' : 'N');
    Serial.print(':');
    Serial.print(sample.timeout ? 'T' : '-');
  }
  Serial.println();
  gBatchCount = 0;
}

void drawGraphPanel(int32_t x, int32_t y, int32_t w, int32_t h) {
  M5.Display.drawRect(x, y, w, h, kColorGrid);
  if (gHistoryCount < 2) {
    return;
  }

  uint32_t minValue = UINT32_MAX;
  uint32_t maxValue = 0;
  for (size_t i = 0; i < gHistoryCount; ++i) {
    const Sample &sample = gHistorySamples[(gHistoryHead + i) % kHistorySize];
    if (!sample.valid) {
      continue;
    }
    minValue = min<uint32_t>(minValue, sample.distanceMm);
    maxValue = max<uint32_t>(maxValue, sample.distanceMm);
  }

  if (minValue == UINT32_MAX) {
    minValue = 0;
    maxValue = 1;
  } else if (minValue == maxValue) {
    maxValue = minValue + 1;
  }

  const int32_t innerX = x + 1;
  const int32_t innerY = y + 1;
  const int32_t innerW = w - 2;
  const int32_t innerH = h - 2;

  for (size_t i = 1; i < gHistoryCount; ++i) {
    const Sample &prev = gHistorySamples[(gHistoryHead + i - 1) % kHistorySize];
    const Sample &curr = gHistorySamples[(gHistoryHead + i) % kHistorySize];
    if (!prev.valid || !curr.valid) {
      continue;
    }

    const int32_t x0 = innerX + static_cast<int32_t>(((i - 1) * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    const int32_t x1 = innerX + static_cast<int32_t>((i * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    const int32_t y0 = innerY + innerH - 1 -
                       static_cast<int32_t>(((prev.distanceMm - minValue) * (innerH - 1)) / (maxValue - minValue));
    const int32_t y1 = innerY + innerH - 1 -
                       static_cast<int32_t>(((curr.distanceMm - minValue) * (innerH - 1)) / (maxValue - minValue));
    M5.Display.drawLine(x0, y0, x1, y1, kColorDistance);
  }

  for (size_t i = 0; i < gHistoryCount; ++i) {
    const Sample &sample = gHistorySamples[(gHistoryHead + i) % kHistorySize];
    if (!sample.timeout) {
      continue;
    }
    const int32_t px = innerX + static_cast<int32_t>((i * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    M5.Display.drawFastVLine(px, innerY, innerH, kColorTimeout);
  }

  M5.Display.setTextColor(kColorText, kColorBg);
  M5.Display.setCursor(x + 3, y + 2);
  M5.Display.printf("Distance mm %lu-%lu", static_cast<unsigned long>(minValue), static_cast<unsigned long>(maxValue));
}

void updateDisplay() {
  if (!gDisplayEnabled || gDisplayDirtySamples < kDisplayRefreshBatchCount) {
    return;
  }

  gDisplayDirtySamples = 0;

  M5.Display.startWrite();
  M5.Display.fillScreen(kColorBg);
  M5.Display.setTextColor(kColorText, kColorBg);
  M5.Display.setCursor(0, 0);
  M5.Display.printf("TOF %s\n", gSensorReady ? "ready" : "missing");
  M5.Display.printf("port: SDA=%d SCL=%d\n", kPortASdaPin, kPortASclPin);
  M5.Display.printf("latest: %u mm %s\n", gLatestSample.distanceMm, gLatestSample.timeout ? "timeout" : "ok");
  M5.Display.printf("budget: %lu us\n", static_cast<unsigned long>(gTimingBudgetUs));
  M5.Display.printf("hist: %u samples\n", static_cast<unsigned>(gHistoryCount));

  drawGraphPanel(0, 48, M5.Display.width(), M5.Display.height() - 48);
  M5.Display.endWrite();
}

void printStatus(const Sample &sample) {
  const uint32_t nowMs = millis();
  const uint32_t elapsedMs = max<uint32_t>(1, nowMs - gLastStatusPrintMs);
  const uint32_t loopDelta = gLoopCount - gLastLoopCount;
  const uint32_t sampleDelta = gSampleCount - gLastSampleCount;
  const float loopsPerSec = (1000.0f * loopDelta) / elapsedMs;
  const float samplePerSec = (1000.0f * sampleDelta) / elapsedMs;

  Serial.print("tof=");
  Serial.print(gSensorReady ? "ready" : "missing");
  Serial.print(" port=\"SDA=");
  Serial.print(kPortASdaPin);
  Serial.print(" SCL=");
  Serial.print(kPortASclPin);
  Serial.print("\" dist=");
  Serial.print(sample.distanceMm);
  Serial.print(" valid=");
  Serial.print(sample.valid ? "yes" : "no");
  Serial.print(" timeout=");
  Serial.print(sample.timeout ? "yes" : "no");
  Serial.print(" budgetUs=");
  Serial.print(gTimingBudgetUs);
  Serial.print(" loopHz=");
  Serial.print(loopsPerSec, 1);
  Serial.print(" sampleHz=");
  Serial.print(samplePerSec, 1);
  Serial.print(" loopUs=");
  Serial.println(gLastLoopUs);

  gLastStatusPrintMs = nowMs;
  gLastLoopCount = gLoopCount;
  gLastSampleCount = gSampleCount;
}

void handleCommand(char command) {
  switch (command) {
    case 'h':
    case '?':
      printHelp();
      break;
    case '1':
      applyTimingBudget(20000);
      Serial.println("timing budget set to 20000 us");
      break;
    case '2':
      applyTimingBudget(33000);
      Serial.println("timing budget set to 33000 us");
      break;
    case '5':
      applyTimingBudget(50000);
      Serial.println("timing budget set to 50000 us");
      break;
    case '0':
      applyTimingBudget(100000);
      Serial.println("timing budget set to 100000 us");
      break;
    case 'l':
      gBatchSerialEnabled = !gBatchSerialEnabled;
      Serial.print("batch_serial=");
      Serial.println(gBatchSerialEnabled ? "on" : "off");
      break;
    case 'd':
      gDisplayEnabled = !gDisplayEnabled;
      Serial.print("display=");
      Serial.println(gDisplayEnabled ? "on" : "off");
      if (!gDisplayEnabled) {
        M5.Display.fillScreen(kColorBg);
      } else {
        gDisplayDirtySamples = kDisplayRefreshBatchCount;
      }
      break;
    case 'p':
      initSensor();
      Serial.print("sensor ");
      Serial.println(gSensorReady ? "detected" : "not detected");
      break;
    case 'r':
      printSampleCsvHeader();
      printSampleCsv(gLatestSample);
      break;
    default:
      break;
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(kColorBg);
  M5.Display.setTextSize(1);

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("M5StickS3 TOF speed test starting");
  if (initSensor()) {
    Serial.print("sensor detected on Port.A SDA=");
    Serial.print(kPortASdaPin);
    Serial.print(" SCL=");
    Serial.println(kPortASclPin);
  } else {
    Serial.println("sensor not detected on Port.A");
  }

  printHelp();
  printSampleCsvHeader();
}

void loop() {
  M5.update();

  const uint32_t loopStartUs = micros();
  ++gLoopCount;

  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == '\n' || command == '\r') {
      continue;
    }
    handleCommand(command);
  }

  if (gSensorReady) {
    gLatestSample = acquireSample();
    appendHistory(gLatestSample);
    appendBatch(gLatestSample);
    ++gDisplayDirtySamples;
    if (gBatchCount >= kBatchSize) {
      flushBatchToSerial();
    }
  }

  const uint32_t nowMs = millis();
  if (nowMs - gLastStatusPrintMs >= kStatusPrintIntervalMs) {
    printStatus(gLatestSample);
  }

  updateDisplay();
  gLastLoopUs = micros() - loopStartUs;
}
