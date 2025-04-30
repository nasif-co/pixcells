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
  float baseline;
  int ledIndex; //first index of leds
  int reading = 0; //latest velostat reading
  int smoothedReading = 0;
  int threshold; //The threshold is dynamic
  int usualMax = 0;

  int brightness = 0; //brightness value used in tapShow method

  //Initializes the cell
  void create(int velostatPin, int ledIndexStart) {
    button = velostatPin;
    ledIndex = ledIndexStart;
  }

  //Calculates the baseline reading value
  void calibrate() {
    baseline = 0;
    for (int i = 0; i < 50; i++) {
      baseline = baseline + analogRead(button);
    }
    baseline = baseline / 50;

    //Based on the baseline, calculate the appropriate threshold
    //The lower the baseline, the more sensitive the sensor is, so
    //the larger the threshold for an actual tap.
    threshold = map(baseline, 0, 1023, 30, 1);
  }

  void readVelostat() {
    reading = analogRead(button);
    smoothedReading = smoothedReading*0.9 + reading*0.1;

    if(reading > usualMax) {
      usualMax = reading;
      threshold = round((usualMax - baseline)/map(baseline, 0, 1023, 90, 0));
      //threshold = 
    }
    //Serial.print(smoothedReading);
    //Serial.print(',');
  }

  void tapShow() {
    readVelostat();
    
    if ( millis() % 50 ) {
      if (smoothedReading > floor(baseline) + threshold) {
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
  Serial.println(analogRead(A2));
}

void calibrationSequence() {
  for (int i = 0; i < NUM_CELLS; i++) {
    cells[i].calibrate();
  }
}
