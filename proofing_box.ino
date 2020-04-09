// Bread Proofing Box
// michael@mwild.me
//
// Maintains a constant temperature inside a proofing box by turning on/off
// a heating pad connected to a relay.
//

#include "DHT.h"
#include "SoftwareSerial.h"

const boolean debug = false;
const int pin_enco_1 = 2; // 2/3 are special pins. the encoder must be here
const int pin_enco_2 = 3;
const int pin_enco_sw = 7; // encoder switch
const int pin_disp_tx = 4; // pin that the display RX is connected to.
const int pin_disp_rx = 11; // needed to create the software serial. doesn't need physical connection
const int baud_rate = 9600; // set correct baud rate. default of s7s is 9600
const int disp_brightness = 255;
const int pin_dht = 5; // pin that the DHT is connected to
const int dht_type = DHT22; // DHT 22 (AM2302)
const int update_freq = 2000; // time to wait between loops
const float enco_sensitivity = 0.25; // how much does the encoder need to turn to update 1 deg c
const int pin_relay = 6; // pin the relay is connected to
const float cushion_temp = 2.0; // how many degrees change is required to change the relay state

char buff[10]; // display output buffer
volatile int enco_prev = 0; // previous encoder direction
volatile long enco_value = 100; // the current encoder value
volatile float target_temp;
boolean setup_mode = false;

SoftwareSerial disp(pin_disp_rx, pin_disp_tx); // the display

DHT dht(pin_dht, dht_type); // the temperature sensor

void setup() {
  if (debug) {
    Serial.begin(baud_rate);
  }

  // init display
  disp.begin(baud_rate); 
  clearDisplay();
  setBrightness(disp_brightness);

  // init dht
  dht.begin();

  // init encoder
  pinMode(pin_enco_1, INPUT);
  pinMode(pin_enco_2, INPUT);
  pinMode(pin_enco_sw, INPUT);

  digitalWrite(pin_enco_1, HIGH); // turn pullup resistor on
  digitalWrite(pin_enco_2, HIGH); // turn pullup resistor on
  digitalWrite(pin_enco_sw, HIGH); // turn pullup resistor on

  attachInterrupt(0, updateEncoder, CHANGE); // call updateEncoder() when interrupt 0 (pin 2) or 1 (pin 3) change
  attachInterrupt(1, updateEncoder, CHANGE);

  // init relay
  pinMode(pin_relay, OUTPUT);
  switchRelay(false);

  // init system
  target_temp = enco_value * enco_sensitivity;
  writeTargetTemperature();
  delay(3000);
}

void loop() {

  if (!digitalRead(pin_enco_sw)) {
    setup_mode = !setup_mode;
  }

  if (setup_mode) {
    writeTargetTemperature();
  
  } else {
    float temp = dht.readTemperature(false);
  
    if (isnan(temp)) {
      disp.print("Err ");
      return;
    }
  
    writeTemperature(temp);

    bool relayOn = relayState();
    float cushion = cushion_temp / 2.0;
    
    float cushioned_target = relayOn
      ? target_temp + cushion
      : target_temp - cushion;

    if (debug) { Serial.print("target: "); Serial.println(cushioned_target); }

    switchRelay(temp <= cushioned_target);
    
  }
  
  delay(update_freq);
}

void writeTemperature(float temp) {
  // temp is a float. we need an int (well actually a string) to send to the display
  // we multiply by 100 and put a decimal place at position 2
  setDecimals(0b000010);
  sprintf(buff, "%4d", (int) (temp * 100)); 
  disp.print(buff);

  if (debug) { Serial.print("current: "); Serial.println(temp); }
}

void writeTargetTemperature() {
  if (debug) {
    Serial.println(target_temp);
  }
  setDecimals(0b010000);
  sprintf(buff, "T %2d", (int) target_temp);
  disp.print(buff);

  delay(500);
}

void clearDisplay() {
  disp.write(0x76); // clear command byte
}

void setBrightness(byte value) {
  disp.write(0x7A);  // brightness command byte
  disp.write(value); // (0--->255)
}

void setDecimals(byte decimals) {  
  disp.write(0x77);     // decimal command byte
  disp.write(decimals); // (Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
}

void updateEncoder() {
  if (!setup_mode) {
    return;
  }
  
  int msb = digitalRead(pin_enco_1);
  int lsb = digitalRead(pin_enco_2);

  int enco = (msb << 1) | lsb; // convert the 2 pin value to single number
  int sum = (enco_prev << 2) | enco; // compare to the previous value

  // add or subtract based on rotation
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) { enco_value++; }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) { enco_value--; }

  enco_prev = enco; // save for next time

  target_temp = enco_value * enco_sensitivity;
  writeTargetTemperature();
}

void switchRelay(boolean on) {
  if (on) {
    digitalWrite(pin_relay, HIGH);
    if (debug) { Serial.println("relay: on"); }
  } else  {
    digitalWrite(pin_relay, LOW);
    if (debug) { Serial.println("relay: off"); }
  }
}

bool relayState() {
  int value = digitalRead(pin_relay);
  bool state = (value == HIGH);
  if (debug) { Serial.println(state ? "relayState() -> on" : "relayState() -> off"); }
  return state;  
}
