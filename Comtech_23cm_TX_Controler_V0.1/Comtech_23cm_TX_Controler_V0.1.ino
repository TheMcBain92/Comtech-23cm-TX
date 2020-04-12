/*Comtech 23cm transmitter - rotary encoder & LCD tuner using logic to determine frequency

Release v0.1 - April 2020 / Arduino IDE version 1.8.12

Adapted by Stephen McBain M5SJM from origional code by
Adrian Leggett - amateur radio callsign M0NWK

This example code is in the public domain.

Using Comtech 23cm transmitter, Arduino UNO, 8 channel Solid State Relay (SSR) module, rotary encoder and 
20 x 4 Liquid Crystal Display (LCD) this sketch:

- Detects clockwise / anti-clockwise direction of rotary encoder
- Calculates relay state based on logic using counters
- Sets relay to on / off state according to logic
- Displays frequency on LCD

Pinout:
D2: Button Press
D3: Rotary Clk
D4: Rotary Detect
D5-12: Relays 1-8
D13: TXon Output

A5: LCD SCL
A4: LCD SDA

*/

/*-----( Import libraries )-----*/
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

//Setup Variables & Fixed Values (EG. Pin Numbers to names)
byte ky_040_clk = 3;               // Connected to CLK on KY-040 - date output 1
byte ky_040_dt = 4;                // Connected to DT on KY-040 - data output 2
byte TXon = 13;                    // TX Control Pin
byte TXState = 0;                  // Is TX ON or OFF
byte buttonpin = 2;                // Pin connected to rotary button
byte buttonpos = 3;                // Button position
byte buttonlaststate = 0;          // Is button currently pressed
byte buttoncount = 0;              // Button Count
byte buttoncountlast = 0;          // Last Button count
byte encoderPosCount = 0;          // 1240MHz - 0 = 1240MHz & 260 = 1367.5MHz
byte currentFreqDecVal=0;          // Stores the decimal value from the array based on current rotary encoder position value
float DisplayFrequency=0000.00;   // Stores the calculated frequency based on current rotary encoder position value
int ky_040_clkValLast;            // Store last value from CLK to determine by comparison the position
int ky_040_clkVal;                // Store CLK value from KY-040
boolean ky_040_status;            // Clockwise = True, Anti-clockwise = False
const char *codeversion = "V0.1";

// Variables to store counter values from relay logic calulation
byte relay_1_val=0;
byte relay_2_val=0;
byte relay_3_val=0;
byte relay_4_val=0;
byte relay_5_val=0;
byte relay_6_val=0;
byte relay_7_val=0;
byte relay_8_val=0;

// Variables to store relay states from relay logic calulation
byte relay_1_state;
byte relay_2_state;
byte relay_3_state;
byte relay_4_state;
byte relay_5_state;
byte relay_6_state;
byte relay_7_state;
byte relay_8_state;

void setup(){ // Setup runs once
  Serial.begin(9600); //Start Serial
  lcd.begin (20, 4);  //LCD Size (horrizontal, Vertical) 
    
  // LCD Backlight ON
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);

  pinssetup(); //Call code to set default state of relay control pins
  serialsetup(); //Call code to print startup messages to Serial Connection
  layout(); //Call code to print the LCD Welcome message & running LCD Layout
}

void layout() {
  // -- LCD Welcome Message
  lcd.setCursor(2, 0); // go home on LCD
  lcd.print("23cm Transmitter");
  lcd.setCursor(7, 1);
  lcd.print(codeversion);
  lcd.setCursor(2, 3);
  lcd.print("mcbainsite.co.uk");
  delay(3000);
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Adapted By");
  lcd.setCursor(3, 1);
  lcd.print("Stephen McBain");
  lcd.setCursor(7, 2);
  lcd.print("M5SJM");
  lcd.setCursor(1, 3);
  lcd.print("Origional by M0NWK");
  delay(3000);
  defaultfreq(); //Call code to print default frequency on screen
}

void defaultfreq() {
  // Display default frequency on LCD
  DisplayFrequency = 1240.00 + (encoderPosCount * 0.5);
  lcd.clear();
  lcd.setCursor(2, 0); // go home on LCD
  lcd.print("23cm Transmitter");
  lcd.setCursor(4,1);
  lcd.print("Transmit Off");
  lcd.setCursor(1,2); // Set cursor position on LCD
  lcd.print("Frequency: ");
  lcd.print(DisplayFrequency); // Print msg to LCD
  lcd.setCursor(1,3); // Set cursor position on LCD
}

void serialsetup() {
  // Print welcome message to serial monitor
  Serial.println("Comtech 23cm Transmitter");
  Serial.println();
  Serial.println(codeversion);
  Serial.println();
  Serial.println("Adapted by Stephen McBain - M5SJM");

  // Print default frequency and rotary encoder position to serial monitor
  Serial.println();
  Serial.println("***********************");
  Serial.println();
  Serial.print("Frequency = 1240MHz");
  Serial.println();
  Serial.print("Default encoder position = ");
  Serial.println(encoderPosCount);
  Serial.println();
  Serial.print("***********************");
  Serial.println();
}

void pinssetup() {
  // Set digital pin type (INPUT / OUTPUT). 5-12 = relays where 5 = relay 1 and 12 = relay 8
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);         
  pinMode(9, OUTPUT);           
  pinMode(10, OUTPUT);           
  pinMode(11, OUTPUT);           
  pinMode(12, OUTPUT);                   
  pinMode (ky_040_clk,INPUT);pinMode (ky_040_dt,INPUT);     
  ky_040_clkValLast = digitalRead(ky_040_clk); // Get value of KY-040 CLK. Tells you the last position 
  pinMode(TXon, OUTPUT);
  pinMode(buttonpin, INPUT_PULLUP);

  //Ensure TX if off
  digitalWrite(TXon,LOW);

  // Set relays for default 1240MHz (1 = HIGH, 2 = LOW). 1240MHz = 00000000
  digitalWrite(5,0);
  digitalWrite(6,0);
  digitalWrite(7,0);
  digitalWrite(8,0);
  digitalWrite(9,0);
  digitalWrite(10,0);
  digitalWrite(11,0);
  digitalWrite(12,0);  
}

void loop() {
  if (digitalRead(buttonpin) == LOW){
    buttonpos = 1;
    if (buttonpos != buttonlaststate){
      buttoncount++;
      buttonlaststate = buttonpos;
    }
  }else{
    buttonpos = 0;
    buttonlaststate = buttonpos;
  }
  
  if (buttoncount > buttoncountlast){
    buttoncountlast = buttoncount;
    if (TXState == 0){
      digitalWrite(TXon,HIGH);
      TXState = 1;
      Serial.println();
      Serial.println("TX On");
      lcd.setCursor(13,1);
      lcd.print("On ");
    }else{
      digitalWrite(TXon,LOW);
      TXState = 0;
      Serial.println();
      Serial.println("TX Off");
      lcd.setCursor(13,1);
      lcd.print("Off");
    }
  }

  ky_040_clkVal = digitalRead(ky_040_clk); // Read value from KY-040 CLK and store in variable

  if (ky_040_clkVal != ky_040_clkValLast){ // Compare KY-040 current value to last value. If not the same this means rotary encoder is rotating
    digitalWrite(TXon,LOW);
    TXState = 0;
    lcd.setCursor(13,1);
    lcd.print("Off");
    if (digitalRead(ky_040_dt) != ky_040_clkVal) {  // Rotary encoder is rotating clockwise
      if (encoderPosCount <= 253){ // If rotary encoder position is less than or equal to maximum encoder value (highest frequency) then increment encoder position by 1.
        encoderPosCount ++;  // Increment encoder position variable by 1 each iteration
        // Use counters to determine state of each relay
    
        if (relay_1_val == 0){
          relay_1_val ++;
          relay_1_state = 1;
        }else if (relay_1_val == 1) {
          relay_1_val --;
          relay_1_state = 0;
        }
               
        relay_2_val ++; // Relay 2
        
        if (relay_2_val < 2){
          relay_2_state = 0;
        } else if (relay_2_val >= 2 && relay_2_val < 4){
          relay_2_state=1;
        } else if (relay_2_val = 3){
          relay_2_state =0;
          relay_2_val = 0;
        }
        
        relay_3_val ++;  // Relay 3
        
        if (relay_3_val < 4){
          relay_3_state = 0;
        } else if (relay_3_val >= 4 && relay_3_val < 8){
          relay_3_state=1;
        } else if (relay_3_val = 7){
          relay_3_state =0;
          relay_3_val = 0;
        }
          
        relay_4_val ++; // Relay 4
        
        if (relay_4_val < 8){
          relay_4_state = 0;
        } else if (relay_4_val >= 8 && relay_4_val < 16){
          relay_4_state=1;
        } else if (relay_4_val = 15){
          relay_4_state =0;
          relay_4_val = 0;
        }
        
        relay_5_val ++;  // Relay 5
        
        if (relay_5_val < 16){
          relay_5_state = 0;
        } else if (relay_5_val >= 16 && relay_5_val < 32){
          relay_5_state=1;
        } else if (relay_5_val = 31){
          relay_5_state =0;
          relay_5_val = 0;
        }
        
        relay_6_val ++; // Relay 6
        
        if (relay_6_val < 32){
          relay_6_state = 0;
        } else if (relay_6_val >= 32 && relay_6_val < 64){
          relay_6_state=1;
        } else if (relay_6_val = 63){
          relay_6_state =0;
          relay_6_val = 0;
        }
        
        relay_7_val ++; // Relay 7
        
        if (relay_7_val < 64){
          relay_7_state = 0;
        } else if (relay_7_val >= 64 && relay_7_val < 128){
          relay_7_state=1;
        } else if (relay_7_val = 127){
          relay_7_state =0;
          relay_7_val = 0;
        }
        
        relay_8_val ++; // Relay 8
        
        if (relay_8_val < 128){
          relay_8_state = 0;
        } else if (relay_8_val >= 128 && relay_8_val < 256){
          relay_8_state=1;
        } else if (relay_8_val = 255){
          relay_8_state =0;
          relay_8_val = 0;
        }
        
        // Print relay value counters to serial monitor
        
        Serial.println();
        Serial.println("Relay value counters:");  
        Serial.print(relay_1_val);
        Serial.print(relay_2_val);
        Serial.print(relay_3_val);
        Serial.print(relay_4_val);
        Serial.print(relay_5_val);
        Serial.print(relay_6_val);
        Serial.print(relay_7_val);
        Serial.println(relay_8_val);
        Serial.println();
        
        // Print relay states to serial monitor
        
        Serial.println("Relay state values:");        
        Serial.print(relay_1_state);
        Serial.print(relay_2_state);
        Serial.print(relay_3_state);
        Serial.print(relay_4_state);
        Serial.print(relay_5_state);
        Serial.print(relay_6_state);
        Serial.print(relay_7_state);
        Serial.println(relay_8_state);       
      }
           
      RelayStatus(); // Call function containing common code 
      ky_040_status = true; // Store rotation status as true
    } else {  // Rotary encoder is rotating anti-clockwise
      ky_040_status = false; // Store rotation status as false
      if (encoderPosCount > 0){   
        encoderPosCount --; // Decrement encoder position variable by 1 each iteration
    
        // Use counters to determine state of each relay
        if (relay_1_val == 0){
          relay_1_val =1;
          relay_1_state = 1;
        } else if (relay_1_val = 1) {
          relay_1_val=0;
          relay_1_state = 0;
        }
                        
        if (relay_2_val == 0){
          relay_2_val=3;
          relay_2_state = 0;
        } else if ((relay_2_val > 0) && relay_2_val < 2){
          relay_2_val --;
          relay_2_state = 0;
        } else if (relay_2_val >= 2 && relay_2_val < 4){
          relay_2_val --;
          relay_2_state=1;
        }
         
        if (relay_3_val == 0){
          relay_3_val=7;
          relay_3_state = 0;
        } else if ((relay_3_val > 0) && relay_3_val < 4){
          relay_3_val --;
          relay_3_state = 0;
        } else if (relay_3_val >= 4 && relay_3_val < 8){
          relay_3_val --;
          relay_3_state=1;
        }
                  
        if (relay_4_val == 0){
          relay_4_val=15;
          relay_4_state = 0;
        } else if ((relay_4_val > 0) && relay_4_val < 8){
          relay_4_val --;
          relay_4_state = 0;
        } else if (relay_4_val >= 8 && relay_4_val < 16){
          relay_4_val --;
          relay_4_state=1;
        }
        
        if (relay_5_val == 0){
          relay_5_val=31;
          relay_5_state = 0;
        } else if ((relay_5_val > 0) && relay_5_val < 16){
          relay_5_val --;
          relay_5_state = 0;
        } else if (relay_5_val >= 16 && relay_5_val < 32){
          relay_5_val --;
          relay_5_state=1;
        }
        
        if (relay_6_val == 0){
          relay_6_val=63;
          relay_6_state = 0;
        } else if ((relay_6_val > 0) && relay_6_val < 32){
          relay_6_val --;
          relay_6_state = 0;
        } else if (relay_6_val >= 32 && relay_6_val < 64){
          relay_6_val --;
          relay_6_state=1;
        }
        
        if (relay_7_val == 0){
          relay_7_val=63;
          relay_7_state = 0;
        } else if ((relay_7_val > 0) && relay_7_val < 64){
          relay_7_val --;
          relay_7_state = 0;
        } else if (relay_7_val >= 64 && relay_7_val < 128){
          relay_7_val --;
          relay_7_state=1;
        }
                   
        if (relay_8_val == 0){
          relay_8_val=127;
          relay_8_state = 0;
        } else if ((relay_8_val > 0) && relay_8_val < 128){
          relay_8_val --;
          relay_8_state = 0;
        } else if (relay_8_val >= 128 && relay_8_val < 256){
          relay_8_val --;
          relay_8_state=1;
        }
        
        
        // Print relay value counters to serial monitor       
        Serial.println();
        Serial.println("Relay value counters:");  
        Serial.print(relay_1_val);
        Serial.print(relay_2_val);
        Serial.print(relay_3_val);
        Serial.print(relay_4_val);
        Serial.print(relay_5_val);
        Serial.print(relay_6_val);
        Serial.print(relay_7_val);
        Serial.println(relay_8_val);
        Serial.println();
        
        // Print relay states to serial monitor  
        Serial.println("Relay state values:");        
        Serial.print(relay_1_state);
        Serial.print(relay_2_state);
        Serial.print(relay_3_state);
        Serial.print(relay_4_state);
        Serial.print(relay_5_state);
        Serial.print(relay_6_state);
        Serial.print(relay_7_state);
        Serial.println(relay_8_state);       
      }                
      RelayStatus();  // Call function containing common code     
    }            
    Serial.println();    
  }   
}

// Common code called by loop for clockwise / anti-clockwise rotation
void RelayStatus() { 
  // Display on LCD current frequency based on current rotary encoder position
  DisplayFrequency = 1240.00 + (encoderPosCount * 0.5);
  lcd.setCursor(12,2); // Set cursor position on LCD
  lcd.print(DisplayFrequency); // Print msg to LCD
  
  // Print to serial monitor the current rotary encoder position, frequency and direction of rotation
  
  Serial.println();
  Serial.println (String("Encoder position: ") + encoderPosCount);
  Serial.println (String("Frequency: ") + DisplayFrequency);
  Serial.print (String("Rotated: "));
  
  if (ky_040_status == 1){
    Serial.println("Clockwise");
    } else if (ky_040_status == 0) {
      Serial.println("Anti-clockwise");
    }
    // Set state of each relay
    digitalWrite(5,relay_1_state);
    digitalWrite(6,relay_2_state);
    digitalWrite(7,relay_3_state);
    digitalWrite(8,relay_4_state);
    digitalWrite(9,relay_5_state);
    digitalWrite(10,relay_6_state);
    digitalWrite(11,relay_7_state);
    digitalWrite(12,relay_8_state);
}
