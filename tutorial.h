#pragma once
#include <Arduino.h>

// Prof. Tangrowth first-boot lecture. Runs once on a new save (starter not
// chosen yet), or again from the serial TUTOR command. Name / region / starter
// interrupt the talk and reuse the existing pickers.

enum TutView : uint8_t {
  TUTVIEW_OFF = 0,
  TUTVIEW_TALK,      // professor + header + dialogue
  TUTVIEW_REGION,    // existing region chooser
  TUTVIEW_STARTER    // existing starter list
};

bool tutorialActive();
TutView tutorialView();
void tutorialRender();
bool tutorialTap(int16_t x, int16_t y);   // true if it ate the tap
void tutorialOnNameDone();
void tutorialOnRegion(uint8_t region);
void tutorialOnStarter(int16_t dex);
void tutorialReplay();                    // serial TUTOR: play it again
