#pragma once
#include "save.h"

// Silent copy of the NVS save onto the microSD. The blob is ~2 KB, so a write
// is milliseconds -- no prompt, no battery gate. latest.tkps is always the
// newest; a dated file is kept per day so yesterday still exists if today's
// write dies mid-card.

#define SAVE_SD_DIR    "/saves"
#define SAVE_SD_LATEST "/saves/latest.tkps"

void saveSdBegin();                 // mkdir after sdBegin()
bool saveSdWrite(bool force);       // export + write; force ignores the throttle
void saveSdPoll();                  // after an NVS flush; cheap if nothing new
bool saveSdHasBackup();
bool saveSdPeek(SaveSummary *out);
bool saveSdLoad(const char *name);  // nullptr = latest.tkps; true = imported
