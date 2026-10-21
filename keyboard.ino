#include <USB.h>
#include <USBMIDI.h>
#include <FastLED.h>

USBMIDI MIDIusb;

#define TOUCH_T1        1
#define TOUCH_T2        2
#define TOUCH_T3        3
#define TOUCH_T4        4
#define TOUCH_T5        5
#define TOUCH_T6        6
#define BTN_T7          44
#define BTN_SW2         9
#define BTN_SW3         10
#define BTN_SW4         11
#define LED_PIN         8
#define NUM_LEDS        8
#define TOUCH_THRESHOLD 25000
#define LONG_PRESS_MS   800
#define DEBOUNCE_MS     20
#define MIDI_CHANNEL    1
#define BASE_VELOCITY   100

enum Mode { NORMAL, NOTE_LENGTH, CHORD, ARP, SCALE_SELECT };
enum ScaleType { CHROMATIC, MAJOR, MINOR, PENTATONIC };

const int SCALES[][8] = {
  {0, 1,  2,  3,  4, 5,  6,  7},
  {0, 2,  4,  5,  7, 9,  11, 12},
  {0, 2,  3,  5,  7, 8,  10, 12},
  {0, 2,  4,  7,  9, 0,  0,  0},
};

CRGB leds[NUM_LEDS];

int           baseNote          = 60;
int           semitone          = 0;
int           noteLengthVal     = 64;
int           currentScale      = MAJOR;
bool          sustainOn         = false;
bool          arpOn             = false;
int           arpNotes[7]       = {0};
int           arpCount          = 0;
int           arpStep           = 0;
unsigned long arpLastStep       = 0;
int           arpBPM            = 120;
int           chordNotes[7]     = {0};
int           chordCount        = 0;
Mode          currentMode       = NORMAL;

int           touchPins[6]         = {TOUCH_T1, TOUCH_T2, TOUCH_T3, TOUCH_T4, TOUCH_T5, TOUCH_T6};
bool          touchCurr[6]         = {false};
bool          touchPrev[6]         = {false};
unsigned long touchPressTime[6]    = {0};
bool          longPressHandled[6]  = {false};

int           btnPins[4]           = {BTN_T7, BTN_SW2, BTN_SW3, BTN_SW4};
bool          btnCurr[4]           = {false};
bool          btnPrev[4]           = {false};
unsigned long btnDebounce[4]       = {0};
unsigned long btnPressTime[4]      = {0};
bool          btnLongHandled[4]    = {false};

CRGB COLOR_IDLE     = CRGB(10,   0,  20);
CRGB COLOR_NORMAL   = CRGB(0,   50, 200);
CRGB COLOR_LENGTH   = CRGB(220, 80,   0);
CRGB COLOR_CHORD    = CRGB(180,  0, 180);
CRGB COLOR_ARP      = CRGB(0,  200, 150);
CRGB COLOR_SCALE    = CRGB(200, 200,  0);
CRGB COLOR_PRESS    = CRGB(255, 255, 255);
CRGB COLOR_OCT_UP   = CRGB(0,  255,  80);
CRGB COLOR_OCT_DOWN = CRGB(255,   0,  60);
CRGB COLOR_SUSTAIN  = CRGB(50,  150, 255);
CRGB COLOR_PANIC    = CRGB(255,   0,   0);

int getScaleNote(int degree) {
  int octaveOffset = (degree / 8) * 12;
  int scaleDeg     = degree % 8;
  return baseNote + SCALES[currentScale][scaleDeg] + octaveOffset;
}

int getMidiNote() {
  return baseNote + SCALES[currentScale][0] + semitone;
}

int getVelocityFromTouch(uint32_t touchVal) {
  uint32_t clamped = constrain(touchVal, 0, TOUCH_THRESHOLD);
  return constrain((int)map(clamped, TOUCH_THRESHOLD, 0, 40, 127), 40, 127);
}

void allNotesOff() {
  for (int ch = 1; ch <= 16; ch++) {
    MIDIusb.controlChange(123, 0, ch);
  }
}

void setAll(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = color;
  FastLED.show();
}

void flashSingle(int index, CRGB color) {
  if (index < 0 || index >= NUM_LEDS) return;
  CRGB prev    = leds[index];
  leds[index]  = color;
  FastLED.show();
  delay(40);
  leds[index]  = prev;
  FastLED.show();
}

void flashAll(CRGB color) {
  CRGB prev[NUM_LEDS];
  for (int i = 0; i < NUM_LEDS; i++) prev[i] = leds[i];
  setAll(color);
  delay(80);
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = prev[i];
  FastLED.show();
}

void updateModeLEDs() {
  if (currentMode == NORMAL) {
    int level = map(baseNote, 24, 108, 0, NUM_LEDS - 1);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i <= level) ? COLOR_NORMAL : COLOR_IDLE;
    }
    if (sustainOn) leds[NUM_LEDS - 1] = COLOR_SUSTAIN;

  } else if (currentMode == NOTE_LENGTH) {
    int level = map(noteLengthVal, 0, 127, 0, NUM_LEDS - 1);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i <= level) ? COLOR_LENGTH : COLOR_IDLE;
    }

  } else if (currentMode == CHORD) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i < chordCount) ? COLOR_CHORD : COLOR_IDLE;
    }

  } else if (currentMode == ARP) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i == arpStep % NUM_LEDS && arpOn) ? COLOR_ARP : COLOR_IDLE;
    }

  } else if (currentMode == SCALE_SELECT) {
    int count = currentScale + 1;
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i < count) ? COLOR_SCALE : COLOR_IDLE;
    }
  }
  FastLED.show();
}

void panic() {
  allNotesOff();
  arpCount   = 0;
  chordCount = 0;
  sustainOn  = false;
  arpOn      = false;
  currentMode = NORMAL;
  for (int i = 0; i < 3; i++) {
    setAll(COLOR_PANIC);
    delay(80);
    setAll(COLOR_IDLE);
    delay(80);
  }
  updateModeLEDs();
}

void startupAnimation() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = COLOR_NORMAL;
    FastLED.show();
    delay(50);
  }
  delay(200);
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    leds[i] = COLOR_IDLE;
    FastLED.show();
    delay(50);
  }
  delay(100);
  updateModeLEDs();
}

void runArpeggiator() {
  if (!arpOn || arpCount == 0) return;
  unsigned long interval = 60000UL / arpBPM;
  if (millis() - arpLastStep >= interval) {
    int prevNote = arpNotes[(arpStep - 1 + arpCount) % arpCount];
    int nextNote = arpNotes[arpStep % arpCount];
    MIDIusb.noteOff(prevNote, 0, MIDI_CHANNEL);
    MIDIusb.noteOn(nextNote,  BASE_VELOCITY, MIDI_CHANNEL);
    arpStep++;
    arpLastStep = millis();
    updateModeLEDs();
  }
}

void onTouchPress(int i) {
  touchPressTime[i]   = millis();
  longPressHandled[i] = false;

  if (currentMode == NORMAL) {
    switch (i) {
      case 0: {
        int      note = getMidiNote();
        uint32_t tval = touchRead(TOUCH_T1);
        int      vel  = getVelocityFromTouch(tval);
        MIDIusb.noteOn(note, vel, MIDI_CHANNEL);
        flashSingle(0, COLOR_PRESS);
        break;
      }
      case 1:
        if (baseNote + 12 <= 108) {
          baseNote += 12;
          flashAll(COLOR_OCT_UP);
          updateModeLEDs();
        }
        break;
      case 2:
        if (baseNote - 12 >= 24) {
          baseNote -= 12;
          flashAll(COLOR_OCT_DOWN);
          updateModeLEDs();
        }
        break;
      case 3:
        semitone = constrain(semitone + 1, -6, 6);
        flashSingle(3, COLOR_PRESS);
        break;
      case 4:
        semitone = constrain(semitone - 1, -6, 6);
        flashSingle(4, COLOR_PRESS);
        break;
      case 5:
        currentMode = NOTE_LENGTH;
        updateModeLEDs();
        break;
    }

  } else if (currentMode == NOTE_LENGTH) {
    switch (i) {
      case 1:
      case 3:
        noteLengthVal = constrain(noteLengthVal + 8, 0, 127);
        MIDIusb.controlChange(73, noteLengthVal, MIDI_CHANNEL);
        flashSingle(i, COLOR_PRESS);
        updateModeLEDs();
        break;
      case 2:
      case 4:
        noteLengthVal = constrain(noteLengthVal - 8, 0, 127);
        MIDIusb.controlChange(73, noteLengthVal, MIDI_CHANNEL);
        flashSingle(i, COLOR_PRESS);
        updateModeLEDs();
        break;
      case 5:
        currentMode = NORMAL;
        updateModeLEDs();
        break;
    }

  } else if (currentMode == CHORD) {
    if (i == 5) {
      if (chordCount > 0) {
        for (int n = 0; n < chordCount; n++) {
          MIDIusb.noteOn(chordNotes[n], BASE_VELOCITY, MIDI_CHANNEL);
        }
        flashAll(COLOR_CHORD);
      }
    } else if (i < 5) {
      int  note  = getScaleNote(i);
      bool found = false;
      for (int n = 0; n < chordCount; n++) {
        if (chordNotes[n] == note) { found = true; break; }
      }
      if (!found && chordCount < 7) {
        chordNotes[chordCount++] = note;
        updateModeLEDs();
      }
    }

  } else if (currentMode == ARP) {
    if (i == 5) {
      arpOn = !arpOn;
      if (!arpOn) allNotesOff();
      updateModeLEDs();
    } else if (i < 5) {
      int  note  = getScaleNote(i);
      bool found = false;
      for (int n = 0; n < arpCount; n++) {
        if (arpNotes[n] == note) { found = true; break; }
      }
      if (!found && arpCount < 7) {
        arpNotes[arpCount++] = note;
        flashSingle(i, COLOR_PRESS);
      }
    }

  } else if (currentMode == SCALE_SELECT) {
    if (i < 4) {
      currentScale = i;
      flashAll(COLOR_SCALE);
      updateModeLEDs();
    }
    if (i == 5) {
      currentMode = NORMAL;
      updateModeLEDs();
    }
  }
}

void onTouchRelease(int i) {
  if (i == 0 && currentMode == NORMAL) {
    if (!sustainOn) MIDIusb.noteOff(getMidiNote(), 0, MIDI_CHANNEL);
  }
  if (currentMode == CHORD && i == 5) {
    if (!sustainOn) {
      for (int n = 0; n < chordCount; n++) {
        MIDIusb.noteOff(chordNotes[n], 0, MIDI_CHANNEL);
      }
    }
  }
}

void onTouchLongPress(int i) {
  if (i == 0) {
    panic();
    return;
  }
  if (currentMode == NORMAL) {
    if (i == 1) {
      currentMode = CHORD;
      chordCount  = 0;
      updateModeLEDs();
    } else if (i == 2) {
      currentMode = ARP;
      arpCount    = 0;
      arpStep     = 0;
      arpOn       = false;
      updateModeLEDs();
    } else if (i == 3) {
      currentMode = SCALE_SELECT;
      updateModeLEDs();
    }
  } else if (currentMode == CHORD && i == 1) {
    currentMode = NORMAL;
    chordCount  = 0;
    allNotesOff();
    updateModeLEDs();
  } else if (currentMode == ARP && i == 2) {
    currentMode = NORMAL;
    arpCount    = 0;
    arpOn       = false;
    allNotesOff();
    updateModeLEDs();
  }
}

void onButtonPress(int i) {
  btnPressTime[i]   = millis();
  btnLongHandled[i] = false;
  switch (i) {
    case 0:
      MIDIusb.controlChange(74, 127, MIDI_CHANNEL);
      flashSingle(7, COLOR_PRESS);
      break;
    case 1:
      MIDIusb.controlChange(20, 127, MIDI_CHANNEL);
      flashSingle(5, COLOR_PRESS);
      break;
    case 2:
      MIDIusb.controlChange(21, 127, MIDI_CHANNEL);
      flashSingle(6, COLOR_PRESS);
      break;
    case 3:
      MIDIusb.controlChange(22, 127, MIDI_CHANNEL);
      flashSingle(7, COLOR_PRESS);
      break;
  }
}

void onButtonLongPress(int i) {
  if (i == 1) {
    arpBPM = constrain(arpBPM + 10, 60, 240);
    flashAll(COLOR_ARP);
  } else if (i == 2) {
    sustainOn = !sustainOn;
    if (!sustainOn) allNotesOff();
    updateModeLEDs();
  } else if (i == 3) {
    arpBPM = constrain(arpBPM - 10, 60, 240);
    flashAll(COLOR_ARP);
  }
}

void readTouchPads() {
  for (int i = 0; i < 6; i++) {
    uint32_t val  = touchRead(touchPins[i]);
    touchCurr[i]  = (val < TOUCH_THRESHOLD);

    if (touchCurr[i] && !touchPrev[i]) onTouchPress(i);
    if (!touchCurr[i] && touchPrev[i]) onTouchRelease(i);

    if (touchCurr[i] && !longPressHandled[i]) {
      if (millis() - touchPressTime[i] >= LONG_PRESS_MS) {
        onTouchLongPress(i);
        longPressHandled[i] = true;
      }
    }
    touchPrev[i] = touchCurr[i];
  }
}

void readButtons() {
  for (int i = 0; i < 4; i++) {
    bool reading = (digitalRead(btnPins[i]) == LOW);
    if (reading != btnPrev[i]) btnDebounce[i] = millis();

    if ((millis() - btnDebounce[i]) > DEBOUNCE_MS) {
      if (reading && !btnCurr[i])        onButtonPress(i);
      if (reading && !btnLongHandled[i]) {
        if (millis() - btnPressTime[i] >= LONG_PRESS_MS) {
          onButtonLongPress(i);
          btnLongHandled[i] = true;
        }
      }
      btnCurr[i] = reading;
    }
    btnPrev[i] = reading;
  }
}

void setup() {
  Serial.begin(115200);
  MIDIusb.begin();
  USB.begin();
  pinMode(BTN_T7,  INPUT_PULLUP);
  pinMode(BTN_SW2, INPUT_PULLUP);
  pinMode(BTN_SW3, INPUT_PULLUP);
  pinMode(BTN_SW4, INPUT_PULLUP);
  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(70);
  startupAnimation();
}

void loop() {
  readTouchPads();
  readButtons();
  runArpeggiator();
  MIDIusb.read();
}
