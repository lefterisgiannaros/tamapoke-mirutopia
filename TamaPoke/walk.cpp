#include "walk.h"
#include "pet.h"
#include "audio.h"
#include "pin_config.h"
#include <Wire.h>
#include <math.h>
#include <SensorQMI8658.hpp>

extern Pet pet;

#define WALK_GIFT_STEPS 400
#define WALK_GIFTS_MAX  2
#define WALK_POLL_MS    120

static SensorQMI8658 imu;
static bool imuUp = false;
static uint32_t hwAtDay = 0;
static uint32_t lastPoll = 0;
static uint32_t lastSoft = 0;
static bool softArmed = true;

static void rollover() {
  uint32_t day = pet.lastSeenEpoch ? pet.lastSeenEpoch / 86400 : 0;
  if (day && day != pet.walkDay) {
    pet.walkDay = day;
    pet.walkSteps = 0;
    pet.walkGifts = 0;
    if (imuUp) hwAtDay = imu.getPedometerCounter();
    pet.saveNow();
  }
}

static bool imuStart() {
  if (imuUp) return true;
  bool ok = imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!ok) ok = imu.begin(Wire, QMI8658_H_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!ok) {
    Serial.println("QMI8658 not found (walk uses tap-safe software fallback)");
    imuUp = false;
    return false;
  }
  imu.configAccelerometer(SensorQMI8658::ACC_RANGE_2G, SensorQMI8658::ACC_ODR_62_5Hz);
  imu.enableAccelerometer();
  imu.configPedometer(50, 200, 100, 200, 20, 10, 0, 4);
  imu.enablePedometer(SensorQMI8658::INTERRUPT_PIN_DISABLE);
  hwAtDay = imu.getPedometerCounter();
  imuUp = true;
  Serial.printf("QMI8658 walk on, hw=%u\n", (unsigned)hwAtDay);
  return true;
}

static void imuStop() {
  if (!imuUp) return;
  imu.disablePedometer();
  imuUp = false;
}

static void grantOne() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  if (pet.walkGifts >= WALK_GIFTS_MAX) return;
  uint32_t need = (uint32_t)(pet.walkGifts + 1) * WALK_GIFT_STEPS;
  if (pet.walkSteps < need) return;

  uint8_t color = (uint8_t)random(3);
  if (pet.berryKnown) {
    for (uint8_t i = 0; i < 3; i++)
      if (pet.lovesBerry(i)) { color = i; break; }
  }
  pet.feedBerry(color);
  pet.walkGifts++;
  pet.walkGiftUntil = millis() + 3500;
  if (!pet.walkMedal) {
    pet.earnWalkMedal();
    sfxPlay(SFX_MEDAL);
  } else {
    sfxPlay(SFX_HEART);
  }
  pet.saveNow();
}

static void addSteps(uint32_t n) {
  if (!n) return;
  if (n > 40) n = 40;          // one poll cannot be a sprint; kills shake-dumps
  pet.walkSteps += n;
  grantOne();
}

static void pollHardware() {
  if (!imuUp) return;
  uint32_t hw = imu.getPedometerCounter();
  if (hw < hwAtDay) hwAtDay = hw;     // chip reset
  uint32_t have = hw - hwAtDay;
  if (have > pet.walkSteps) addSteps(have - pet.walkSteps);
}

static void pollSoftware() {
  if (!imuUp) return;
  if (pet.walkSteps > 0 && imu.getPedometerCounter() > hwAtDay)
    return;                             // hardware is alive; do not double-count
  float x, y, z;
  if (!imu.getAccelerometer(x, y, z)) return;
  float mag = sqrtf(x * x + y * y + z * z);
  uint32_t now = millis();
  if (mag < 1.08f) softArmed = true;
  if (softArmed && mag > 1.28f && now - lastSoft > 320) {
    softArmed = false;
    lastSoft = now;
    addSteps(1);
  }
}

void walkBegin() {
  if (pet.walkOn) imuStart();
}

void walkPoll() {
  if (!pet.walkOn) return;
  uint32_t now = millis();
  if (now - lastPoll < WALK_POLL_MS) return;
  lastPoll = now;
  rollover();
  if (!imuUp) imuStart();
  pollHardware();
  pollSoftware();
}

void walkSetEnabled(bool on) {
  pet.walkOn = on;
  if (on) {
    if (!pet.walkDay && pet.lastSeenEpoch) pet.walkDay = pet.lastSeenEpoch / 86400;
    imuStart();
  } else {
    imuStop();
  }
  pet.saveNow();
}

bool walkEnabled() { return pet.walkOn; }

void walkToggle() { walkSetEnabled(!pet.walkOn); }

uint32_t walkStepsToday() { return pet.walkSteps; }

bool walkShowGift() { return pet.walkGiftUntil && millis() < pet.walkGiftUntil; }

void walkAddDebug(uint16_t n) {
  if (!pet.walkOn) walkSetEnabled(true);
  rollover();
  addSteps(n);
}
