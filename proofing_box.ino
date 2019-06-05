// Bread Proofing Box
// michael@mwild.me
//
// Maintains a constant temperature inside a proofing box by turning on/off
// a heating pad connected to a relay.
//

#include "DHT.h"
#include "SoftwareSerial.h"

const int pin_disp_tx = 8; // pin that the display RX is connected to.
const int pin_disp_rx = 7; // needed to create the software serial. doesn't need physical connection
const int baud_rate = 9600; // set correct baud rate. default of s7s is 9600
const int disp_brightness = 255;
const int pin_dht = 2; // pin that the DHT is connected to
const int dht_type = DHT22; // DHT 22 (AM2302)
const int update_freq = 2000; // time to wait between loops

char buff[10]; // display output buffer

SoftwareSerial disp(pin_disp_rx, pin_disp_tx); // the display

DHT dht(pin_dht, dht_type); // the temperature sensor

void setup() {
  // init display
  disp.begin(baud_rate); 
  clearDisplay();
  setBrightness(disp_brightness);
  delay(100);
  disp.print("HELO");

  // init dht
  dht.begin();
}

void loop() {
  delay(update_freq);
  
  float humid = dht.readHumidity(); 
  float temp = dht.readTemperature(false);

  if (isnan(temp) || isnan(humid)) {
    disp.print("Err ");
    return;
  }

  // todo: should i use the heat_index or just the temp?
  float heat_index = dht.computeHeatIndex(temp, humid, false);

  writeTemperature(heat_index);
}

void writeTemperature(float temp) {
  // temp is a float. we need an int (well actually a string) to send to the display
  // we multiply by 100 and put a decimal place at position 2
  setDecimals(0b000010);
  sprintf(buff, "%4d", (int) (temp * 100)); 
  disp.print(buff);
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
