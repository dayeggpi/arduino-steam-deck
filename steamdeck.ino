/* DIY Stream Deck Keyboard Code with key bindings settings in settings.ini locally
 * by: dayeggpi
 * date: May 2026
 * license: Creative Commons - Attribution - Non-Commercial.
 * MODIFIED to support Keyboard Modifier Combinations + EEPROM config via serial
 */

#include "Keyboard.h"
#include <Grove_LED_Bar.h>
#include <EEPROM.h>

Grove_LED_Bar bar(15, 14, 1);  // Clock pin, Data pin, Orientation

#define EEPROM_MAGIC 0x42
#define EEPROM_BASE  1
#define PAGES        10 // ADJUST PAGES
#define BUTTONS      4

struct KeyCommand {
  char modifier;
  char modifier2;
  char key;
};

KeyCommand bindings[PAGES][BUTTONS];

int pageFlag  = 1;
bool configMode = false;

const int buttonPin2 = 2;
const int buttonPin3 = 3;
const int buttonPin4 = 4;
const int buttonPin5 = 5;
const int buttonPin6 = 6;

int pressedPin2 = HIGH, pressedPin3 = HIGH, pressedPin4 = HIGH, pressedPin5 = HIGH, pressedPin6 = HIGH;
int lastState2  = HIGH, lastState3  = HIGH, lastState4  = HIGH, lastState5  = HIGH, lastState6  = HIGH;

// ── EEPROM ────────────────────────────────────────────────────────────────────

void saveToEEPROM() {
  EEPROM.update(0, EEPROM_MAGIC);
  for (int p = 0; p < PAGES; p++) {
    for (int b = 0; b < BUTTONS; b++) {
      int addr = EEPROM_BASE + (p * BUTTONS + b) * 3;
      EEPROM.update(addr,     (byte)bindings[p][b].modifier);
      EEPROM.update(addr + 1, (byte)bindings[p][b].modifier2);
      EEPROM.update(addr + 2, (byte)bindings[p][b].key);
    }
  }
}

void loadFromEEPROM() {
  if (EEPROM.read(0) != EEPROM_MAGIC) {
    memset(bindings, 0, sizeof(bindings));
    saveToEEPROM();
    return;
  }
  for (int p = 0; p < PAGES; p++) {
    for (int b = 0; b < BUTTONS; b++) {
      int addr = EEPROM_BASE + (p * BUTTONS + b) * 3;
      bindings[p][b].modifier  = (char)EEPROM.read(addr);
      bindings[p][b].modifier2 = (char)EEPROM.read(addr + 1);
      bindings[p][b].key       = (char)EEPROM.read(addr + 2);
    }
  }
}

// ── Serial config ─────────────────────────────────────────────────────────────

void dumpConfig() {
  for (int p = 0; p < PAGES; p++) {
    for (int b = 0; b < BUTTONS; b++) {
      Serial.print("P"); Serial.print(p + 1);
      Serial.print("B"); Serial.print(b + 1);
      Serial.print(" ");
      Serial.print((byte)bindings[p][b].modifier);  Serial.print(" ");
      Serial.print((byte)bindings[p][b].modifier2); Serial.print(" ");
      Serial.println((byte)bindings[p][b].key);
    }
  }
  Serial.println("DUMP_END");
}

void handleSerial() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line == "CFG_START") {
    configMode = true;
    Keyboard.releaseAll();
    Serial.println("READY");

  } else if (line == "CFG_END") {
    if (configMode) {
      saveToEEPROM();
      configMode = false;
      Serial.println("SAVED");
    }

  } else if (line == "CFG_RESET") {
    memset(bindings, 0, sizeof(bindings));
    saveToEEPROM();
    configMode = false;
    Serial.println("RESET");

  } else if (line == "CFG_DUMP") {
    dumpConfig();

  } else if (configMode && line[0] == 'P') {
    // Format: "P<page>B<btn> <mod1> <mod2> <key>"  (1-indexed, values 0-255)
    int bPos = line.indexOf('B');
    int sp1  = line.indexOf(' ');
    if (bPos < 0 || sp1 < 0 || bPos >= sp1) { Serial.println("ERR_FMT"); return; }
    int page = line.substring(1, bPos).toInt() - 1;
    int btn  = line.substring(bPos + 1, sp1).toInt() - 1;

    if (page < 0 || page >= PAGES || btn < 0 || btn >= BUTTONS) {
      Serial.println("ERR_RANGE"); return;
    }
    if (sp1 < 0) { Serial.println("ERR_FMT"); return; }
    String vals = line.substring(sp1 + 1);

    int sp2 = vals.indexOf(' ');
    int sp3 = vals.indexOf(' ', sp2 + 1);
    if (sp2 < 0 || sp3 < 0) { Serial.println("ERR_FMT"); return; }

    bindings[page][btn].modifier  = (char)vals.substring(0, sp2).toInt();
    bindings[page][btn].modifier2 = (char)vals.substring(sp2 + 1, sp3).toInt();
    bindings[page][btn].key       = (char)vals.substring(sp3 + 1).toInt();
    Serial.println("OK");
  }
}

// ── Key execution ─────────────────────────────────────────────────────────────

void executeCommand(KeyCommand cmd) {
  if (cmd.modifier  != 0) Keyboard.press(cmd.modifier);
  if (cmd.modifier2 != 0) Keyboard.press(cmd.modifier2);
  Keyboard.press(cmd.key);
  delay(30);
}

// ── Setup / Loop ──────────────────────────────────────────────────────────────

void setup() {
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(buttonPin4, INPUT_PULLUP);
  pinMode(buttonPin5, INPUT_PULLUP);
  pinMode(buttonPin6, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.setTimeout(500);

  Keyboard.begin();
  delay(50);
  bar.begin();
  bar.setLevel(1);

  loadFromEEPROM();
}

void loop() {
  handleSerial();

  if (configMode) return;  // freeze button processing while configuring

  pressedPin2 = digitalRead(2);
  pressedPin3 = digitalRead(3);
  pressedPin4 = digitalRead(4);
  pressedPin5 = digitalRead(5);
  pressedPin6 = digitalRead(6);

  if (pressedPin2 != lastState2 && pressedPin2 == LOW)
    executeCommand(bindings[pageFlag - 1][0]);

  if (pressedPin3 != lastState3 && pressedPin3 == LOW)
    executeCommand(bindings[pageFlag - 1][1]);

  if (pressedPin4 != lastState4 && pressedPin4 == LOW)
    executeCommand(bindings[pageFlag - 1][2]);

  if (pressedPin5 != lastState5 && pressedPin5 == LOW)
    executeCommand(bindings[pageFlag - 1][3]);

  if (pressedPin6 != lastState6 && pressedPin6 == LOW) {
    pageFlag = (pageFlag % 10) + 1; // ADJUST PAGES
    bar.setLevel(pageFlag);
  }

  lastState2 = pressedPin2;
  lastState3 = pressedPin3;
  lastState4 = pressedPin4;
  lastState5 = pressedPin5;
  lastState6 = pressedPin6;

  Keyboard.releaseAll();
}
