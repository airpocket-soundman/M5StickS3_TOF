#include <Arduino.h>
#include <array>
#include <cmath>
#include <cstring>
#include <M5Unified.h>
#include <VL53L0X.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <vl53l4cd_class.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr uint8_t kTofAddress = 0x29;
constexpr uint8_t kGp2yAddress = 0x40;
constexpr int kPortASdaPin = 9;
constexpr int kPortASclPin = 10;
constexpr int kHatSdaPin = 43;
constexpr int kHatSclPin = 44;
constexpr int kGp2yAnalogPin = 8;  // GP2Y0E03 Vout(A), white wire
constexpr uint32_t kStatusIntervalMs = 500;
constexpr uint32_t kDisplayIntervalMs = 100;
constexpr uint32_t kVl53TimingBudgetUs = 20000;
constexpr uint32_t kVl6180TimeoutMs = 80;
constexpr uint32_t kSampleRateIntervalMs = 1000;
// Oversample the sensor (GP2Y0E03 fast mode cycle: ~1.9ms) so no update is
// missed; duplicate reads are collapsed by the raw-change tracking.
constexpr uint32_t kI2cPollIntervalUs = 1000;
// Decimate history pushes so one graph screen covers a few seconds.
constexpr uint32_t kGp2yHistoryDecimation = 4;  // 500Hz/4 -> 125Hz, 240pts = ~1.9s
constexpr size_t kHistorySize = 240;
constexpr uint32_t kDisplayRefreshBatchCount = 10;

constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t kColorBg = color565(0, 0, 0);
constexpr uint16_t kColorText = color565(240, 240, 240);
constexpr uint16_t kColorGrid = color565(48, 48, 48);
constexpr uint16_t kColorDistance = color565(0, 220, 120);
constexpr uint16_t kColorTimeout = color565(220, 60, 60);

// Wireless theremin: stream a distance-pitched sine to the Tab5 synth over
// UDP. Protocol matches AtomS3_I2S_TX: 16kHz/16bit mono packets with an
// 8-byte header (seq:4, gate:1, reserved:3). The Tab5 applies its own amp
// envelope from the gate, so the tone itself streams continuously.
constexpr const char* kApSsid = "Tab5Synth";
constexpr const char* kApPassword = "tab5synth";
const IPAddress kUdpTarget(192, 168, 4, 1);
constexpr uint16_t kUdpPort = 5005;
constexpr uint32_t kAudioSampleRate = 16000;
constexpr size_t kAudioChunkFrames = 256;  // 16ms per chunk
constexpr size_t kUdpChunkFrames = 128;    // receiver accepts <=160 frames/packet
constexpr size_t kUdpHeaderBytes = 8;
// Ripple-to-sound: a rolling recorder captures the displacement waveform
// (water ripples at a few Hz to tens of Hz). Each detected onset (droplet
// impact) anchors a fixed window; when the window completes it is played
// back time-compressed so the ripple's own waveform becomes audible while
// keeping its shape.
constexpr uint16_t kGateMaxMm = 400;          // valid-measurement limit
constexpr float kDisplacementGain = 1333.0f;  // audio counts per mm of motion (tuned: 3000 clipped)
constexpr float kTwoPi = 6.28318530718f;

constexpr uint32_t kRecordRateHz = 500;  // uniform recording clock (sample-hold)
constexpr size_t kRecordRingSeconds = 15;
constexpr size_t kRecordRingSize = kRecordRateHz * kRecordRingSeconds;  // 7500 (15KB)
constexpr size_t kWindowSamples = kRecordRateHz * 2;                    // 2s window (1000)
// Playback reads the 500Hz recording at 16kHz -> exactly 32x compression;
// a 5s window becomes a 156ms grain.
constexpr float kRecordHighPassHz = 0.5f;  // remove DC water level, keep ripples
constexpr float kOnsetThresholdMmDefault = 3.0f;   // above VL6180X noise floor (~2.9mm peaks)
constexpr float kOnsetRatio = 2.0f;                // env_fast > env_slow * ratio
constexpr float kEnvFastPeakDecay = 0.9868f;       // peak-hold, ~150ms decay @500Hz
constexpr float kEnvSlowAlpha = 0.00664f;          // ~300ms @500Hz
constexpr uint32_t kOnsetRefractorySamples = 150;  // 300ms @500Hz (avoid per-crest retriggers)
constexpr uint32_t kOnsetDebounceSamples = 25;     // condition must hold 50ms before firing
constexpr uint32_t kOnsetPrerollSamples = 75;      // window starts 150ms before detection
constexpr uint32_t kOnsetPeakTrackSamples = 250;   // track event peak for 0.5s after firing
constexpr uint32_t kValidBlankUs = 400000;         // no onsets until 400ms of valid readings
// Retrigger guard: after an onset, a new one must exceed the decaying
// reference of the previous peak. A naturally decaying ripple stays under
// it; a fresh droplet jumps over it.
// Two-stage retrigger guard: hold the event's peak during the track window
// (prevents double-fires within one droplet), then decay fast so the next
// droplet is accepted quickly even while the previous sound is still playing.
constexpr float kOnsetRefDecayFast = 0.99f;  // ~0.2s time constant @500Hz
constexpr float kOnsetRefFactor = 1.2f;
constexpr size_t kMaxPendingOnsets = 4;
constexpr size_t kGrainQueueSize = 4;  // 4 x 5KB
constexpr int kGrainRepeatsAfterEnd = 0;  // raw: play each window exactly once
constexpr float kGrainRepeatDecay = 0.6f;
// Record-side noise filter, matched to each sensor's information bandwidth.
// GP2Y0E03 measured true update rate is ~300Hz; a 60Hz cutoff removes the
// sample-hold staircase and LSB quantization steps while leaving the ripple
// band (<50Hz) untouched.
constexpr float kRecordLowPassHzTof = 40.0f;
constexpr float kRecordLowPassHzGp2y = 120.0f;  // interpolation removes the staircase
constexpr size_t kGrainFadeSamples = 64;  // declick at grain boundaries
// Ring sample scale: mm * kRingScale stored as int16. 64 gives +/-512mm
// headroom (splash spikes exceeded the old +/-128mm) at 0.016mm resolution.
constexpr float kRingScale = 64.0f;

enum class SensorType : uint8_t {
  None,
  Vl53L0X,
  Vl6180X,
  Vl53L4CD,
  Gp2y0e03,
};

struct Sample {
  uint32_t timestampMs = 0;
  float distanceMm = 0.0f;  // float keeps GP2Y0E03's 0.156mm resolution
  bool valid = false;
  bool timeout = false;
};

VL53L0X gVl53;
TwoWire gTofWire = TwoWire(0);
VL53L4CD gVl53L4cd(&gTofWire, -1);
M5Canvas gCanvas(&M5.Display);
SensorType gSensorType = SensorType::None;
const char* gBusLabel = "-";
uint8_t gGp2yRawMsb = 0;
uint8_t gGp2yRawLsb = 0;
// True sensor update rate: counts polls where the raw register value changed.
uint16_t gGp2yLastRaw = 0xFFFF;
uint32_t gGp2yRawChanges = 0;
uint32_t gGp2yLastRawChanges = 0;
float gGp2yRawChangeHz = 0.0f;
// Catmull-Rom spline reconstruction over the recent real sensor updates.
// The recorder evaluates the spline at (now - kInterpDelayUs), so it always
// has points on both sides of the target time (chosen offline against the
// raw captures: spline + LP120Hz reproduced ripples best).
constexpr size_t kInterpEvents = 8;
constexpr uint32_t kInterpDelayUs = 10000;  // 10ms reconstruction latency
portMUX_TYPE gInterpMux = portMUX_INITIALIZER_UNLOCKED;
float gEvV[kInterpEvents] = {};
uint32_t gEvT[kInterpEvents] = {};
size_t gEvHead = 0;   // next write slot
size_t gEvCount = 0;  // valid entries
Sample gLatestSample;
uint32_t gLastStatusMs = 0;
uint32_t gLastDisplayMs = 0;
uint32_t gSampleRateWindowStartMs = 0;
uint32_t gSampleCountInWindow = 0;
float gSampleRateHz = 0.0f;
std::array<Sample, kHistorySize> gHistory = {};
size_t gHistoryHead = 0;
size_t gHistoryCount = 0;
uint32_t gDisplayDirtySamples = 0;
bool gCanvasReady = false;

WiFiUDP gUdp;
uint32_t gUdpSequence = 0;
uint8_t gUdpPacket[kUdpHeaderBytes + kUdpChunkFrames * 2] = {};
// Rotating chunks: the local speaker plays from the buffer asynchronously.
constexpr size_t kAudioChunkBuffers = 3;
int16_t gAudioChunks[kAudioChunkBuffers][kAudioChunkFrames] = {};
size_t gAudioChunkIndex = 0;
bool gSpeakerReady = false;
volatile float gDisplacementMm = 0.0f;  // latest sensor reading (sample-hold)
volatile int16_t gAudioPeak = 0;
volatile bool gGate = false;
volatile bool gWifiOk = false;
TaskHandle_t gAudioTask = nullptr;

// --- Rolling recorder state (all touched only from the audio task) ---
int16_t gRecordRing[kRecordRingSize] = {};  // ripple motion, mm * 256
uint64_t gRecordTotal = 0;                  // absolute sample counter
float gRecordHpLp = 0.0f;
float gRecordLp = 0.0f;
float gEnvFast = 0.0f;
float gEnvSlow = 0.0f;
uint64_t gLastOnsetSample = 0;
uint32_t gOnsetHoldCount = 0;
float gOnsetRefEnv = 0.0f;
volatile uint32_t gLastInvalidUs = 0;
// Median-of-3 filter for VL6180X outlier spikes.
float gMedianBuf[3] = {0.0f, 0.0f, 0.0f};
size_t gMedianIndex = 0;
uint64_t gPendingOnsets[kMaxPendingOnsets] = {};
size_t gPendingOnsetCount = 0;
// Grain queue (producer and consumer are both the audio task).
int16_t gGrains[kGrainQueueSize][kWindowSamples] = {};
float gGrainScale[kGrainQueueSize] = {};  // per-grain auto-normalize factor
size_t gGrainRead = 0;
size_t gGrainWrite = 0;
size_t gGrainCount = 0;
// Snapshot of the most recently extracted grain, for serial waveform dumps.
int16_t gLastGrain[kWindowSamples] = {};
volatile bool gLastGrainValid = false;
// Raw capture: every I2C poll (timestamp + raw register value), no filtering.
// For offline reconstruction/filter experiments on the PC.
constexpr size_t kRawCapCount = 4800;  // ~6s at ~800Hz polling
uint32_t gRawCapTime[kRawCapCount] = {};
uint16_t gRawCapVal[kRawCapCount] = {};
volatile size_t gRawCapIndex = kRawCapCount;  // == kRawCapCount means idle
uint32_t gRawCapStartUs = 0;
volatile bool gRawCapDumpPending = false;
// Polyphonic grain voices: each completed window starts immediately on a
// free voice with its own wire ID, so overlapping droplets overlap in sound
// (and as separate streams on the Tab5 side).
constexpr size_t kTxVoices = 4;
constexpr uint8_t kTailChunks = 12;  // ~200ms of gate-off silence after a grain
struct TxVoice {
  bool active = false;
  int16_t buf[kWindowSamples] = {};
  float pos = 0.0f;  // fractional read position (playback-speed control)
  uint8_t id = 0;
  uint8_t tail = 0;
};
// Grain playback speed: 1.0 = raw 32x compression (62ms), 0.15 (PC-tuned
// default) stretches each grain to ~417ms at a proportionally lower pitch.
volatile float gPlaybackSpeed = 0.2f;
TxVoice gTxVoices[kTxVoices] = {};
uint8_t gNextVoiceId = 1;
// History of the last transmitted grains (exact UDP payload content), for
// per-grain waveform inspection on the PC via the 'w' command.
constexpr size_t kSentLogCount = 4;
int16_t gSentGrains[kSentLogCount][kWindowSamples] = {};
uint8_t gSentIds[kSentLogCount] = {};
size_t gSentWrite = 0;
size_t gSentCount = 0;
// Stats for the display.
volatile uint32_t gOnsetCount = 0;
volatile uint32_t gGrainsPlayed = 0;
volatile uint32_t gGrainsDropped = 0;
volatile float gEnvFastView = 0.0f;
// Runtime-tunable parameters (serial commands).
volatile float gOnsetThresholdMm = kOnsetThresholdMmDefault;
volatile float gOutputGain = kDisplacementGain;
volatile float gRecordLpHz = kRecordLowPassHzGp2y;
// Simulated droplet injector: decaying 8Hz sine added to the displacement.
volatile float gSimAmplitudeMm = 0.0f;
float gSimPhase = 0.0f;
char gSerialCmd[16] = {};
size_t gSerialCmdLen = 0;
// Cross-task requests: sensor register writes happen only in the sensor task.
volatile bool gReprobeRequest = false;
volatile int gAccumRequest = -1;
TaskHandle_t gSensorTask = nullptr;

bool probeAddress(uint8_t address) {
  gTofWire.beginTransmission(address);
  return gTofWire.endTransmission() == 0;
}

bool writeVl6180Reg8(uint16_t reg, uint8_t value) {
  gTofWire.beginTransmission(kTofAddress);
  gTofWire.write(static_cast<uint8_t>(reg >> 8));
  gTofWire.write(static_cast<uint8_t>(reg & 0xFF));
  gTofWire.write(value);
  return gTofWire.endTransmission() == 0;
}

bool readVl6180Reg8(uint16_t reg, uint8_t& value) {
  gTofWire.beginTransmission(kTofAddress);
  gTofWire.write(static_cast<uint8_t>(reg >> 8));
  gTofWire.write(static_cast<uint8_t>(reg & 0xFF));
  if (gTofWire.endTransmission(false) != 0) {
    return false;
  }
  if (gTofWire.requestFrom(static_cast<int>(kTofAddress), 1) != 1) {
    return false;
  }
  value = gTofWire.read();
  return true;
}

void loadVl6180Tuning() {
  writeVl6180Reg8(0x0207, 0x01);
  writeVl6180Reg8(0x0208, 0x01);
  writeVl6180Reg8(0x0096, 0x00);
  writeVl6180Reg8(0x0097, 0xFD);
  writeVl6180Reg8(0x00E3, 0x01);
  writeVl6180Reg8(0x00E4, 0x03);
  writeVl6180Reg8(0x00E5, 0x02);
  writeVl6180Reg8(0x00E6, 0x01);
  writeVl6180Reg8(0x00E7, 0x03);
  writeVl6180Reg8(0x00F5, 0x02);
  writeVl6180Reg8(0x00D9, 0x05);
  writeVl6180Reg8(0x00DB, 0xCE);
  writeVl6180Reg8(0x00DC, 0x03);
  writeVl6180Reg8(0x00DD, 0xF8);
  writeVl6180Reg8(0x009F, 0x00);
  writeVl6180Reg8(0x00A3, 0x3C);
  writeVl6180Reg8(0x00B7, 0x00);
  writeVl6180Reg8(0x00BB, 0x3C);
  writeVl6180Reg8(0x00B2, 0x09);
  writeVl6180Reg8(0x00CA, 0x09);
  writeVl6180Reg8(0x0198, 0x01);
  writeVl6180Reg8(0x01B0, 0x17);
  writeVl6180Reg8(0x01AD, 0x00);
  writeVl6180Reg8(0x00FF, 0x05);
  writeVl6180Reg8(0x0100, 0x05);
  writeVl6180Reg8(0x0199, 0x05);
  writeVl6180Reg8(0x01A6, 0x1B);
  writeVl6180Reg8(0x01AC, 0x3E);
  writeVl6180Reg8(0x01A7, 0x1F);
  writeVl6180Reg8(0x0030, 0x00);

  writeVl6180Reg8(0x0011, 0x10);
  writeVl6180Reg8(0x010A, 0x30);
  writeVl6180Reg8(0x003F, 0x46);
  writeVl6180Reg8(0x0031, 0xFF);
  writeVl6180Reg8(0x0041, 0x63);
  writeVl6180Reg8(0x002E, 0x01);
  writeVl6180Reg8(0x001B, 0x09);
  writeVl6180Reg8(0x003E, 0x31);
  writeVl6180Reg8(0x0014, 0x24);
}

bool initVl6180() {
  uint8_t modelId = 0;
  if (!readVl6180Reg8(0x0000, modelId) || modelId != 0xB4) {
    return false;
  }

  uint8_t freshOutOfReset = 0;
  if (!readVl6180Reg8(0x0016, freshOutOfReset)) {
    return false;
  }
  if (freshOutOfReset == 1) {
    loadVl6180Tuning();
    if (!writeVl6180Reg8(0x0016, 0x00)) {
      return false;
    }
  }
  return true;
}

bool initVl53() {
  gVl53.setBus(&gTofWire);
  gVl53.setTimeout(kVl6180TimeoutMs);
  if (!gVl53.init()) {
    return false;
  }
  gVl53.setMeasurementTimingBudget(kVl53TimingBudgetUs);
  gVl53.startContinuous();
  return true;
}

bool writeGp2yReg8(uint8_t reg, uint8_t value) {
  gTofWire.beginTransmission(kGp2yAddress);
  gTofWire.write(reg);
  gTofWire.write(value);
  return gTofWire.endTransmission() == 0;
}

bool readGp2yRegs(uint8_t startReg, uint8_t* buffer, size_t len) {
  gTofWire.beginTransmission(kGp2yAddress);
  gTofWire.write(startReg);
  if (gTofWire.endTransmission(false) != 0) {
    return false;
  }
  if (gTofWire.requestFrom(static_cast<int>(kGp2yAddress), static_cast<int>(len)) !=
      static_cast<int>(len)) {
    while (gTofWire.available()) {
      gTofWire.read();
    }
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buffer[i] = static_cast<uint8_t>(gTofWire.read());
  }
  return true;
}

bool initGp2y() {
  // Fast mode: accumulation N=1 (~1.9ms cycle, ~500Hz) + burst measurement.
  if (!writeGp2yReg8(0xA8, 0x00)) {
    return false;
  }
  delay(2);
  if (!writeGp2yReg8(0x3F, 0x38)) {
    return false;
  }
  delay(2);
  return true;
}

bool initVl53L4Cd() {
  gVl53L4cd.begin();
  if (gVl53L4cd.InitSensor() != VL53L4CD_ERROR_NONE) {
    return false;
  }
  if (gVl53L4cd.VL53L4CD_SetRangeTiming(10, 0) != VL53L4CD_ERROR_NONE) {
    return false;
  }
  if (gVl53L4cd.VL53L4CD_StartRanging() != VL53L4CD_ERROR_NONE) {
    return false;
  }
  return true;
}

void beginBus(int sda, int scl) {
  gTofWire.end();
  gTofWire.begin(sda, scl, 400000U);
  gTofWire.setTimeOut(20);
}

bool detectGp2yOnCurrentBus() {
  if (probeAddress(kGp2yAddress) && initGp2y()) {
    gSensorType = SensorType::Gp2y0e03;
    return true;
  }
  return false;
}

bool detectTofOnCurrentBus() {
  if (probeAddress(kTofAddress)) {
    if (initVl6180()) {
      gSensorType = SensorType::Vl6180X;
      return true;
    }
    if (initVl53L4Cd()) {
      gSensorType = SensorType::Vl53L4CD;
      return true;
    }
    if (initVl53()) {
      gSensorType = SensorType::Vl53L0X;
      return true;
    }
  }
  return false;
}

bool detectSensor() {
  gSensorType = SensorType::None;

  // GP2Y0E03 first: with both sensors attached it wins because of its far
  // higher sample rate (~500Hz vs 50-100Hz for the ToF units).
  beginBus(kHatSdaPin, kHatSclPin);
  if (detectGp2yOnCurrentBus()) {
    gBusLabel = "Hat G43/G44";
    gRecordLpHz = kRecordLowPassHzGp2y;
    return true;
  }
  beginBus(kPortASdaPin, kPortASclPin);
  if (detectGp2yOnCurrentBus()) {
    gBusLabel = "Grove G9/G10";
    gRecordLpHz = kRecordLowPassHzGp2y;
    return true;
  }

  beginBus(kPortASdaPin, kPortASclPin);
  if (detectTofOnCurrentBus()) {
    gBusLabel = "Grove G9/G10";
    gRecordLpHz = kRecordLowPassHzTof;
    return true;
  }
  beginBus(kHatSdaPin, kHatSclPin);
  if (detectTofOnCurrentBus()) {
    gBusLabel = "Hat G43/G44";
    gRecordLpHz = kRecordLowPassHzTof;
    return true;
  }

  gBusLabel = "-";
  return false;
}

const char* sensorName() {
  switch (gSensorType) {
    case SensorType::Vl53L0X:
      return "VL53L0X";
    case SensorType::Vl6180X:
      return "VL6180X";
    case SensorType::Vl53L4CD:
      return "VL53L4CD";
    case SensorType::Gp2y0e03:
      return "GP2Y0E03";
    default:
      return "NONE";
  }
}

Sample readGp2ySample() {
  Sample sample;
  sample.timestampMs = millis();

  uint8_t buffer[2] = {0, 0};
  if (!readGp2yRegs(0x5E, buffer, sizeof(buffer))) {
    sample.timeout = true;
    return sample;
  }

  gGp2yRawMsb = buffer[0];
  gGp2yRawLsb = buffer[1];
  const uint16_t raw = static_cast<uint16_t>((buffer[0] << 4) | (buffer[1] >> 4));
  if (raw != gGp2yLastRaw) {
    gGp2yLastRaw = raw;
    ++gGp2yRawChanges;
  }
  if (gRawCapIndex < kRawCapCount) {
    gRawCapTime[gRawCapIndex] = micros() - gRawCapStartUs;
    gRawCapVal[gRawCapIndex] = raw;
    ++gRawCapIndex;
    if (gRawCapIndex == kRawCapCount) {
      gRawCapDumpPending = true;
    }
  }
  // Datasheet: distance[cm] = raw / 16 / 2^shift (default shift=2).
  // Keep the full 0.156mm/LSB resolution.
  sample.distanceMm = static_cast<float>(raw) * 10.0f / 64.0f;
  sample.valid = !(buffer[0] == 0xFF && buffer[1] == 0xFF);
  sample.timeout = false;
  return sample;
}

Sample readVl53Sample() {
  Sample sample;
  sample.timestampMs = millis();
  sample.distanceMm = static_cast<float>(gVl53.readRangeContinuousMillimeters());
  sample.timeout = gVl53.timeoutOccurred();
  sample.valid = !sample.timeout;
  return sample;
}

Sample readVl6180Sample() {
  Sample sample;
  sample.timestampMs = millis();

  if (!writeVl6180Reg8(0x0018, 0x01)) {
    sample.timeout = true;
    return sample;
  }

  uint8_t status = 0;
  const uint32_t startMs = millis();
  while (millis() - startMs < kVl6180TimeoutMs) {
    if (!readVl6180Reg8(0x004F, status)) {
      sample.timeout = true;
      return sample;
    }
    if ((status & 0x07U) == 0x04U) {
      break;
    }
    delay(1);
  }

  if ((status & 0x07U) != 0x04U) {
    sample.timeout = true;
    return sample;
  }

  uint8_t distance = 0;
  uint8_t rangeStatus = 0;
  if (!readVl6180Reg8(0x0062, distance) || !readVl6180Reg8(0x004D, rangeStatus)) {
    sample.timeout = true;
    return sample;
  }

  writeVl6180Reg8(0x0015, 0x07);

  const uint8_t errorCode = (rangeStatus >> 4) & 0x0F;
  sample.distanceMm = static_cast<float>(distance);
  sample.valid = (errorCode == 0) && (distance != 255);
  sample.timeout = false;
  return sample;
}

Sample readVl53L4CdSample() {
  Sample sample;
  sample.timestampMs = millis();

  uint8_t ready = 0;
  const uint32_t startMs = millis();
  while (millis() - startMs < kVl6180TimeoutMs) {
    if (gVl53L4cd.VL53L4CD_CheckForDataReady(&ready) != VL53L4CD_ERROR_NONE) {
      sample.timeout = true;
      return sample;
    }
    if (ready) {
      break;
    }
    delay(1);
  }

  if (!ready) {
    sample.timeout = true;
    return sample;
  }

  VL53L4CD_Result_t result = {};
  if (gVl53L4cd.VL53L4CD_GetResult(&result) != VL53L4CD_ERROR_NONE) {
    sample.timeout = true;
    return sample;
  }
  gVl53L4cd.VL53L4CD_ClearInterrupt();

  sample.distanceMm = static_cast<float>(result.distance_mm);
  sample.valid = (result.range_status == 0);
  sample.timeout = false;
  return sample;
}

void sendUdpChunks(const int16_t* chunk, bool gate, uint8_t voice_id) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  for (size_t offset = 0; offset < kAudioChunkFrames; offset += kUdpChunkFrames) {
    std::memcpy(gUdpPacket, &gUdpSequence, 4);
    gUdpPacket[4] = gate ? 1 : 0;
    gUdpPacket[5] = voice_id;
    gUdpPacket[6] = 0;
    gUdpPacket[7] = 0;
    std::memcpy(gUdpPacket + kUdpHeaderBytes, &chunk[offset], kUdpChunkFrames * 2);
    ++gUdpSequence;
    if (gUdp.beginPacket(kUdpTarget, kUdpPort) == 1) {
      gUdp.write(gUdpPacket, sizeof(gUdpPacket));
      gUdp.endPacket();
    }
  }
}

// One 500Hz recording step: push the latest displacement into the rolling
// ring, run onset detection, and cut finished windows into the grain queue.
// Evaluate the Catmull-Rom spline through the recent sensor updates at the
// given absolute time. Returns the held value when not enough points.
float splineDisplacement(uint32_t target) {
  float v[kInterpEvents];
  uint32_t t[kInterpEvents];
  size_t count, head;
  taskENTER_CRITICAL(&gInterpMux);
  count = gEvCount;
  head = gEvHead;
  memcpy(v, const_cast<const float*>(gEvV), sizeof(v));
  memcpy(t, const_cast<const uint32_t*>(gEvT), sizeof(t));
  taskEXIT_CRITICAL(&gInterpMux);

  if (count == 0) {
    return gDisplacementMm;
  }

  // Chronological order: index 0 = oldest.
  float cv[kInterpEvents];
  uint32_t ct[kInterpEvents];
  for (size_t k = 0; k < count; ++k) {
    const size_t src = (head + kInterpEvents - count + k) % kInterpEvents;
    cv[k] = v[src];
    ct[k] = t[src];
  }

  if (static_cast<int32_t>(target - ct[count - 1]) >= 0 || count == 1) {
    return cv[count - 1];  // beyond newest: hold
  }
  if (static_cast<int32_t>(target - ct[0]) <= 0) {
    return cv[0];  // before oldest: hold oldest
  }

  size_t j = 0;
  while (j + 1 < count && static_cast<int32_t>(ct[j + 1] - target) <= 0) {
    ++j;
  }
  const uint32_t span = ct[j + 1] - ct[j];
  if (span == 0) {
    return cv[j + 1];
  }
  float u = static_cast<float>(target - ct[j]) / static_cast<float>(span);
  u = std::max(0.0f, std::min(1.0f, u));

  const float p0 = cv[j > 0 ? j - 1 : 0];
  const float p1 = cv[j];
  const float p2 = cv[j + 1];
  const float p3 = cv[j + 2 < count ? j + 2 : count - 1];
  return 0.5f * ((2.0f * p1) + (-p0 + p2) * u + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u * u +
                 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * u * u * u);
}

uint32_t gRecordClockUs = 0;  // virtual 500Hz recording timeline

void recordStep(float hp_coef) {
  // Advance the virtual record clock exactly 2ms per step (the audio task
  // calls recordStep in bursts, so wall-clock time cannot be used per call).
  gRecordClockUs += 1000000 / kRecordRateHz;
  const int32_t drift = static_cast<int32_t>(micros() - gRecordClockUs);
  if (drift > 20000 || drift < -20000) {
    gRecordClockUs = micros();
  }
  // Spline reconstruction of the sensor stream (sub-LSB smoothness instead
  // of a zero-order-hold staircase).
  float x = splineDisplacement(gRecordClockUs - kInterpDelayUs);
  if (gRecordTotal == 0) {
    // Start the DC filter settled at the current level so power-on does not
    // look like a huge displacement step.
    gRecordHpLp = x;
  }
  if (gSimAmplitudeMm > 0.001f) {
    gSimPhase += kTwoPi * 8.0f / static_cast<float>(kRecordRateHz);
    if (gSimPhase >= kTwoPi) {
      gSimPhase -= kTwoPi;
    }
    x += gSimAmplitudeMm * sinf(gSimPhase);
    gSimAmplitudeMm *= 0.99867f;  // ~1.5s decay time constant @500Hz
  }
  gRecordHpLp += hp_coef * (x - gRecordHpLp);
  const float raw_motion = x - gRecordHpLp;
  // Low-pass to strip sensor noise while keeping the ripple band intact.
  const float lp_coef = 1.0f - expf(-kTwoPi * gRecordLpHz / static_cast<float>(kRecordRateHz));
  gRecordLp += lp_coef * (raw_motion - gRecordLp);
  const float motion = gRecordLp;
  gRecordRing[gRecordTotal % kRecordRingSize] =
      static_cast<int16_t>(std::max(-32767.0f, std::min(32767.0f, motion * kRingScale)));
  ++gRecordTotal;

  const float magnitude = motion < 0 ? -motion : motion;
  // Peak-hold envelope: does not dip at the wave's zero crossings, so the
  // debounce can require sustained activity.
  gEnvFast = std::max(magnitude, gEnvFast * kEnvFastPeakDecay);
  gEnvSlow += kEnvSlowAlpha * (magnitude - gEnvSlow);
  gEnvFastView = gEnvFast;

  // Onset: a sudden rise above the decaying trend = a new droplet.
  // Debounced: the condition must hold continuously so single-sample
  // sensor spikes cannot fire.
  if (gRecordTotal - gLastOnsetSample < kOnsetPeakTrackSamples) {
    // Within the event: hold the running peak (no decay) so a growing burst
    // cannot retrigger as a phantom droplet.
    if (gEnvFast > gOnsetRefEnv) {
      gOnsetRefEnv = gEnvFast;
    }
  } else {
    // Event over: release the guard quickly for the next droplet.
    gOnsetRefEnv *= kOnsetRefDecayFast;
  }
  const float fire_level = std::max(static_cast<float>(gOnsetThresholdMm), gOnsetRefEnv * kOnsetRefFactor);
  const bool refractory_over = (gRecordTotal - gLastOnsetSample) >= kOnsetRefractorySamples;
  const bool condition = gEnvFast > fire_level && gEnvFast > gEnvSlow * kOnsetRatio;
  if (condition) {
    ++gOnsetHoldCount;
  } else {
    gOnsetHoldCount = 0;
  }
  // Arm onsets only after the filters have settled (2s after boot), and only
  // when the sensor has been returning valid readings for a while (invalid ->
  // valid transitions produce phantom steps).
  const bool armed = gRecordTotal >= kRecordRateHz * 2 && (micros() - gLastInvalidUs) > kValidBlankUs;
  if (armed && refractory_over && gOnsetHoldCount >= kOnsetDebounceSamples) {
    gOnsetHoldCount = 0;
    gLastOnsetSample = gRecordTotal;
    gOnsetRefEnv = gEnvFast;
    ++gOnsetCount;
    if (gPendingOnsetCount < kMaxPendingOnsets) {
      // Anchor before the rise: debounce compensation plus pre-roll so the
      // very first attack transient is always inside the window.
      const uint64_t rewind = kOnsetDebounceSamples + kOnsetPrerollSamples;
      const uint64_t anchor = gRecordTotal > rewind ? gRecordTotal - rewind : 0;
      gPendingOnsets[gPendingOnsetCount++] = anchor;
    }
  }

  // A window is complete when kWindowSamples have elapsed since its onset.
  if (gPendingOnsetCount > 0 && gRecordTotal >= gPendingOnsets[0] + kWindowSamples) {
    const uint64_t start = gPendingOnsets[0];
    for (size_t i = 1; i < gPendingOnsetCount; ++i) {
      gPendingOnsets[i - 1] = gPendingOnsets[i];
    }
    --gPendingOnsetCount;

    if (gGrainCount < kGrainQueueSize) {
      int16_t* grain = gGrains[gGrainWrite];
      int32_t peak = 1;
      for (size_t i = 0; i < kWindowSamples; ++i) {
        grain[i] = gRecordRing[(start + i) % kRecordRingSize];
        const int32_t magnitude = grain[i] < 0 ? -grain[i] : grain[i];
        if (magnitude > peak) {
          peak = magnitude;
        }
      }
      // Auto-normalize: every droplet plays at a consistent, clip-free level
      // regardless of ripple amplitude (amplification capped to avoid
      // boosting pure noise into audibility).
      float scale = 24000.0f / static_cast<float>(peak);
      if (scale > 100.0f) {
        scale = 100.0f;
      }
      gGrainScale[gGrainWrite] = scale;
      memcpy(gLastGrain, grain, sizeof(gLastGrain));
      gLastGrainValid = true;
      gGrainWrite = (gGrainWrite + 1) % kGrainQueueSize;
      ++gGrainCount;
    } else {
      ++gGrainsDropped;
    }
  }
}

// Assign queued grains to free voices (each starts playing immediately).
void startQueuedGrains() {
  while (gGrainCount > 0) {
    TxVoice* free_voice = nullptr;
    for (auto& voice : gTxVoices) {
      if (!voice.active) {
        free_voice = &voice;
        break;
      }
    }
    if (free_voice == nullptr) {
      break;  // all voices busy: grain stays queued
    }
    const int16_t* grain = gGrains[gGrainRead];
    // Bake normalize + master volume + declick fades into the voice buffer.
    const float scale = gGrainScale[gGrainRead] * (gOutputGain / 1333.0f);
    for (size_t i = 0; i < kWindowSamples; ++i) {
      float value = static_cast<float>(grain[i]) * scale;
      if (i < kGrainFadeSamples) {
        value *= static_cast<float>(i) / static_cast<float>(kGrainFadeSamples);
      } else if (i + kGrainFadeSamples >= kWindowSamples) {
        value *= static_cast<float>(kWindowSamples - 1 - i) / static_cast<float>(kGrainFadeSamples);
      }
      free_voice->buf[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, value)));
    }
    gGrainRead = (gGrainRead + 1) % kGrainQueueSize;
    --gGrainCount;
    free_voice->pos = 0;
    free_voice->tail = kTailChunks;
    free_voice->id = gNextVoiceId++;
    memcpy(gSentGrains[gSentWrite], free_voice->buf, sizeof(free_voice->buf));
    gSentIds[gSentWrite] = free_voice->id;
    gSentWrite = (gSentWrite + 1) % kSentLogCount;
    if (gSentCount < kSentLogCount) {
      ++gSentCount;
    }
    if (gNextVoiceId == 0) {
      gNextVoiceId = 1;
    }
    free_voice->active = true;
    ++gGrainsPlayed;
  }
}

void audioTask(void*) {
  const float record_hp_coef = 1.0f - expf(-kTwoPi * kRecordHighPassHz / static_cast<float>(kRecordRateHz));
  constexpr size_t kRecordDivider = kAudioSampleRate / kRecordRateHz;  // 32
  static int16_t voice_chunk[kAudioChunkFrames];
  static int32_t mix[kAudioChunkFrames];
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    for (size_t s = 0; s < kAudioChunkFrames / kRecordDivider; ++s) {
      recordStep(record_hp_coef);  // 8 recording steps per 16ms chunk = 500Hz
    }
    startQueuedGrains();

    std::memset(mix, 0, sizeof(mix));
    bool any_gate = false;
    for (auto& voice : gTxVoices) {
      if (!voice.active) {
        continue;
      }
      bool gate;
      if (voice.pos < static_cast<float>(kWindowSamples - 1)) {
        const float speed = gPlaybackSpeed;
        for (size_t i = 0; i < kAudioChunkFrames; ++i) {
          if (voice.pos < static_cast<float>(kWindowSamples - 1)) {
            const size_t idx = static_cast<size_t>(voice.pos);
            const float frac = voice.pos - static_cast<float>(idx);
            voice_chunk[i] = static_cast<int16_t>(static_cast<float>(voice.buf[idx]) * (1.0f - frac) +
                                                  static_cast<float>(voice.buf[idx + 1]) * frac);
            voice.pos += speed;
          } else {
            voice_chunk[i] = 0;
          }
        }
        gate = true;
      } else {
        // Gate-off tail: keeps the receiver's stream alive through release.
        std::memset(voice_chunk, 0, sizeof(voice_chunk));
        gate = false;
        if (voice.tail > 0) {
          --voice.tail;
        }
        if (voice.tail == 0) {
          voice.active = false;
        }
      }
      sendUdpChunks(voice_chunk, gate, voice.id);
      for (size_t i = 0; i < kAudioChunkFrames; ++i) {
        mix[i] += voice_chunk[i];
      }
      any_gate = any_gate || gate;
    }

    int16_t* chunk = gAudioChunks[gAudioChunkIndex];
    gAudioChunkIndex = (gAudioChunkIndex + 1) % kAudioChunkBuffers;
    int16_t peak = 0;
    for (size_t i = 0; i < kAudioChunkFrames; ++i) {
      chunk[i] = static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, mix[i])));
      const int16_t magnitude = chunk[i] < 0 ? -chunk[i] : chunk[i];
      if (magnitude > peak) {
        peak = magnitude;
      }
    }
    gAudioPeak = peak;
    gGate = any_gate;
    // Local monitor on the built-in speaker (mixed).
    if (gSpeakerReady && M5.Speaker.isPlaying(0) < 2) {
      M5.Speaker.playRaw(chunk, kAudioChunkFrames, kAudioSampleRate, false, 1, 0, false);
    }
    // 256 frames @16kHz = 16ms cadence; receiver ring absorbs jitter.
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(16));
  }
}

void updateDisplacementFromSample(const Sample& sample) {
  if (sample.valid && sample.distanceMm <= kGateMaxMm) {
    // Median-of-3 rejects the sensor's occasional single-sample outliers.
    gMedianBuf[gMedianIndex] = static_cast<float>(sample.distanceMm);
    gMedianIndex = (gMedianIndex + 1) % 3;
    const float a = gMedianBuf[0];
    const float b = gMedianBuf[1];
    const float c = gMedianBuf[2];
    const float median = std::max(std::min(a, b), std::min(std::max(a, b), c));
    gDisplacementMm = median;
    const size_t newest = (gEvHead + kInterpEvents - 1) % kInterpEvents;
    if (gEvCount == 0 || median != gEvV[newest]) {
      // A real new sensor value: append to the spline event ring.
      taskENTER_CRITICAL(&gInterpMux);
      gEvV[gEvHead] = median;
      gEvT[gEvHead] = micros();
      gEvHead = (gEvHead + 1) % kInterpEvents;
      if (gEvCount < kInterpEvents) {
        ++gEvCount;
      }
      taskEXIT_CRITICAL(&gInterpMux);
    }
  }
  else {
    gLastInvalidUs = micros();
  }
  // Invalid readings hold the previous level; the recorder's high-pass
  // keeps the held value from becoming an audible step.
}

Sample acquireSample() {
  switch (gSensorType) {
    case SensorType::Vl53L0X:
      return readVl53Sample();
    case SensorType::Vl6180X:
      return readVl6180Sample();
    case SensorType::Vl53L4CD:
      return readVl53L4CdSample();
    case SensorType::Gp2y0e03:
      return readGp2ySample();
    default:
      return {};
  }
}

void appendHistory(const Sample& sample) {
  const size_t index = (gHistoryHead + gHistoryCount) % kHistorySize;
  gHistory[index] = sample;
  if (gHistoryCount < kHistorySize) {
    ++gHistoryCount;
  } else {
    gHistoryHead = (gHistoryHead + 1) % kHistorySize;
  }
}

void updateSampleRate() {
  const uint32_t now = millis();
  if (gSampleRateWindowStartMs == 0) {
    gSampleRateWindowStartMs = now;
  }

  ++gSampleCountInWindow;
  const uint32_t elapsedMs = now - gSampleRateWindowStartMs;
  if (elapsedMs >= kSampleRateIntervalMs) {
    gSampleRateHz = (1000.0f * static_cast<float>(gSampleCountInWindow)) /
                    static_cast<float>(elapsedMs);
    gSampleCountInWindow = 0;
    gSampleRateWindowStartMs = now;
  }
}

float samplePeriodMs() {
  if (gSampleRateHz <= 0.0f) {
    return 0.0f;
  }
  return 1000.0f / gSampleRateHz;
}

template <typename T>
void drawGraphPanel(T& gfx, int32_t x, int32_t y, int32_t w, int32_t h) {
  gfx.drawRect(x, y, w, h, kColorGrid);
  if (gHistoryCount < 2) {
    return;
  }

  float minValue = 1.0e9f;
  float maxValue = -1.0e9f;
  for (size_t i = 0; i < gHistoryCount; ++i) {
    const Sample& sample = gHistory[(gHistoryHead + i) % kHistorySize];
    if (!sample.valid) {
      continue;
    }
    minValue = std::min(minValue, sample.distanceMm);
    maxValue = std::max(maxValue, sample.distanceMm);
  }

  if (minValue > maxValue) {
    minValue = 0.0f;
    maxValue = 1.0f;
  } else if (maxValue - minValue < 0.1f) {
    maxValue = minValue + 0.1f;
  }

  const int32_t innerX = x + 1;
  const int32_t innerY = y + 1;
  const int32_t innerW = w - 2;
  const int32_t innerH = h - 2;

  for (size_t i = 1; i < gHistoryCount; ++i) {
    const Sample& prev = gHistory[(gHistoryHead + i - 1) % kHistorySize];
    const Sample& curr = gHistory[(gHistoryHead + i) % kHistorySize];
    if (!prev.valid || !curr.valid) {
      continue;
    }

    const float range = maxValue - minValue;
    const int32_t x0 = innerX + static_cast<int32_t>(((i - 1) * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    const int32_t x1 = innerX + static_cast<int32_t>((i * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    const int32_t y0 = innerY + innerH - 1
                     - static_cast<int32_t>(((prev.distanceMm - minValue) * static_cast<float>(innerH - 1)) / range);
    const int32_t y1 = innerY + innerH - 1
                     - static_cast<int32_t>(((curr.distanceMm - minValue) * static_cast<float>(innerH - 1)) / range);
    gfx.drawLine(x0, y0, x1, y1, kColorDistance);
  }

  for (size_t i = 0; i < gHistoryCount; ++i) {
    const Sample& sample = gHistory[(gHistoryHead + i) % kHistorySize];
    if (!sample.timeout) {
      continue;
    }
    const int32_t px = innerX + static_cast<int32_t>((i * (innerW - 1)) / max<size_t>(1, gHistoryCount - 1));
    gfx.drawFastVLine(px, innerY, innerH, kColorTimeout);
  }

  gfx.setTextColor(kColorText, kColorBg);
  gfx.setCursor(x + 4, y + 2);
  gfx.printf("mm %.1f-%.1f", static_cast<double>(minValue), static_cast<double>(maxValue));
}

void updateDisplay() {
  const uint32_t now = millis();
  if (gDisplayDirtySamples < kDisplayRefreshBatchCount || now - gLastDisplayMs < kDisplayIntervalMs) {
    return;
  }
  gLastDisplayMs = now;
  gDisplayDirtySamples = 0;

  auto& gfx = gCanvasReady ? static_cast<lgfx::LGFXBase&>(gCanvas) : static_cast<lgfx::LGFXBase&>(M5.Display);
  gfx.fillScreen(kColorBg);
  gfx.setTextColor(kColorText, kColorBg);
  gfx.setCursor(0, 0);
    gfx.printf("TOF %s\n", sensorName());
    gfx.printf("bus: %s\n", gBusLabel);
    gfx.printf("latest: %.1f mm %s\n", static_cast<double>(gLatestSample.distanceMm),
               gLatestSample.timeout ? "timeout" : "ok");
    gfx.printf("valid: %s rate: %.1fHz %.2fms\n", gLatestSample.valid ? "yes" : "no",
               static_cast<double>(gSampleRateHz), static_cast<double>(samplePeriodMs()));
    gfx.printf("env:%.1fmm on:%lu gr:%lu/%lu\n", static_cast<double>(gEnvFastView),
               static_cast<unsigned long>(gOnsetCount), static_cast<unsigned long>(gGrainsPlayed),
               static_cast<unsigned long>(gGrainsDropped));
    gfx.printf("wifi:%s gate:%s peak:%d\n", gWifiOk ? "OK" : "--", gGate ? "ON" : "off",
               static_cast<int>(gAudioPeak));
  gfx.println("BtnA: re-detect");

  // Status lamps: red = capture window in progress, green = grain playing.
  if (gPendingOnsetCount > 0) {
    gfx.fillCircle(M5.Display.width() - 14, 14, 9, kColorTimeout);
  }
  if (gGate) {
    gfx.fillCircle(M5.Display.width() - 36, 14, 9, kColorDistance);
  }

  drawGraphPanel(gfx, 0, 48, M5.Display.width(), M5.Display.height() - 48);

  if (gCanvasReady) {
    gCanvas.pushSprite(0, 0);
  }
}

void printStatus() {
  const uint32_t now = millis();
  if (now - gLastStatusMs < kStatusIntervalMs) {
    return;
  }
  gLastStatusMs = now;

  Serial.print("tof=");
  Serial.print(sensorName());
  Serial.print(" dist=");
  Serial.print(gLatestSample.distanceMm);
  Serial.print(" valid=");
  Serial.print(gLatestSample.valid ? "yes" : "no");
  Serial.print(" timeout=");
  Serial.print(gLatestSample.timeout ? "yes" : "no");
  Serial.print(" sampleHz=");
  Serial.print(gSampleRateHz, 1);
  Serial.print(" sampleMs=");
  Serial.print(samplePeriodMs(), 2);
  Serial.print(" wifi=");
  Serial.print(gWifiOk ? "ok" : "--");
  Serial.print(" gate=");
  Serial.print(gGate ? "on" : "off");
  Serial.print(" audioPeak=");
  Serial.print(static_cast<int>(gAudioPeak));
  Serial.print(" env=");
  Serial.print(gEnvFastView, 2);
  Serial.print(" onsets=");
  Serial.print(gOnsetCount);
  Serial.print(" grains=");
  Serial.print(gGrainsPlayed);
  Serial.print(" dropped=");
  Serial.print(gGrainsDropped);
  if (gSensorType == SensorType::Gp2y0e03) {
    const uint32_t changes = gGp2yRawChanges - gGp2yLastRawChanges;
    gGp2yLastRawChanges = gGp2yRawChanges;
    gGp2yRawChangeHz = (1000.0f * static_cast<float>(changes)) / static_cast<float>(kStatusIntervalMs);
    Serial.printf(" raw=0x%02X_%02X rawChangeHz=%.0f analogMv=%u bus=%s", gGp2yRawMsb, gGp2yRawLsb,
                  static_cast<double>(gGp2yRawChangeHz),
                  static_cast<unsigned>(analogReadMilliVolts(kGp2yAnalogPin)), gBusLabel);
  }
  Serial.println();
}

void execSerialCommand(const char* command) {
  if (strcmp(command, "x") == 0) {
    gSimAmplitudeMm = 5.0f;
    Serial.println("[CMD] simulated droplet injected (5mm, 8Hz)");
  } else if (strcmp(command, "X") == 0) {
    gSimAmplitudeMm = 15.0f;
    Serial.println("[CMD] big simulated droplet injected (15mm, 8Hz)");
  } else if (strcmp(command, "t+") == 0) {
    gOnsetThresholdMm = gOnsetThresholdMm * 1.5f;
    Serial.printf("[CMD] threshold=%.2fmm\n", static_cast<double>(gOnsetThresholdMm));
  } else if (strcmp(command, "t-") == 0) {
    gOnsetThresholdMm = std::max(0.05f, gOnsetThresholdMm / 1.5f);
    Serial.printf("[CMD] threshold=%.2fmm\n", static_cast<double>(gOnsetThresholdMm));
  } else if (strcmp(command, "g+") == 0) {
    gOutputGain = gOutputGain * 1.5f;
    Serial.printf("[CMD] gain=%.0f\n", static_cast<double>(gOutputGain));
  } else if (strcmp(command, "g-") == 0) {
    gOutputGain = std::max(100.0f, gOutputGain / 1.5f);
    Serial.printf("[CMD] gain=%.0f\n", static_cast<double>(gOutputGain));
  } else if (strcmp(command, "r") == 0) {
    gRawCapStartUs = micros();
    gRawCapIndex = 0;
    Serial.println("[RAWCAP] start (~6s, auto-dump when full)");
  } else if (strcmp(command, "s+") == 0) {
    gPlaybackSpeed = std::min(1.0f, gPlaybackSpeed * 1.25f);
    Serial.printf("[CMD] playback speed=%.3fx (%.0fms)\n", static_cast<double>(gPlaybackSpeed),
                  static_cast<double>(kWindowSamples / gPlaybackSpeed / 16.0f));
  } else if (strcmp(command, "s-") == 0) {
    gPlaybackSpeed = std::max(0.05f, gPlaybackSpeed / 1.25f);
    Serial.printf("[CMD] playback speed=%.3fx (%.0fms)\n", static_cast<double>(gPlaybackSpeed),
                  static_cast<double>(kWindowSamples / gPlaybackSpeed / 16.0f));
  } else if (strcmp(command, "l+") == 0) {
    gRecordLpHz = std::min(240.0f, gRecordLpHz * 1.5f);
    Serial.printf("[CMD] record LP=%.0fHz\n", static_cast<double>(gRecordLpHz));
  } else if (strcmp(command, "l-") == 0) {
    gRecordLpHz = std::max(10.0f, gRecordLpHz / 1.5f);
    Serial.printf("[CMD] record LP=%.0fHz\n", static_cast<double>(gRecordLpHz));
  } else if (strcmp(command, "f") == 0) {
    gAccumRequest = 0x00;
    Serial.println("[CMD] requested accumulation=1 (fast)");
  } else if (strcmp(command, "a") == 0) {
    gAccumRequest = 0x01;
    Serial.println("[CMD] requested accumulation=5 (accurate)");
  } else if (strcmp(command, "w") == 0) {
    if (gSentCount == 0) {
      Serial.println("[TXGRAIN] none");
    } else {
      // Oldest first. These samples are the exact UDP payload (normalized,
      // faded, master volume applied) played back at 16kHz.
      for (size_t k = 0; k < gSentCount; ++k) {
        const size_t slot = (gSentWrite + kSentLogCount - gSentCount + k) % kSentLogCount;
        Serial.printf("[TXGRAIN] index=%u id=%u n=%u unit=int16pcm playrate=16000\n", static_cast<unsigned>(k),
                      static_cast<unsigned>(gSentIds[slot]), static_cast<unsigned>(kWindowSamples));
        for (size_t i = 0; i < kWindowSamples; i += 10) {
          for (size_t j = i; j < i + 10 && j < kWindowSamples; ++j) {
            Serial.print(gSentGrains[slot][j]);
            if (j + 1 < i + 10 && j + 1 < kWindowSamples) {
              Serial.print(',');
            }
          }
          Serial.println();
        }
        Serial.println("[TXGRAIN_END]");
      }
      Serial.println("[TXGRAIN_ALL_END]");
    }
  } else if (strcmp(command, "p") == 0) {
    Serial.printf("[PARAMS] threshold=%.2fmm gain=%.0f lp=%.0fHz env=%.2f envSlow=%.2f onsets=%lu grains=%lu\n",
                  static_cast<double>(gOnsetThresholdMm), static_cast<double>(gOutputGain),
                  static_cast<double>(gRecordLpHz), static_cast<double>(gEnvFastView),
                  static_cast<double>(gEnvSlow), static_cast<unsigned long>(gOnsetCount),
                  static_cast<unsigned long>(gGrainsPlayed));
  } else {
    Serial.println("[CMD] x=sim droplet X=big t+/t-=threshold g+/g-=gain l+/l-=LP f=fast a=accurate w=dump p=params");
  }
}

void processSerialCommands() {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (gSerialCmdLen > 0) {
        gSerialCmd[gSerialCmdLen] = '\0';
        gSerialCmdLen = 0;
        execSerialCommand(gSerialCmd);
      }
    } else if (gSerialCmdLen + 1 < sizeof(gSerialCmd)) {
      gSerialCmd[gSerialCmdLen++] = static_cast<char>(c);
    }
  }
}

void reprobeSensor() {
  if (gSensorType == SensorType::Vl53L0X) {
    gVl53.stopContinuous();
  }
  if (gSensorType == SensorType::Vl53L4CD) {
    gVl53L4cd.VL53L4CD_StopRanging();
  }
  detectSensor();
  gLatestSample = {};
  gHistoryHead = 0;
  gHistoryCount = 0;
  gDisplayDirtySamples = 0;
  gSampleRateWindowStartMs = millis();
  gSampleCountInWindow = 0;
  gSampleRateHz = 0.0f;
  Serial.print("sensor=");
  Serial.println(sensorName());
}

// All sensor I2C access lives in a dedicated task so display rendering and
// serial dumps can never interrupt the acquisition stream (they used to
// block loop() for tens of ms, punching visible holes in the recording).
void sensorTask(void*) {
  uint32_t historyDecimationCounter = 0;
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    if (gReprobeRequest) {
      gReprobeRequest = false;
      reprobeSensor();
    }
    if (gAccumRequest >= 0) {
      if (gSensorType == SensorType::Gp2y0e03 &&
          writeGp2yReg8(0xA8, static_cast<uint8_t>(gAccumRequest))) {
        Serial.printf("[CMD] GP2Y accumulation reg=0x%02X applied\n", gAccumRequest);
      }
      gAccumRequest = -1;
    }

    if (gSensorType != SensorType::None) {
      gLatestSample = acquireSample();
      updateSampleRate();
      updateDisplacementFromSample(gLatestSample);

      const uint32_t decimation = (gSensorType == SensorType::Gp2y0e03) ? kGp2yHistoryDecimation : 1;
      if (++historyDecimationCounter >= decimation) {
        historyDecimationCounter = 0;
        appendHistory(gLatestSample);
        ++gDisplayDirtySamples;
      }
    } else {
      gLatestSample = {};
      gGate = false;
    }

    vTaskDelayUntil(&wake, 1);  // 1ms cadence (1kHz tick)
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextSize(1);
  gCanvas.setColorDepth(16);
  gCanvasReady = gCanvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  if (gCanvasReady) {
    gCanvas.setTextSize(1);
  }

  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(kGp2yAnalogPin, ADC_11db);

  Serial.println();
  Serial.println("M5StickS3 ToF displacement-to-audio (UDP -> Tab5Synth)");
  gSampleRateWindowStartMs = millis();
  detectSensor();
  Serial.print("sensor=");
  Serial.println(sensorName());

  if (M5.Speaker.isEnabled()) {
    M5.Speaker.begin();
    M5.Speaker.setVolume(128);
    gSpeakerReady = true;
    Serial.println("speaker ready");
  } else {
    Serial.println("speaker not available");
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(kApSsid, kApPassword);
  xTaskCreate(audioTask, "audio_tx", 4096, nullptr, 3, &gAudioTask);
  xTaskCreate(sensorTask, "sensor_poll", 4096, nullptr, 4, &gSensorTask);
}

void loop() {
  M5.update();
  processSerialCommands();

  if (gRawCapDumpPending) {
    gRawCapDumpPending = false;
    Serial.printf("[RAWCAP] n=%u unit=raw12bit fmt=dt_us,raw\n", static_cast<unsigned>(kRawCapCount));
    for (size_t i = 0; i < kRawCapCount; ++i) {
      Serial.print(gRawCapTime[i]);
      Serial.print(':');
      Serial.print(gRawCapVal[i]);
      Serial.println();
    }
    Serial.println("[RAWCAP_END]");
  }

  if (M5.BtnA.wasPressed()) {
    gReprobeRequest = true;
  }

  const bool wifiOk = WiFi.status() == WL_CONNECTED;
  if (wifiOk != gWifiOk) {
    gWifiOk = wifiOk;
    Serial.printf("[WIFI] %s ip=%s\n", wifiOk ? "connected" : "disconnected",
                  wifiOk ? WiFi.localIP().toString().c_str() : "-");
  }

  printStatus();
  updateDisplay();
}
