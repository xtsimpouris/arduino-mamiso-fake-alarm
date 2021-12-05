#include "FastLED.h"

// ---------- Constants
#define DEBUG false

#define TIME_MILLISEC 1.0
#define TIME_SEC      1000.0 * TIME_MILLISEC
#define TIME_MINUTE   60.0 * TIME_SEC
#define TIME_HOUR     60.0 * TIME_MINUTE
#define TIME_DAY      24.0 * TIME_HOUR

#define ALARM_STATE_OFFLINE   0
#define ALARM_STATE_WAITING   1
#define ALARM_STATE_ONLINE    2

#define NUM_LEDS              4
#define BRIGHTNESS            100
#define DATA_PIN              3
#define PHOTO_PIN             A0
#define LED_TYPE              WS2811
#define NUMBER_OF_PATTERNS    7
#define NIGHT_THRESHOLD       600.0

// All patterns are based on this time delay
#define MAIN_DELAY            900.0 * TIME_MILLISEC

// Pattern stays the same for this amount of seconds
#define PATTERN_KEEP_FOR      2.0 * TIME_HOUR

// We check sunlight every after..
#define CHECK_SUNLIGHT_EVERY  1.0 * TIME_MINUTE

// After sunset, we wait the following amount of time before alarm starts
// As we do not know when everybody goes to sleep, we assume a lot of time when "alarm" is online
#define WAIT_AFTER_SUNSET     3.0 * TIME_HOUR

// Reset system after sunrise
// Always keeps system fresh
#define REBOOT_SYSTEM_AFTER_SUNRISE  3.0 * TIME_HOUR

// ---------- Variables

// Number of leds
CRGB leds[NUM_LEDS];

// Current pattern
int pattern = 0;
unsigned int cycles = 0;

// Time to keep track how much time has passed for current pattern
unsigned long pattern_start_millis = 0;

// Alarm state
int alarm_state;
unsigned long alarm_state_started_waiting;
unsigned long alarm_state_last_sunlight_check;
unsigned long alarm_state_sun_went_up;

// Other
double light_history[5];

void setup() {
  // To protect led strip
  delay(3.0 * TIME_SEC);

  // Setup LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Init time for the first pattern
  resetPatternSequence();
  alarm_state = ALARM_STATE_OFFLINE;
  alarm_state_last_sunlight_check = 0;

  light_history[0] = analogRead(A0);
  light_history[1] = light_history[0];
  light_history[2] = light_history[0];
  light_history[3] = light_history[0];
  light_history[4] = light_history[0];

  #if DEBUG
    // Some basic status printing
    Serial.begin(9600);
  #endif

}

/*
 * Reboot system
 */
void(* reboot) (void) = 0;

/*
 * Main loop where magic happens
 */
void loop() {
  if (millis() - alarm_state_last_sunlight_check > CHECK_SUNLIGHT_EVERY) {
    double lastLightValue = getLightValue();
    #if DEBUG
      Serial.print("We will check sunlight: ");
      Serial.println(lastLightValue);
    #endif
    alarm_state_last_sunlight_check = millis();
    // We have sun
    if (lastLightValue > NIGHT_THRESHOLD) {
      #if DEBUG
        Serial.println("..Still sun");
      #endif

      // This is the first time, let's show a signal
      if (alarm_state != ALARM_STATE_OFFLINE) {
        sendSignal(2);
        alarm_state_sun_went_up = millis();
      }

      // If sun is up after having at least a cycle, reset the system to keep fresh
      if (alarm_state == ALARM_STATE_OFFLINE && millis() - alarm_state_sun_went_up > REBOOT_SYSTEM_AFTER_SUNRISE && cycles > 0)
        return reboot();

      // Everything goes black
      // Sun is up, no need to wait less
      setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CHECK_SUNLIGHT_EVERY);
      alarm_state = ALARM_STATE_OFFLINE;
      return;
    }

    // It is night
    if (alarm_state == ALARM_STATE_OFFLINE) {
      #if DEBUG
        Serial.println("Sun went down, let's start waiting");
      #endif
      sendSignal(3);

      alarm_state = ALARM_STATE_WAITING;
      alarm_state_started_waiting = millis();
      delay(CHECK_SUNLIGHT_EVERY);
      return;
    }

    // It is night, are we still waiting?
    if (alarm_state == ALARM_STATE_WAITING) {
      // Delay after sunset?
      if (millis() - alarm_state_started_waiting < WAIT_AFTER_SUNSET) {
        #if DEBUG
          Serial.println("Sun still down, waiting..");
        #endif
        delay(CHECK_SUNLIGHT_EVERY);
        return;
      }

      #if DEBUG
        Serial.println("Start messing with the led");
      #endif
      // We do not wait any more
      alarm_state = ALARM_STATE_ONLINE;
      goToTheNextPattern();
      resetPatternTime();
    }
  }
  
  if (alarm_state == ALARM_STATE_ONLINE) {
    executeCurrentPattern();

    if (hasPatternExceededTime()) {
      goToTheNextPattern();
      resetPatternTime();
    }
  }
}

/*
 * Checks if pattern time has exceeded
 */
bool hasPatternExceededTime() {
  return millis() - pattern_start_millis > PATTERN_KEEP_FOR;
}

/*
 * Gets to the next pattern
 */
void goToTheNextPattern() {
  // Deterministic
  // pattern = (pattern + 1) % NUMBER_OF_PATTERNS;

  // "random"
  pattern = (millis() + (int)getLightValue()) % NUMBER_OF_PATTERNS;

  #if DEBUG
    Serial.print("Pattern ");
    Serial.print(pattern + 1);
    Serial.println(" starts.");
  #endif
}

/*
 * Signal
 */
void sendSignal(unsigned int times) {
  while(times > 0) {
    times -= 1;
    setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, TIME_SEC / 15);
    setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, TIME_SEC / 15);
  }
}

/*
 * Reset pattern sequence
 */
void resetPatternSequence() {
  // Visual indicator sequence has been reset
  sendSignal(1);
  resetPatternTime();
  pattern = 0;
}

/*
 * Resets pattern time
 */
void resetPatternTime() {
  cycles = 0;
  pattern_start_millis = millis();
}

/*
 * Executes current pattern
 */
void executeCurrentPattern() {
  cycles += 1;
  if (pattern == 0)
    pattern_01();

  else if (pattern == 1)
    pattern_02();

  else if (pattern == 2)
    pattern_03();

  else if (pattern == 3)
    pattern_04();

  else if (pattern == 4)
    pattern_05();

  else if (pattern == 5)
    pattern_06();

  else if (pattern == 6)
    pattern_07();

  // Something wierd happened, let's restart
  else {
    #if DEBUG
      Serial.println("Do no know how to handle pattern, restarting");
    #endif
    resetPatternSequence();
  }
}

/*
 * Prints a pattern
 */
void setView(CRGB l1, CRGB l2, CRGB l3, CRGB l4, unsigned long del) {
  leds[0] = l1;
  leds[1] = l2;
  leds[2] = l3;
  leds[3] = l4;
  FastLED.show();

  delay(del);
}

// XXX --- --- ---
// --- XXX --- ---
// --- --- XXX ---
// --- --- --- XXX
void pattern_01() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, MAIN_DELAY);
}

// XXX --- --- ---
// --- XXX --- ---
// --- --- XXX ---
// --- --- --- XXX
// --- --- XXX ---
// --- XXX --- ---
void pattern_02() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
}

// XXX XXX --- ---
// --- --- XXX XXX
void pattern_03() {
  setView(CRGB::White, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::White, CRGB::White, MAIN_DELAY);
}

// XXX --- --- ---
// XXX --- --- XXX
void pattern_04() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::White, MAIN_DELAY);
}

// XXX --- --- ---
// --- --- --- XXX
void pattern_05() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, MAIN_DELAY);
}

// XXX --- --- ---
// XXX XXX --- ---
// XXX XXX XXX ---
// XXX XXX XXX XXX
// --- --- --- ---
void pattern_06() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::White, CRGB::White, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::White, CRGB::White, CRGB::White, MAIN_DELAY);
  setView(CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, MAIN_DELAY);
}

// XXX --- --- XXX
// XXX --- XXX ---
// XXX XXX --- ---
// XXX --- XXX ---
void pattern_07() {
  setView(CRGB::White, CRGB::Black, CRGB::Black, CRGB::White, MAIN_DELAY);
  setView(CRGB::White, CRGB::Black, CRGB::White, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::White, CRGB::Black, CRGB::Black, MAIN_DELAY);
  setView(CRGB::White, CRGB::Black, CRGB::White, CRGB::Black, MAIN_DELAY);
}

/*
 * Returns light value
 */
double getLightValue() {
  light_history[4] = light_history[3];
  light_history[3] = light_history[2];
  light_history[2] = light_history[1];
  light_history[1] = light_history[0];

  int val = 0;
  val += analogRead(A0);
  val += analogRead(A0);
  val += analogRead(A0);
  val += analogRead(A0);

  Serial.println(val / 4.0);

  light_history[0] =
    (val / 4.0)      * 0.4 +
    light_history[1] * 0.2 +
    light_history[2] * 0.2 +
    light_history[3] * 0.1 +
    light_history[4] * 0.1;

  return light_history[0];
}
