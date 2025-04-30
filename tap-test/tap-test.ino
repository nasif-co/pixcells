#include <FastLED.h>
#define NUM_LEDS 15*4
#define DATA_PIN 2

int val = 0;
float avgVal;

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2815, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  pinMode(A0, INPUT);
  calibrate();
}

void loop() {
  //Readback velostat
  int button = analogRead(A0);
  if ( millis() % 50 ) {
    if (button > floor(avgVal) + 5) {
      val++;
    } else {
      val--;
    }
  }

  val = constrain(val, 0, 255);
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(val, 0, val);
  }
  FastLED.show();
}

void calibrate() {
  calibrate(50);
}

void calibrate(int iterations) {
  for (int i = 0; i < iterations; i++) {
    avgVal = avgVal + analogRead(A0);
  }
  avgVal = avgVal/iterations;
}
