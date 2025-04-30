//Constants
#include <FastLED.h>
#define NUM_CELLS 15
#define NUM_LEDS NUM_CELLS*4
#define DATA_PIN 2
//#define threshold 20

//LED Strip Object
CRGB leds[NUM_LEDS];

//Object for the Cells
struct Cell {
  int button; //pin for the velostat
  unsigned long baselineSum;
  int baseline;
  int ledIndex; //first index of leds
  int reading = 0; //latest velostat reading
  int smoothedReading = 0;
  int threshold; //The threshold is dynamic
  int peak = baseline + 5;

  int brightness = 0; //brightness value used in tapShow method

  //Initializes the cell
  void create(int velostatPin, int ledIndexStart) {
    button = velostatPin;
    ledIndex = ledIndexStart;
  }

  //Calculates the baseline reading value
  void calibrate() {
    baselineSum = 0;
    for (int i = 0; i < 50; i++) {
      baselineSum += analogRead(button);
    }
    baseline = round(baselineSum/50);
    peak = baseline + 5;

    //Based on the baseline, calculate the appropriate threshold
    //The lower the baseline, the more sensitive the sensor is, so
    //the larger the threshold for an actual tap.
    threshold = map(baseline, 0, 1023, 25, 1);
  }

  void readVelostat() {
    reading = analogRead(button);
    smoothedReading = smoothedReading*0.9 + reading*0.1;

    if(reading > peak) {
      peak = reading;
      int travel = peak - baseline;
      //float percentageOfTravelToTrigger = ;
      int newThreshold = round(travel*map(travel, 0, 1023, 5, 95)/100);
      if(newThreshold > threshold) {
        threshold = newThreshold;
      }
    }
  }

  void tapShow() {
    readVelostat();
    
    if ( millis() % 50 ) {
      if (reading > baseline + threshold) {
        brightness++;
      } else {
        brightness--;
      }
    }

    brightness = constrain(brightness, 0, 255);
    for ( int i = 0; i < 4; i++) {
      leds[ledIndex + i] = CRGB(brightness, 0, brightness);
    }
  }
};

//Create cell array
Cell cells[NUM_CELLS];

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2815, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed

  //Add the cells to the array and find baseline reading
  for (int i = 0; i < NUM_CELLS; i++) {
    pinMode(54 + i, INPUT);
    cells[i].create(54 + i, i * 4);
    cells[i].calibrate();
  }
}

int debugging = 0;

void loop() {
  //Light color testing
//    for ( int i = 0; i < NUM_LEDS; i++) {
//      leds[i] = CRGB(255, 0, 255);
//    }

  for ( int i = 0; i < NUM_CELLS; i++) {
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

  Serial.println(cells[debugging].smoothedReading);
//  Serial.print(cells[debugging].reading);
//  Serial.print(',');
//  Serial.print(cells[debugging].baseline + cells[debugging].threshold);
//  Serial.print(',');
//  Serial.println(cells[debugging].baseline);
}

void calibrationSequence() {
  for (int i = 0; i < NUM_CELLS; i++) {
    cells[i].calibrate();
  }
}
