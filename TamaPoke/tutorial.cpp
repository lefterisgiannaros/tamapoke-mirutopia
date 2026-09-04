#include "tutorial.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "species.h"
#include "pet.h"
#include "audio.h"
#include "sdmon.h"
#include <SD_MMC.h>

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool starterRegionDone;
void openKeyboardFor(uint8_t target);

#define CX 233
#define TUT_ART_W 220
#define TUT_ART_H 220
#define TUT_ART_X ((LCD_WIDTH - TUT_ART_W) / 2)
#define TUT_ART_Y 46
#define TUT_SKIP_X 110
#define TUT_SKIP_Y 36
#define TUT_SKIP_W 72
#define TUT_SKIP_H 30
#define TUT_BOX_X 73
#define TUT_BOX_Y 298
#define TUT_BOX_W 320
#define TUT_BOX_H 122
#define TUT_BEGIN_X 93
#define TUT_BEGIN_Y 368
#define TUT_BEGIN_W 280
#define TUT_BEGIN_H 52

enum : uint8_t {
  ACT_NONE = 0,
  ACT_NAME,
  ACT_REGION,
  ACT_STARTER,
  ACT_BEGIN,
  ACT_HEADER
};
#define FL_FMT 0x01   // l1 is a printf format with the trainer name

struct TutStep {
  const char *header;   // section title, or nullptr to keep the last one
  const char *l1;
  const char *l2;
  uint8_t act;
  uint8_t flags;
};

// Bitmap font has no accents. Section headers get their own card.
static const TutStep SCRIPT[] = {
  { "NEW TRAINER", nullptr, nullptr, ACT_HEADER, 0 },
  { "NEW TRAINER", "Hello there!", nullptr, ACT_NONE, 0 },
  { "NEW TRAINER", "Welcome to the", "world of Pokemon!", ACT_NONE, 0 },
  { "NEW TRAINER", "Before we begin,", "tell me your name.", ACT_NAME, 0 },
  { "NEW TRAINER", "Ah! So you're %s!", nullptr, ACT_NONE, FL_FMT },
  { "NEW TRAINER", "Every Trainer has a", "place they call home.", ACT_NONE, 0 },
  { "NEW TRAINER", "Which region will", "you explore?", ACT_REGION, 0 },
  { "NEW TRAINER", "Excellent choice!", nullptr, ACT_NONE, 0 },
  { "NEW TRAINER", "Every great Trainer", "needs a partner.", ACT_NONE, 0 },
  { "NEW TRAINER", "Which Pokemon will", "you choose?", ACT_STARTER, 0 },
  { "NEW TRAINER", "A fine choice!", nullptr, ACT_NONE, 0 },
  { "NEW TRAINER", "From here on, the two", "of you will grow together.", ACT_NONE, 0 },

  { "YOUR POKEMON", nullptr, nullptr, ACT_HEADER, 0 },
  { "YOUR POKEMON", "Taking care of a Pokemon", "is a Trainer's first duty.", ACT_NONE, 0 },
  { "YOUR POKEMON", "Your Pokemon has", "four needs:", ACT_NONE, 0 },
  { "YOUR POKEMON", "FOOD.  JOY.", "ENERGY.  HYGIENE.", ACT_NONE, 0 },
  { "YOUR POKEMON", "Keep an eye on them!", nullptr, ACT_NONE, 0 },
  { "YOUR POKEMON", "A hungry or unhappy", "Pokemon won't do its best.", ACT_NONE, 0 },
  { "YOUR POKEMON", "And don't forget to", "clean up after it!", ACT_NONE, 0 },

  { "FEEDING", nullptr, nullptr, ACT_HEADER, 0 },
  { "FEEDING", "Let's start with food.", nullptr, ACT_NONE, 0 },
  { "FEEDING", "Choose FEED to give", "your Pokemon a Berry.", ACT_NONE, 0 },
  { "FEEDING", "There are three", "kinds of Berry.", ACT_NONE, 0 },
  { "FEEDING", "Every Pokemon has", "a favorite.", ACT_NONE, 0 },
  { "FEEDING", "Find it, and your Pokemon", "will be especially happy!", ACT_NONE, 0 },
  { "FEEDING", "You can also", "give it Candy.", ACT_NONE, 0 },
  { "FEEDING", "But too much Candy", "makes it gain weight.", ACT_NONE, 0 },

  { "PLAYING", nullptr, nullptr, ACT_HEADER, 0 },
  { "PLAYING", "Pokemon need fun, too!", nullptr, ACT_NONE, 0 },
  { "PLAYING", "Choose TRAIN to", "play together.", ACT_NONE, 0 },
  { "PLAYING", "Playing raises JOY", "and helps it stay fit.", ACT_NONE, 0 },
  { "PLAYING", "Keep an eye on", "its ENERGY, though!", ACT_NONE, 0 },

  { "PETTING", nullptr, nullptr, ACT_HEADER, 0 },
  { "PETTING", "Your Pokemon loves", "attention.", ACT_NONE, 0 },
  { "PETTING", "Tap your Pokemon", "to pet it.", ACT_NONE, 0 },
  { "PETTING", "A little affection", "goes a long way!", ACT_NONE, 0 },
  { "PETTING", "The bond between Trainer", "and Pokemon matters.", ACT_NONE, 0 },

  { "BATH", nullptr, nullptr, ACT_HEADER, 0 },
  { "BATH", "Oh my!", nullptr, ACT_NONE, 0 },
  { "BATH", "Looks like someone", "needs a bath!", ACT_NONE, 0 },
  { "BATH", "Choose BATH to clean up", "your Pokemon and any mess.", ACT_NONE, 0 },
  { "BATH", "A clean Pokemon is", "a happy Pokemon!", ACT_NONE, 0 },

  { "REST", nullptr, nullptr, ACT_HEADER, 0 },
  { "REST", "Even Pokemon need", "to rest.", ACT_NONE, 0 },
  { "REST", "Use the LIGHT button to", "put your Pokemon to sleep.", ACT_NONE, 0 },
  { "REST", "Sleep restores ENERGY.", "Needs fall more slowly.", ACT_NONE, 0 },
  { "REST", "A good Trainer knows", "when to let it rest.", ACT_NONE, 0 },

  { "GROWING", nullptr, nullptr, ACT_HEADER, 0 },
  { "GROWING", "Your Pokemon grows", "stronger as time passes.", ACT_NONE, 0 },
  { "GROWING", "Its level rises", "as it grows.", ACT_NONE, 0 },
  { "GROWING", "Take good care of it,", "and you'll be ready!", ACT_NONE, 0 },

  { "EVOLUTION", nullptr, nullptr, ACT_HEADER, 0 },
  { "EVOLUTION", "Sometimes its growth", "brings an amazing change.", ACT_NONE, 0 },
  { "EVOLUTION", "That's right!", nullptr, ACT_NONE, 0 },
  { "EVOLUTION", "Evolution!", nullptr, ACT_NONE, 0 },
  { "EVOLUTION", "When it is ready,", "you will get the choice.", ACT_NONE, 0 },
  { "EVOLUTION", "You can let it evolve...", nullptr, ACT_NONE, 0 },
  { "EVOLUTION", "...or keep it as it is", "for a little longer.", ACT_NONE, 0 },

  { "TRAINING", nullptr, nullptr, ACT_HEADER, 0 },
  { "TRAINING", "Of course, a Trainer", "must also train!", ACT_NONE, 0 },
  { "TRAINING", "Open your Pokemon's", "Battle information.", ACT_NONE, 0 },
  { "TRAINING", "There you'll find its", "battle stats and IVs.", ACT_NONE, 0 },
  { "TRAINING", "IVs are traits your", "Pokemon is born with.", ACT_NONE, 0 },
  { "TRAINING", "No two Pokemon are", "exactly alike!", ACT_NONE, 0 },
  { "TRAINING", "Training can make your", "Pokemon even stronger.", ACT_NONE, 0 },
  { "TRAINING", "The training bag", "builds STRENGTH.", ACT_NONE, 0 },
  { "TRAINING", "The reaction test", "builds SPEED.", ACT_NONE, 0 },
  { "TRAINING", "And the ball game", "trains DEFENSE.", ACT_NONE, 0 },

  { "MOVES", nullptr, nullptr, ACT_HEADER, 0 },
  { "MOVES", "A strong Pokemon needs", "strong moves!", ACT_NONE, 0 },
  { "MOVES", "Your Pokemon learns", "moves as it grows.", ACT_NONE, 0 },
  { "MOVES", "Later on, TMs will give", "you even more options.", ACT_NONE, 0 },
  { "MOVES", "Choose your moves", "carefully.", ACT_NONE, 0 },
  { "MOVES", "A clever Trainer knows", "their Pokemon's strengths!", ACT_NONE, 0 },

  { "PARTY", nullptr, nullptr, ACT_HEADER, 0 },
  { "PARTY", "One Pokemon is good.", nullptr, ACT_NONE, 0 },
  { "PARTY", "A team is even better!", nullptr, ACT_NONE, 0 },
  { "PARTY", "Build a party of up to", "six Pokemon.", ACT_NONE, 0 },
  { "PARTY", "Choose your team", "carefully.", ACT_NONE, 0 },
  { "PARTY", "Different Pokemon have", "different strengths.", ACT_NONE, 0 },
  { "PARTY", "A balanced team can", "overcome many challenges!", ACT_NONE, 0 },

  { "BATTLES", nullptr, nullptr, ACT_HEADER, 0 },
  { "BATTLES", "Now we're getting", "serious!", ACT_NONE, 0 },
  { "BATTLES", "Battles are fought", "one move at a time.", ACT_NONE, 0 },
  { "BATTLES", "Choose a Pokemon.", nullptr, ACT_NONE, 0 },
  { "BATTLES", "Then choose its move.", nullptr, ACT_NONE, 0 },
  { "BATTLES", "Watch the type matchups!", nullptr, ACT_NONE, 0 },
  { "BATTLES", "Some types are strong", "against others...", ACT_NONE, 0 },
  { "BATTLES", "...while some are weak.", nullptr, ACT_NONE, 0 },
  { "BATTLES", "And some moves can cause", "status conditions!", ACT_NONE, 0 },
  { "BATTLES", "Learn your matchups,", "and battle like a Trainer.", ACT_NONE, 0 },

  { "GYMS", nullptr, nullptr, ACT_HEADER, 0 },
  { "GYMS", "Think you're ready for", "the big leagues?", ACT_NONE, 0 },
  { "GYMS", "Then it's time to", "challenge a Gym Leader!", ACT_NONE, 0 },
  { "GYMS", "Each region has", "eight Gyms.", ACT_NONE, 0 },
  { "GYMS", "Defeat their teams", "and earn their Badges.", ACT_NONE, 0 },
  { "GYMS", "Badges prove your strength", "as a Pokemon Trainer.", ACT_NONE, 0 },
  { "GYMS", "But don't rush in", "unprepared!", ACT_NONE, 0 },
  { "GYMS", "Train your team.", "Learn your moves.", ACT_NONE, 0 },
  { "GYMS", "Know your type matchups.", nullptr, ACT_NONE, 0 },

  { "LEAGUE", nullptr, nullptr, ACT_HEADER, 0 },
  { "LEAGUE", "Eight Badges won't be", "the end of your journey.", ACT_NONE, 0 },
  { "LEAGUE", "Beyond the Gyms awaits", "the Elite Four.", ACT_NONE, 0 },
  { "LEAGUE", "Defeat them all...", nullptr, ACT_NONE, 0 },
  { "LEAGUE", "...and you'll face", "the Champion!", ACT_NONE, 0 },
  { "LEAGUE", "Only the strongest", "Trainers reach the top.", ACT_NONE, 0 },

  { "POKEDEX", nullptr, nullptr, ACT_HEADER, 0 },
  { "POKEDEX", "And don't forget", "your Pokedex!", ACT_NONE, 0 },
  { "POKEDEX", "Every Pokemon you raise", "can be registered.", ACT_NONE, 0 },
  { "POKEDEX", "There are hundreds of", "Pokemon to discover.", ACT_NONE, 0 },
  { "POKEDEX", "And some may even appear", "in their Shiny forms!", ACT_NONE, 0 },
  { "POKEDEX", "Complete the Pokedex,", "one Pokemon at a time.", ACT_NONE, 0 },

  { "EGGS", nullptr, nullptr, ACT_HEADER, 0 },
  { "EGGS", "Your journey doesn't end", "with one Pokemon.", ACT_NONE, 0 },
  { "EGGS", "When a new egg appears,", "you raise another Pokemon.", ACT_NONE, 0 },
  { "EGGS", "Your region helps decide", "which Pokemon can hatch.", ACT_NONE, 0 },
  { "EGGS", "Take good care of them", "and later partners benefit.", ACT_NONE, 0 },

  { "REGION", nullptr, nullptr, ACT_HEADER, 0 },
  { "REGION", "Your chosen region is", "only the beginning.", ACT_NONE, 0 },
  { "REGION", "Pokemon from many regions", "can join your collection.", ACT_NONE, 0 },
  { "REGION", "Explore them all and", "complete your Pokedex!", ACT_NONE, 0 },

  { "FINAL", nullptr, nullptr, ACT_HEADER, 0 },
  { "FINAL", "Well, %s...", nullptr, ACT_NONE, FL_FMT },
  { "FINAL", "I believe you're ready.", nullptr, ACT_NONE, 0 },
  { "FINAL", "Raise your Pokemon.", nullptr, ACT_NONE, 0 },
  { "FINAL", "Train your team.", nullptr, ACT_NONE, 0 },
  { "FINAL", "Challenge the Gyms.", nullptr, ACT_NONE, 0 },
  { "FINAL", "Earn your Badges.", nullptr, ACT_NONE, 0 },
  { "FINAL", "And discover every", "Pokemon you can!", ACT_NONE, 0 },
  { "FINAL", "Most importantly...", nullptr, ACT_NONE, 0 },
  { "FINAL", "Take good care of", "your partners.", ACT_NONE, 0 },
  { "FINAL", "Your Pokemon journey", "starts now!", ACT_BEGIN, 0 },
};

#define TUT_COUNT ((uint8_t)(sizeof(SCRIPT) / sizeof(SCRIPT[0])))

enum : uint8_t { HOLD_TALK = 0, HOLD_NAME, HOLD_REGION, HOLD_STARTER };

static uint8_t hold = HOLD_TALK;
static uint16_t *art = nullptr;
static bool artTried = false;

static const TutStep &step() {
  uint8_t i = pet.tutStep < TUT_COUNT ? pet.tutStep : (uint8_t)(TUT_COUNT - 1);
  return SCRIPT[i];
}

static void center(const char *s, int y, uint8_t size, uint16_t col) {
  if (!s) return;
  gfx->setTextColor(col);
  gfx->setTextSize(size);
  gfx->setCursor(CX - (int)strlen(s) * (6 * size) / 2, y);
  gfx->print(s);
}

static const char *trainerLabel() {
  return pet.trainerName[0] ? pet.trainerName : "TRAINER";
}

static void fmtLine(char *buf, size_t n, const char *fmt) {
  snprintf(buf, n, fmt, trainerLabel());
}

static void persist() { pet.saveNow(); }

static uint8_t findAct(uint8_t act) {
  for (uint8_t i = 0; i < TUT_COUNT; i++)
    if (SCRIPT[i].act == act) return i;
  return (uint8_t)(TUT_COUNT - 1);
}

static void go(uint8_t i) {
  if (i >= TUT_COUNT) i = (uint8_t)(TUT_COUNT - 1);
  pet.tutStep = i;
  hold = HOLD_TALK;
  persist();
}

static void finish() {
  pet.tutDone = true;
  persist();
  hold = HOLD_TALK;
  sfxPlay(SFX_MEDAL);
}

static void advance() {
  if (pet.tutStep + 1 >= TUT_COUNT) { finish(); return; }
  pet.tutStep++;
  hold = HOLD_TALK;
  persist();
  sfxPlay(SFX_TAP);
}

static void skipTalk() {
  sfxPlay(SFX_TAP);
  if (!pet.trainerName[0]) {
    go(findAct(ACT_NAME));
    hold = HOLD_NAME;
    openKeyboardFor(1);   // KB_TRAINER
    return;
  }
  if (!starterRegionDone) {
    go(findAct(ACT_REGION));
    hold = HOLD_REGION;
    return;
  }
  if (pet.awaitingStarter()) {
    go(findAct(ACT_STARTER));
    hold = HOLD_STARTER;
    return;
  }
  go(findAct(ACT_BEGIN));
}

static void loadArt() {
  if (artTried) return;
  artTried = true;
  if (!sdReady) return;
  File f = SD_MMC.open("/tangrowth.rgb", FILE_READ);
  if (!f) f = SD_MMC.open("/mons/tangrowth.rgb", FILE_READ);
  const uint32_t need = (uint32_t)TUT_ART_W * TUT_ART_H * 2;
  if (!f || f.size() != need) { if (f) f.close(); return; }
  art = (uint16_t *)ps_malloc(need);
  if (!art) { f.close(); return; }
  if (f.read((uint8_t *)art, need) != need) {
    free(art);
    art = nullptr;
  }
  f.close();
}

static void drawArt() {
  loadArt();
  if (art) {
    gfx->draw16bitRGBBitmap(TUT_ART_X, TUT_ART_Y, art, TUT_ART_W, TUT_ART_H);
    return;
  }
  gfx->fillRoundRect(TUT_ART_X + 20, TUT_ART_Y + 24, TUT_ART_W - 40, TUT_ART_H - 48, 20, UI_BAR_OK);
  center("PROF.", TUT_ART_Y + 88, 3, UI_WHITE);
  center("TANGROWTH", TUT_ART_Y + 128, 2, UI_WHITE);
}

static bool hitSkip(int16_t x, int16_t y) {
  return x >= TUT_SKIP_X && x <= TUT_SKIP_X + TUT_SKIP_W &&
         y >= TUT_SKIP_Y && y <= TUT_SKIP_Y + TUT_SKIP_H;
}

static void drawSkip() {
  gfx->fillRoundRect(TUT_SKIP_X, TUT_SKIP_Y, TUT_SKIP_W, TUT_SKIP_H, 8, UI_WHITE);
  gfx->drawRoundRect(TUT_SKIP_X, TUT_SKIP_Y, TUT_SKIP_W, TUT_SKIP_H, 8, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(TUT_SKIP_X + 12, TUT_SKIP_Y + 8);
  gfx->print("SKIP");
}

static void drawTalkBox(const TutStep &s) {
  char a[40], b[40];
  a[0] = b[0] = 0;
  if (s.l1) {
    if (s.flags & FL_FMT) fmtLine(a, sizeof(a), s.l1);
    else snprintf(a, sizeof(a), "%s", s.l1);
  }
  if (s.l2) snprintf(b, sizeof(b), "%s", s.l2);

  const int boxH = (s.act == ACT_BEGIN) ? 78 : TUT_BOX_H;
  gfx->fillRoundRect(TUT_BOX_X, TUT_BOX_Y, TUT_BOX_W, boxH, 16, UI_WHITE);
  gfx->drawRoundRect(TUT_BOX_X, TUT_BOX_Y, TUT_BOX_W, boxH, 16, UI_INK);
  gfx->setTextColor(UI_BAR_OK);
  gfx->setTextSize(1);
  gfx->setCursor(TUT_BOX_X + 16, TUT_BOX_Y + 12);
  gfx->print("PROF. TANGROWTH");

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  int y = TUT_BOX_Y + (b[0] ? 36 : 52);
  if (a[0]) {
    gfx->setCursor(CX - (int)strlen(a) * 6, y);
    gfx->print(a);
  }
  if (b[0]) {
    gfx->setCursor(CX - (int)strlen(b) * 6, y + 26);
    gfx->print(b);
  }

  if (s.act == ACT_BEGIN) {
    gfx->fillRoundRect(TUT_BEGIN_X, TUT_BEGIN_Y, TUT_BEGIN_W, TUT_BEGIN_H, 14, UI_BAR_OK);
    gfx->drawRoundRect(TUT_BEGIN_X, TUT_BEGIN_Y, TUT_BEGIN_W, TUT_BEGIN_H, 14, UI_INK);
    center("BEGIN ADVENTURE", TUT_BEGIN_Y + 18, 2, UI_WHITE);
  } else {
    gfx->setTextColor(UI_TRACK);
    gfx->setTextSize(1);
    gfx->setCursor(CX - 21 * 3, TUT_BOX_Y + TUT_BOX_H - 18);
    gfx->print("tap to continue");
  }
}

bool tutorialActive() {
  if (pet.tutDone) return false;
  return pet.awaitingStarter() || pet.tutStep > 0;
}

TutView tutorialView() {
  if (!tutorialActive()) return TUTVIEW_OFF;
  if (hold == HOLD_REGION) return TUTVIEW_REGION;
  if (hold == HOLD_STARTER) return TUTVIEW_STARTER;
  return TUTVIEW_TALK;
}

void tutorialRender() {
  const TutStep &s = step();
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CX, 231, UI_BG_DAY);

  if (s.act == ACT_HEADER) {
    gfx->fillRoundRect(70, 150, 326, 140, 18, UI_WHITE);
    gfx->drawRoundRect(70, 150, 326, 140, 18, UI_INK);
    gfx->fillRect(90, 168, 286, 6, UI_BAR_WARN);
    gfx->fillRect(90, 266, 286, 6, UI_BAR_WARN);
    center(s.header ? s.header : "", 208, 3, UI_INK);
    gfx->setTextColor(UI_TRACK);
    gfx->setTextSize(1);
    gfx->setCursor(CX - 21 * 3, 320);
    gfx->print("tap to continue");
    drawSkip();
    gfx->flush();
    return;
  }

  drawArt();
  if (s.header) {
    gfx->fillRoundRect(90, 268, 286, 26, 8, UI_BAR_WARN);
    center(s.header, 274, 2, UI_INK);
  }
  drawTalkBox(s);
  drawSkip();
  gfx->flush();
}

bool tutorialTap(int16_t x, int16_t y) {
  if (!tutorialActive() || hold != HOLD_TALK) return false;
  if (hitSkip(x, y)) { skipTalk(); return true; }

  const TutStep &s = step();
  if (s.act == ACT_BEGIN) {
    bool onBtn = x >= TUT_BEGIN_X && x <= TUT_BEGIN_X + TUT_BEGIN_W &&
                 y >= TUT_BEGIN_Y && y <= TUT_BEGIN_Y + TUT_BEGIN_H;
    if (!onBtn) return true;   // wait for the button
    finish();
    return true;
  }
  if (s.act == ACT_NAME) {
    hold = HOLD_NAME;
    openKeyboardFor(1);
    sfxPlay(SFX_TAP);
    return true;
  }
  if (s.act == ACT_REGION) {
    hold = HOLD_REGION;
    sfxPlay(SFX_TAP);
    return true;
  }
  if (s.act == ACT_STARTER) {
    hold = HOLD_STARTER;
    sfxPlay(SFX_TAP);
    return true;
  }
  advance();
  return true;
}

void tutorialOnNameDone() {
  if (!tutorialActive() || hold != HOLD_NAME) return;
  if (!pet.trainerName[0]) pet.renameTrainer("TRAINER");
  advance();
}

void tutorialOnRegion(uint8_t region) {
  if (!tutorialActive() || hold != HOLD_REGION) return;
  pet.setRegion(region);
  starterRegionDone = true;
  advance();
}

void tutorialOnStarter(int16_t dex) {
  if (!tutorialActive() || hold != HOLD_STARTER) return;
  pet.chooseStarter(dex);
  advance();
}

void tutorialReplay() {
  pet.tutDone = false;
  pet.tutStep = 0;
  hold = HOLD_TALK;
  persist();
}
