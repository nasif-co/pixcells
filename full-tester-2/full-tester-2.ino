//Constants
#include <FastLED.h>
#define NUM_CELLS 15
#define NUM_LEDS NUM_CELLS * 4
#define DATA_PIN 2

#define MSPERBEAT 500
#define LEEWAY 300

//LED Strip Object
CRGB leds[NUM_LEDS];

int defaultDeviations[] = {
  0,
  0,
  23,
  11,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

//Object for the Cells
struct Cell {
  int button;  //pin for the velostat
  unsigned long baselineSum;
  int baseline;
  int ledIndex;     //first index of leds
  int reading = 0;  //latest velostat reading
  int smoothedReading = 0;
  int binary = 0;
  int lastBinary = 0;
  int debouncedBinary = 0;
  int deviation = 10;  //Calculated based on the baseline
  unsigned long lastDebounceTime = 0;
  unsigned long hits[4] = { 0, 0, 0, 0 };
  int hitCount = 0;
  int currentPattern = 0;
  CRGB cellColor = CRGB(200, 255, 116);

  uint8_t brightness = 128;  //brightness value used in tapShow method

  //Initializes the cell
  void create(int velostatPin, int ledIndexStart) {
    button = velostatPin;
    ledIndex = ledIndexStart;

    for (int i = 0; i < 4; i++) {
      leds[ledIndex + i] = cellColor;
    }
  }

  //Calculates the baseline reading value
  void calibrate() {
    baselineSum = 0;
    for (int i = 0; i < 50; i++) {
      baselineSum += analogRead(button);
    }
    baseline = round(baselineSum / 50);

    //Based on the baseline, calculate the appropriate deviation
    //The lower the baseline, the more sensitive the sensor is, so
    //the larger the threshold for an actual tap.
    deviation = map(baseline, 0, 1023, 20, 8);

    for (int i = 0; i < 4; i++) {
      hits[i] = 0;
    }
  }

  void readVelostat() {
    reading = analogRead(button);
    smoothedReading = smoothedReading * 0.9 + reading * 0.1;

    if (reading - smoothedReading > deviation) {
      //Peaking
      binary = 1;
    } else {
      //Not peaking
      binary = 0;
    }

    //debounce binary
    if (binary != lastBinary) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > 80) {
      // if the button state has changed:
      if (binary != debouncedBinary) {
        debouncedBinary = binary;

        if (binary == 1) {  //Its a hit
          //Add to hits and shift it back
          for (int i = 0; i < 3; i++) {
            hits[i] = hits[i + 1];
          }
          hits[3] = millis();

          checkRhythm();
        }
      }
    }

    lastBinary = binary;
  }

  void tapShow() {
    readVelostat();

    if (millis() % 50) {
      if (debouncedBinary > 0) {
        brightness = qadd8(brightness, 1);
      } else {
        brightness = qsub8(brightness, 1);
      }
    }

    brightness = constrain(brightness, 0, 255);
    for (int i = 0; i < 4; i++) {
      leds[ledIndex + i] = cellColor;
      leds[ledIndex + i].nscale8(brightness);
    }
  }

  void checkRhythm() {
    if (hits[0] != 0 && hits[1] != 0 && hits[2] != 0 && hits[3] != 0) {
      if (isPatternOne()) {
        currentPattern = 1;
        cellColor = CRGB(0, 255, 0);
      }
      else if (isPatternTwo()) {
        currentPattern = 2;
        cellColor = CRGB(255, 0, 0);
      }
      else if (isPatternThree()) {
        currentPattern = 3;
        cellColor = CRGB(0, 0, 200);
      }
      else if (isPatternFour()) {
        currentPattern = 4;
        cellColor = CRGB(255, 0, 200);
      }
    }
  }
  // hit 1, hit 2, hit 3, hit 4
  boolean isPatternOne() {
    if (
      (hits[1] - hits[0] - MSPERBEAT < LEEWAY) && 
      (hits[2] - hits[1] - MSPERBEAT < LEEWAY) && 
      (hits[3] - hits[2]) - MSPERBEAT < LEEWAY) 
    {
      return true;
    } else {
      return false;
    }
  }
  // hit 1, skip 2, hit 3, skip 4
  boolean isPatternTwo() {
    if (
      (hits[1] - hits[0]) - MSPERBEAT*2 < LEEWAY && 
      (hits[2] - hits[1]) - MSPERBEAT*2 < LEEWAY && 
      (hits[3] - hits[2]) - MSPERBEAT*2 < LEEWAY) 
    {
      return true;
    } else {
      return false;
    }
  }
  // hit 1, skip 2, hit 3, hit 4
  boolean isPatternThree() {
    if (
      (hits[1] - hits[0]) - MSPERBEAT*2 < LEEWAY && 
      (hits[2] - hits[1]) - MSPERBEAT < LEEWAY && 
      (hits[3] - hits[2]) - MSPERBEAT < LEEWAY) 
    {
      return true;
    } else {
      return false;
    }
  }
  // hit 1, hit 2, hit 3, skip 4
  boolean isPatternFour() {
    if (
      (hits[1] - hits[0]) - MSPERBEAT < LEEWAY && 
      (hits[2] - hits[1]) - MSPERBEAT < LEEWAY && 
      (hits[3] - hits[2]) - MSPERBEAT*2 < LEEWAY) 
    {
      return true;
    } else {
      return false;
    }
  }
};

//Create cell array
Cell cells[NUM_CELLS];

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2815, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(255);
  //Add the cells to the array and find baseline reading
  for (int i = 0; i < NUM_CELLS; i++) {
    pinMode(54 + i, INPUT);
    cells[i].create(54 + i, i * 4);
    cells[i].calibrate();
  }

  cells[2].deviation = defaultDeviations[2];
  cells[3].deviation = defaultDeviations[3];
  //cells[4].deviation = defaultDeviations[2];
}

int debugging = 0;

void loop() {
  //Light color testing
  //    for ( int i = 0; i < NUM_LEDS; i++) {
  //      leds[i] = CRGB(255, 0, 255);
  //    }

  for (int i = 0; i < NUM_CELLS; i++) {
    cells[i].tapShow();
  }
  FastLED.show();
  //Serial.println();


  //Set the cell being debugged
  if (Serial.available()) {
    int input = Serial.parseInt();
    while (Serial.available() > 0) {
      Serial.read();
    }
    if (input >= 0 && input <= 14) {
      debugging = input;
    }
  }

  //Serial.print(cells[debugging].debouncedBinary * 1023); //blue
  //Serial.print(',');
  //Serial.print(cells[debugging].smoothedReading); // red
  //Serial.print(',');
  //Serial.print(cells[debugging].reading); //green
  //Serial.print(',');
  //orange
  //Serial.println(cells[debugging].deviation);
  for (int i = 0; i < 4; i++) {
    Serial.print(',');
    Serial.print(cells[debugging].hits[i]);
  }
  Serial.println();
  //Serial.println(cells[debugging].currentPattern);
}

void calibrationSequence() {
  for (int i = 0; i < NUM_CELLS; i++) {
    cells[i].calibrate();
  }
}
