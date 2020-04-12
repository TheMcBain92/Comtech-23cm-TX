/*
 Sets the frequency of the ATV transmitter.

  The circuit:
  - LCD and RTC connected Analog pin4 + pin5 I2c bus
  - Temp Sensors OneWire to Digital pin2 (4.7K Resistor between VCC and Data Line)
  - SD Card -MISO pin12
            -MOSI pin11
            -SCK pin13
            -CS pin10 Changable, See //SetupSD below.

  created 2018
  by Stephen McBain <http://mcbainsite.co.uk>
 
*/
const char *codeversion = "V0.0.1";

#include <Wire.h>                  // I2C Master lib for ATTinys which use USI
#include <LiquidCrystal_I2C.h>
// -- LCD Setup Assign pins to names
#define I2C_ADDR 0x27  //Address location of I2C LCD
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

//Setup LCD Pins etc...
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

// The TinyWire library uses the lower 7 bits in the address byte, 
// shifts them left and adds a 1 for write and 0 for read. See line 46 of TinyWireM.cpp.
// 
// The address of the SP055 is 1100 0010 for write, 
// See datasheets/SP5055.PDF, table 4 and table 1.
// 00 : 0V to 0,2V
// 01 : Always valid (don't know what this means in the datasheet)
// 10 : 0,3V to 0,7V
// 11 : 0,8V to 13,2V
//                   `-------vv
#define SP5055_ADDR   B01100001  // 0xC2 >> 1
 
const int ledPin   = 13;
const int audioPin = 3; // D3 (pin 2)

const char PROGMEM CWTEXT[] = "PI4RCG = WWW.BALLONVOSSENJACHT.NL = ";

void setup() {
  Serial.begin(9600); //Start Serial
  Wire.begin();       // initialize I2C lib
  lcd.begin (20, 4);  //LCD Size (horrizontal, Vertical) 
  
  pinMode(ledPin, OUTPUT);
  pinMode(audioPin, OUTPUT);
  
  digitalWrite(ledPin, HIGH);
  setFrequency(1252000000L);
  digitalWrite(ledPin, LOW);
  
  // LCD Backlight ON
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);

  // -- LCD Welcome Message
  lcd.setCursor(1, 0); // go home on LCD
  lcd.print("Comtech 23cm Transmitter");
  lcd.setCursor(7, 1);
  lcd.print(codeversion);
  lcd.setCursor(2, 3);
  lcd.print("mcbainsite.co.uk");
  delay(3000);
  headder();
}

void headder() //Setup LCD with logging active message
{
  Serial.println(F("Adapted by Stephen McBain"));
  lcd.clear();
  lcd.home();
  lcd.print(F("Callsign M5SJM"));
  layout();
}

void layout()// standard layout of screen
{
  lcd.setCursor(0, 1);
  lcd.print(F("VHF:"));
  lcd.setCursor(10, 1);
  lcd.print(F("IN :"));
  lcd.setCursor(0, 2);
  lcd.print(F("OUT:"));
  lcd.setCursor(10, 2);
  lcd.print(F("HF :"));
  lcd.setCursor(0, 3);
  lcd.print(F("ATV:"));
  lcd.setCursor(10, 3);
  lcd.print(F("S6 :N/A"));
  loop;
}

void loop() {
  readfrequency();
  digitalWrite(ledPin, HIGH);
  setFrequency(1252000000L);
  digitalWrite(ledPin, LOW);
}

void readfrequency() {
  
}

void setFrequency(unsigned long ftx) {
  /*
   * Pseudo-code for calculating the divider
   * 
   * Ftx = Fcomp * 16 * divider
   * Where Fcomp = 4MHz/512 = 7812,5 Hz
   * 
   * Which means: 
   * divider = Ftx/(16 * Fcomp)
   * divider = Ftx/(16 * 7812,5)
   * divider = Ftx/(125.000)
   * 
   * Example:
   * Ftx = 1252 MHz
   * divider = (1.252.000.000 / 125.000)
   * divider = (1.252.000 / 125)
   * divider = 10.016 (decimal)
   * divider = 0010 0111 0010 0000 (binary)
   *              2    7   2    0
   * 
   */

  unsigned long divider = ftx / 125000;

  byte dividerLSB = (byte) divider;
  byte dividerMSB = (byte) (divider >> 8);

  /*
   * Code for sending stuff over i2c to the SP5055. Draft.
   */
  Wire.beginTransmission(SP5055_ADDR); // Address SP5055.
  
  Wire.write(dividerMSB);           // Programmable Divider MSB
                                        // bit 7: Always 0
                                        // bit 6 to 0: 2^14 to 2^8
                                        
  Wire.write(dividerLSB);           // Programmable Divider LSB
                                        // bit 7 to 0: 2^7 to 2^0

  Wire.write(B10001110);            // Charge pump and test bits.
                                        // bit 7: Always 1
                                        // bit 6: CP, Charge pump: 
                                        //        0 = 50uA
                                        //        1 = 170uA
                                        // bit 5: T1 test mode bit,
                                        //        0 = normal mode
                                        //        1 = test mode, connects Fcomp to P6 
                                        //            and Fdiv to P7.
                                        // bit 4: T0 test mode bit,
                                        //        0 = normal mode
                                        //        1 = disable charge pump
                                        // bit 3: Always 1
                                        // bit 2: Always 1
                                        // bit 1: Always 1
                                        // bit 0: OS, Varactor drive disable switch
                                        //        0 = normal mode
                                        //        1 = Disable charge pump drive amplifier
  
  Wire.write(B00000000);            // I/O port control bits (blinkin' lights)
                                        // bit 7: P7
                                        // bit 6: P6
                                        // bit 5: P5
                                        // bit 4: P4
                                        // bit 3: P3
                                        // bit 2: don't care
                                        // bit 1: don't care
                                        // bit 0: P0
  
  Wire.endTransmission();          // Send to the slave
}
