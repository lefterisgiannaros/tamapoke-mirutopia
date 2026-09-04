#pragma once
#include <Arduino.h>

// Pocket walk. Bonus only: never drains, never gates level or evolution.
// Off by default -- the IMU is left asleep until the player turns it on.
void walkBegin();                 // after pet.begin()
void walkPoll();                  // cheap; call from loop
void walkSetEnabled(bool on);
bool walkEnabled();
void walkToggle();
uint32_t walkStepsToday();
bool walkShowGift();
void walkAddDebug(uint16_t n);    // serial WALK +N
