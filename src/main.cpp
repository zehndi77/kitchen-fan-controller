#include <Arduino.h>

/**************
 *  Dimmer Project
 *  **************

 *  ---------------------- OUTPUT & INPUT Pin table ---------------------
 *  +---------------+-------------------------+-------------------------+
 *  |   Board       | INPUT Pin               | OUTPUT Pin              |
 *  |               | Zero-Cross              |                         |
 *  +---------------+-------------------------+-------------------------+
 *  | Mega   /
 *  | pro16MHzatmega328
 *                  | D2 (NOT CHANGABLE)      | D0-D1, D3-D70           |
 *  +---------------+-------------------------+-------------------------+
 *  | Uno           | D2 (NOT CHANGABLE)      | D0-D1, D3-D20           |
 *  +---------------+-------------------------+-------------------------+
 */

#include <RBDdimmer.h> //

// #define USE_SERIAL  SerialUSB //Serial for boards whith USB serial port
#define USE_SERIAL Serial
#define outputPin 4  // D4
#define zerocross D2 // for boards with CHANGEBLE input pins

#define LEDPIN LED_BUILTIN

#define EXTERN_ANALOG_PIN A0 // analog pin for 0..10V external control voltage input
#define POTI_ANALOG_PIN A3   // analog pin for potentiometer

#define MODE_0_PIN 8 // D8 digital pin for mode 0 input
#define MODE_1_PIN 9 // D9 digital pin for mode 1 input

dimmerLamp dimmer(outputPin); // initialase port for dimmer for MEGA, Leonardo, UNO, Arduino M0, Arduino Zero

int outVal = 0;
int analogVal = 0;
int prevAnalogVal = 0;
unsigned long boostStartTime = 0;
bool boostActive = false;
const unsigned long BOOST_DURATION = 10000; // 10 seconds in ms
const int LOW_THRESHOLD = 6; // 1% of 1023
const int RISE_THRESHOLD = 5; // 5% of 1023

enum Mode
{
  EXTERNAL_MODE_100,
  EXTERNAL_MODE_90,
  ON_BOARDPOTI_MODE,
  FIXED_MODE_90
};

byte mode = EXTERNAL_MODE_100;
String modeNames[] = {"EXTERNAL_MODE_100", "EXTERNAL_MODE_90", "ON_BOARDPOTI_MODE_100", "FIXED_MODE_90"};

byte readMode(void)
{
  byte currentMode;
  // Read mode pins
  bool mode0 = digitalRead(MODE_0_PIN);
  bool mode1 = digitalRead(MODE_1_PIN);
  if (!mode0 && !mode1)
    currentMode = EXTERNAL_MODE_100;
  else if (!mode0 && mode1)
    currentMode = EXTERNAL_MODE_90;
  else if (mode0 && !mode1)
    currentMode = ON_BOARDPOTI_MODE;
  else if (mode0 && mode1)
    currentMode = FIXED_MODE_90;
  return currentMode;
}

void setup()
{
  USE_SERIAL.begin(115200);
  USE_SERIAL.println("############################");
  USE_SERIAL.println("Dimming with potentiometer");
  USE_SERIAL.println("############################");
  USE_SERIAL.println("SWITCH 1 |  2 : MODE");
  USE_SERIAL.println("       0 |  0 : FIXED_MODE_90 ");
  USE_SERIAL.println("       1 |  0 : External Mode 90% ");
  USE_SERIAL.println("       0 |  1 : On-Board Potentiometer Mode ");
  USE_SERIAL.println("       1 |  1 : External Mode 100% ");
  USE_SERIAL.println("############################");

  pinMode(LEDPIN, OUTPUT);
  pinMode(MODE_0_PIN, INPUT_PULLUP);
  pinMode(MODE_1_PIN, INPUT_PULLUP);

  dimmer.begin(NORMAL_MODE, ON); // dimmer initialisation: name.begin(MODE, STATE)
}
byte counter = 0;
void loop()
{
  if (counter < 255)
  {
    counter++;
  }

  prevAnalogVal = analogVal;
  analogVal = analogRead(EXTERN_ANALOG_PIN);

  // Check for boost trigger conditions
  // Condition 1: Input falls below 10%
  if (analogVal < LOW_THRESHOLD && prevAnalogVal >= LOW_THRESHOLD)
  {
    boostStartTime = millis();
    boostActive = true;
    USE_SERIAL.println("  [BOOST ACTIVATED due to FALL] ");
  }
  // Condition 2: Input rises from below 3% to above 10%
  else if (prevAnalogVal < RISE_THRESHOLD && analogVal > LOW_THRESHOLD)
  {
    boostStartTime = millis();
    boostActive = true;
    USE_SERIAL.println("  [BOOST ACTIVATED due to RISE] ");
  }

  // Check if boost duration has expired
  if (boostActive && (millis() - boostStartTime >= BOOST_DURATION))
  {
    boostActive = false;
    USE_SERIAL.println("  [BOOST DEACTIVATED due to time] ");
  }

  mode = readMode();

  if (boostActive)
  {
    // Boost mode: set output to 100% (or scaled)
    outVal = 81;
  }
  else if (mode == ON_BOARDPOTI_MODE)
    outVal = map(analogRead(POTI_ANALOG_PIN), 0, 1023, 0, 100); // analogRead(analog_pin), min_analog, max_analog, 0%, 100%);
  else if (mode == FIXED_MODE_90)
    outVal = 90;
  else if (mode == EXTERNAL_MODE_100)
    outVal = map(analogVal, 0, 1023, 0, 100); // analogRead(analog_pin), min_analog, max_analog, 0%, 100%);
  else if (mode == EXTERNAL_MODE_90)
    outVal = map(analogVal, 0, 1023, 8, 82); // analogRead(analog_pin), min_analog, max_analog, 100%, 0%);
    // adapt 10/90 to values that fit 0V and 10V respectively, adapted to real values without oversteering
  dimmer.setPower(outVal); // name.setPower(0%-100%)

// Debugging for the first 200 loops
  if (counter < 200)
  {
    //  Serial output
    USE_SERIAL.print("  Mode:   ");
    USE_SERIAL.print(modeNames[mode]);
    USE_SERIAL.print("  |  AnalogVal:   ");
    USE_SERIAL.print(analogVal);
    if (boostActive)
    {
      USE_SERIAL.print("  [BOOST ACTIVE]");
    }
    USE_SERIAL.print("  OutVal[%]:   ");
    USE_SERIAL.println(outVal);
  }

  // Visual feedback on LED 0% short flash  (2ms ON)  to 100% long flash (200ms ON)
  delay(2 * outVal);
  digitalWrite(LEDPIN, 0);
  delay(250 - (2 * outVal));
  digitalWrite(LEDPIN, 1);
}
