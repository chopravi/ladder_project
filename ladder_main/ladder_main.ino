// Libraries

#include <Wire.h>
#include <math.h>
#include "Adafruit_MPR121.h"

// Pin definitions
#define beam1PinVoltage 5  // Digital pin 5 is wired to Optocoupler #1 (Beam break #1)
#define beam2PinVoltage 7  // Digital pin 7 is wired to Optocoupler #2 (Beam break #2)
#define beam3PinVoltage 9  // Digital pin 9 is wired to Optocoupler #3 (Beam break #3)
#define outputPinVoltage 3    //Digital pin 3 is wired to solid-state relay, which sends 5V when Mode = HIGH
#define rung1 22
#define rung2 23
#define rung3 24
#define rung4 25
#define rung5 26
#define rung6 27
#define rung7 28
#define rung8 29
#define rung9 30
#define rung10 31
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// defining variables
int minimumBlinkInterval = 5000;   // minimum interval at which to blink (milliseconds)
int maximumBlinkInterval = 10000;  // maximum interval at which to blink if the center beam is hit
const int arrayLength = 50;        // Number of cycles that the array is able to store
int touchCountArray[arrayLength];  // creating array to store touch readings, initialized to all zeros
int currentIndex = 0;              // Index for the current reading stored in array
bool ladderON = false;             // if mouse is on the ladder
unsigned long currentMillis = 0;   //timestamp variable, will be a counter
unsigned long previousMillis = 0;
bool lightOutput = false;  // Declare your output variabe

// Define the states for the LED light in response to beam break
enum class State {
  IDLE,   //default condition, light's off
  SHORT,  //two endbeams are broken, light's turned on temporarily
  LONG,   //two endbeams and centerbeam are both broken, light's turned on for a prolonged period of time
};

// Declare the state variable
State state = State::IDLE;

void setup() {
  pinMode(beam1PinVoltage, INPUT_PULLUP);  // Input pullup means that the signal for digital pin 5 is HIGH if circuit is switched off!
  pinMode(beam2PinVoltage, INPUT_PULLUP);  // Input pullup means that the signal for digital pin 7 is HIGH if circuit is switched off!
  pinMode(beam3PinVoltage, INPUT_PULLUP);  // Input pullup means that the signal for digital pin 9 is HIGH if circuit is switched off!
  Serial.begin(9600);                      // Init serial connection
  Serial.println("Basic MOSFET Driver Test");
  pinMode(outputPinVoltage, OUTPUT);
  while (!Serial) {  // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1)
      ;
  }
  Serial.println("MPR121 found!");
  pinMode(rung1, OUTPUT);
  pinMode(rung2, OUTPUT);
  pinMode(rung3, OUTPUT);
  pinMode(rung4, OUTPUT);
  pinMode(rung5, OUTPUT);
  pinMode(rung6, OUTPUT);
  pinMode(rung7, OUTPUT);
  pinMode(rung8, OUTPUT);
  pinMode(rung9, OUTPUT);
  pinMode(rung10, OUTPUT);
}

void loop() {
  // Update the timestamp
  currentMillis = millis();  // Update the timestamp every millisecond

  //getting capacitive touch input
  capacitive_touch();
  // Update the light states
  updateStateMachine();

  // Set the light output
  digitalWrite(outputPinVoltage, lightOutput);

  // put a delay so it isn't overwhelming
  delay(100);
}
//code for switching between states
void updateStateMachine() {
  bool beam1Broken = digitalRead(beam1PinVoltage);     // read state of beam #1
  bool beam2Broken = digitalRead(beam2PinVoltage);  // read state of beam #2
  bool beam3Broken = digitalRead(beam3PinVoltage);  // read state of beam #3
  switch (state) {
    case State::IDLE:
      lightOutput = LOW;
      // If the endbeam or centerbeam switch their logic, then change the state from idle to short
      if ((beam1Broken && ladderON == 0) || (beam3Broken && ladderON == 0)) {
        state = State::SHORT;
        previousMillis = currentMillis;
      }
      break;

    case State::SHORT:
      lightOutput = HIGH;
      // If the timestamp becomes greater than time 1, return to idle
      if (currentMillis - previousMillis >= minimumBlinkInterval) {
        state = State::IDLE;
      }
      // If centerbeam switch its logic and timestamp is less than time 2, switch to LONG
      else if (beam2Broken && currentMillis - previousMillis < maximumBlinkInterval) {
        state = State::LONG;
      }
      break;

    case State::LONG:
      lightOutput = HIGH;
      // If the endbeam or centerbeam switch their logic, then change the state from long to idle
      if (currentMillis - previousMillis > maximumBlinkInterval) {
        state = State::IDLE;
      }
      break;
  }
}
void capacitive_touch() {
  currtouched = cap.touched();
  touchCountArray[arrayLength];
  // Get the touch status as a 12-bit number
  int touchCount = 0;
  // Loop through each touch pad to count the number of touches
  for (uint8_t i = 0; i < 12; i++) {
    if (currtouched & (1 << i)) {
      touchCount++;
    }
  }
  // Store the touches in the array
  touchCountArray[currentIndex] = touchCount;
  // Move to the next index, replace the previous
  currentIndex = (currentIndex + 1) % arrayLength;
  for (int j = 0; j < arrayLength; j++) {
    Serial.print(touchCountArray[j]);
  }
  Serial.println(" ");
  //finding the sum of touchCountArray each cycle
  int sum = 0;
  for (int i = 0; i < arrayLength; i++) {
    sum += touchCountArray[i];
  }
  // Calculate the average
  float average = (float)sum / arrayLength;

  // Print the average to the serial monitor
  Serial.print("Average: ");
  Serial.println(average);

  //defining ladderON boolean
  if (average < 0.4) {
    ladderON = false;
  } else if (average >= 0.4) {
    ladderON = true;
  }
  Serial.print("LadderON: ");
  Serial.println(ladderON);
}
