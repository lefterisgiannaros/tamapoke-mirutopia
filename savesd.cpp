#include "savesd.h"
#include "sdmon.h"
#include "pet.h"
#include <SD_MMC.h>
#include <time.h>
#include <string.h>

extern Pet pet;

#define SAVE_SD_KEEP_DAYS 14
#define SAVE_SD_THROTTLE_MS 60000UL

static uint32_t lastWriteMs = 0;
static uint32_t lastWriteDay = 0;

static bool writeBlob(const char *path, const uint8_t *buf, size_t n) {
  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return false;
  size_t w = f.write(buf, n);
  f.close();
  return w == n;
}

static bool readBlob(const char *path, uint8_t *buf, size_t cap, size_t *outN) {
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  size_t n = f.size();
  if (n < SAVE_HDR + 2 || n > cap) { f.close(); return false; }
  if (f.read(buf, n) != n) { f.close(); return false; }
  f.close();
  *outN = n;
  return true;
}

static uint32_t dayOf(uint32_t epoch) { return epoch ? epoch / 86400 : 0; }

static void datedPath(char *out, size_t cap, uint32_t epoch) {
  time_t tt = epoch;
  struct tm tmv;
  gmtime_r(&tt, &tmv);
  snprintf(out, cap, SAVE_SD_DIR "/%04d%02d%02d.tkps",
           tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
}

static void pruneOld(uint32_t today) {
  if (!today) return;
  File dir = SD_MMC.open(SAVE_SD_DIR);
  if (!dir) return;
  File e;
  while ((e = dir.openNextFile())) {
    const char *nm = e.name();
    e.close();
    if (!nm) continue;
    const char *base = strrchr(nm, '/');
    base = base ? base + 1 : nm;
    if (strlen(base) != 13 || strcmp(base + 8, ".tkps") != 0) continue;
    bool digits = true;
    for (int i = 0; i < 8; i++) if (base[i] < '0' || base[i] > '9') digits = false;
    if (!digits) continue;
    int y = (base[0] - '0') * 1000 + (base[1] - '0') * 100 +
            (base[2] - '0') * 10 + (base[3] - '0');
    int mo = (base[4] - '0') * 10 + (base[5] - '0');
    int d = (base[6] - '0') * 10 + (base[7] - '0');
    if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31) continue;
    struct tm tmv = {};
    tmv.tm_year = y - 1900;
    tmv.tm_mon = mo - 1;
    tmv.tm_mday = d;
    time_t t = mktime(&tmv);
    if (t <= 0) continue;
    uint32_t day = (uint32_t)t / 86400;
    if (today > day && today - day > SAVE_SD_KEEP_DAYS) {
      char path[40];
      snprintf(path, sizeof(path), SAVE_SD_DIR "/%s", base);
      SD_MMC.remove(path);
    }
  }
  dir.close();
}

void saveSdBegin() {
  if (!sdReady) return;
  SD_MMC.mkdir(SAVE_SD_DIR);
}

bool saveSdWrite(bool force) {
  if (!sdReady) return false;
  uint32_t now = millis();
  uint32_t day = dayOf(pet.lastSeenEpoch);
  if (!force && lastWriteMs && now - lastWriteMs < SAVE_SD_THROTTLE_MS &&
      day == lastWriteDay)
    return true;
  static uint8_t buf[SAVE_BLOB_MAX];
  size_t n = saveExport(buf, sizeof(buf));
  if (!n) return false;
  SD_MMC.mkdir(SAVE_SD_DIR);
  if (!writeBlob(SAVE_SD_LATEST, buf, n)) return false;
  if (pet.lastSeenEpoch) {
    char dated[40];
    datedPath(dated, sizeof(dated), pet.lastSeenEpoch);
    writeBlob(dated, buf, n);
    pruneOld(day);
  }
  lastWriteMs = now ? now : 1;
  lastWriteDay = day;
  return true;
}

void saveSdPoll() {
  if (!sdReady) return;
  saveSdWrite(false);
}

bool saveSdHasBackup() {
  SaveSummary s;
  return saveSdPeek(&s);
}

bool saveSdPeek(SaveSummary *out) {
  if (!out || !sdReady) return false;
  static uint8_t buf[SAVE_BLOB_MAX];
  size_t n = 0;
  if (readBlob(SAVE_SD_LATEST, buf, sizeof(buf), &n) && savePeek(buf, n, out))
    return true;

  File dir = SD_MMC.open(SAVE_SD_DIR);
  if (!dir) return false;
  char best[40] = "";
  File e;
  while ((e = dir.openNextFile())) {
    const char *nm = e.name();
    size_t sz = e.size();
    e.close();
    if (!nm || sz < SAVE_HDR + 2 || sz > SAVE_BLOB_MAX) continue;
    const char *base = strrchr(nm, '/');
    base = base ? base + 1 : nm;
    if (strlen(base) != 13 || strcmp(base + 8, ".tkps") != 0) continue;
    if (!best[0] || strcmp(base, best) > 0) snprintf(best, sizeof(best), "%s", base);
  }
  dir.close();
  if (!best[0]) return false;
  char path[48];
  snprintf(path, sizeof(path), SAVE_SD_DIR "/%s", best);
  if (!readBlob(path, buf, sizeof(buf), &n)) return false;
  return savePeek(buf, n, out);
}

bool saveSdLoad(const char *name) {
  if (!sdReady) return false;
  char path[48];
  if (!name || !name[0]) snprintf(path, sizeof(path), "%s", SAVE_SD_LATEST);
  else if (name[0] == '/') snprintf(path, sizeof(path), "%s", name);
  else snprintf(path, sizeof(path), SAVE_SD_DIR "/%s", name);
  if (strncmp(path, SAVE_SD_DIR "/", 7) != 0) return false;
  static uint8_t buf[SAVE_BLOB_MAX];
  size_t n = 0;
  if (!readBlob(path, buf, sizeof(buf), &n)) return false;
  return saveImport(buf, n);
}
