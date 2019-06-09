// Bread Proofing Box
// michael@mwild.me
//
// Maintains a constant temperature inside a proofing box by turning on/off
// a heating pad connected to a relay.
//

#include "DHT.h"
#include "SoftwareSerial.h"

const int pin_enco_1 = 2; // 2/3 are special pins. the encoder must be here
const int pin_enco_2 = 3;
const int pin_enco_sw = 4; // encoder switch
const int pin_disp_tx = 8; // pin that the display RX is connected to.
const int pin_disp_rx = 7; // needed to create the software serial. doesn't need physical connection
const int baud_rate = 9600; // set correct baud rate. default of s7s is 9600
const int disp_brightness = 255;
const int pin_dht = 12; // pin that the DHT is connected to
const int dht_type = DHT22; // DHT 22 (AM2302)
const int update_freq = 2000; // time to wait between loops

char buff[10]; // display output buffer
volatile int enco_prev = 0; // previous encoder direction
volatile long enco_value = 25; // the current encoder value
int target_temp;

SoftwareSerial disp(pin_disp_rx, pin_disp_tx); // the display

DHT dht(pin_dht, dht_type); // the temperature sensor

void setup() {
  // init display
  disp.begin(baud_rate); 
  clearDisplay();
  setBrightness(disp_brightness);
  delay(100);
  disp.write("JELO");

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


  // init system
  setTargetTemperature(enco_value);
}

void loop() {

  if (enco_value != target_temp) {
    // encoder was rotated
    setTargetTemperature(enco_value);
    
  }


  float humid = dht.readHumidity(); 
  float temp = dht.readTemperature(false);

  if (isnan(temp) || isnan(humid)) {
    disp.print("Err ");
    return;
  }

  // todo: should i use the heat_index or just the temp?
  float heat_index = dht.computeHeatIndex(temp, humid, false);

  writeTemperature(heat_index);

  delay(update_freq);
}

void writeTemperature(float temp) {
  // temp is a float. we need an int (well actually a string) to send to the display
  // we multiply by 100 and put a decimal place at position 2
  setDecimals(0b000010);
  sprintf(buff, "%4d", (int) (temp * 100)); 
  disp.print(buff);
}

void setTargetTemperature(int target) {
  target_temp = target;
  setDecimals(0b010000);
  sprintf(buff, "T %2d", target_temp);
  disp.print(buff);

  delay(2000);
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
  int msb = digitalRead(pin_enco_1);
  int lsb = digitalRead(pin_enco_2);

  int enco = (msb << 1) | lsb; // convert the 2 pin value to single number
  int sum = (enco_prev << 2) | enco; // compare to the previous value

  // add or subtract based on rotation
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) enco_value++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) enco_value--;

  enco_prev = enco; // save for next time
}
