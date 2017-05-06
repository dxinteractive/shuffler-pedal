//
// SHUFFLER
//

#include <Arduino.h>
#include <Wire.h>

//
// PINS
//

const int PIN_INPUT_LED = 2;
const int PIN_SELECTOR_SWITCH_1_LED = 3;
const int PIN_OUTPUT_LED = 4;
const int PIN_SELECTOR_SWITCH_2_LED = 5;
const int PIN_INPUT_SWITCH = 6; // HIGH when on, LOW when off
const int PIN_SELECTOR_SWITCH_1 = 7; // HIGH when normal, LOW when Z loop is toggled
const int PIN_SELECTOR_SWITCH_2 = 8; // HIGH when normal, LOW when Z loop is toggled
const int PIN_KILL_RELAY = 9; // HIGH to kill, LOW for normal
const int PIN_SELECTOR_1 = A0; // ~0 A, ~180 B, ~360 C, ~590 D, ~800 E, ~1023 bypass
const int PIN_SELECTOR_2 = A1; // ~0 A, ~180 B, ~360 C, ~590 D, ~800 E, ~1023 bypass
const int PIN_SELECTOR_3 = A2; // ~0 A, ~180 B, ~360 C, ~590 D, ~800 E, ~1023 bypass
const int PIN_SELECTOR_4 = A3; // ~0 A, ~180 B, ~360 C, ~590 D, ~800 E, ~1023 bypass
const int PIN_WIRE_1 = A4; // WHITE WIRE
const int PIN_WIRE_2 = A5; // BLACK WIRE
const int PIN_OUTPUT_SWITCH = A6; // ~510 when normal, ~1023 when off, ~0 when F loop is first

//
// STATE
//

int selector_1 = 5; // 0 A, 1 B, 2 C, 3 D, 4 E, 5 bypass
int selector_2 = 5; // 0 A, 1 B, 2 C, 3 D, 4 E, 5 bypass
int selector_3 = 5; // 0 A, 1 B, 2 C, 3 D, 4 E, 5 bypass
int selector_4 = 5; // 0 A, 1 B, 2 C, 3 D, 4 E, 5 bypass
int input_switch = 0; // 0 off, 1 on
int selector_switch_1 = 0; // 0 normal, 1 Z loop is in use
int selector_switch_2 = 0; // 0 normal, 1 Z loop is in use
int output_switch = 0; // 0 normal, 1 F loop is in front, 2 off
int feedback_detected = 0; // 0 normal, 1 means selector config has created a feedback loop
int feedback_cooldown = 0; // countdown after feedback detected before reactivating relay

int loops = 0;

int readSelector(int pin) {
  int val = analogRead(pin);
  if(val > 1000) {
    return 5;
  }
  if(val > 700) {
    return 4;
  }
  if(val > 450) {
    return 3;
  }
  if(val > 250) {
    return 2;
  }
  if(val > 90) {
    return 1;
  }
  return 0;
}

void requestSelectors() {
  byte message[] = {
    selector_1,
    selector_2,
    selector_3,
    selector_4,
    input_switch,
    selector_switch_1,
    selector_switch_2,
    output_switch
  };

  Wire.write(message,8);
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_INPUT_LED, OUTPUT);
  pinMode(PIN_SELECTOR_SWITCH_1_LED, OUTPUT);
  pinMode(PIN_OUTPUT_LED, OUTPUT);
  pinMode(PIN_SELECTOR_SWITCH_2_LED, OUTPUT);
  pinMode(PIN_KILL_RELAY, OUTPUT);

  Wire.begin(8); // join i2c bus with address #8
  Wire.onRequest(requestSelectors);
}



void loop() {
  
  if(digitalRead(PIN_INPUT_SWITCH)==HIGH) {
    input_switch = 1;
  } else {
    input_switch = 0;
  }

  if(digitalRead(PIN_SELECTOR_SWITCH_1)==HIGH) {
    selector_switch_1 = 0;
  } else {
    selector_switch_1 = 1;
  }

  if(digitalRead(PIN_SELECTOR_SWITCH_2)==HIGH) {
    selector_switch_2 = 0;
  } else {
    selector_switch_2 = 1;
  }

  int output_switch_read = analogRead(PIN_OUTPUT_SWITCH);
  if(output_switch_read > 1000) {
    output_switch = 2;
  } else if(output_switch_read > 400) {
    output_switch = 0;
  } else {
    output_switch = 1;
  }

  selector_1 = readSelector(PIN_SELECTOR_1);
  selector_2 = readSelector(PIN_SELECTOR_2);
  selector_3 = readSelector(PIN_SELECTOR_3);
  selector_4 = readSelector(PIN_SELECTOR_4);

  int by_loop[6] = {}; 
  by_loop[selector_1]++;
  by_loop[selector_2]++;
  by_loop[selector_3]++;
  by_loop[selector_4]++;  

  feedback_detected = 0;
  if(by_loop[0]>1 || by_loop[1]>1 || by_loop[2]>1 || by_loop[3]>1 || by_loop[4]>1) {
    feedback_detected = 1;
  }

  //
  // KILL RELAY
  //

  if(feedback_detected && !input_switch) {
    digitalWrite(PIN_KILL_RELAY, HIGH);
    feedback_cooldown = 10;
  } else if(feedback_cooldown > 0) {
    feedback_cooldown--;
  } else {
    digitalWrite(PIN_KILL_RELAY, LOW);
  }

  //
  // INPUT LED
  //
  
  if(feedback_detected) {
    if(input_switch) {
      digitalWrite(PIN_INPUT_LED, HIGH);
    } else {
      if(loops%4<2) {
        digitalWrite(PIN_INPUT_LED, LOW);
      } else {
        digitalWrite(PIN_INPUT_LED, HIGH);
      }
    }
  } else {
    if(input_switch) {
      digitalWrite(PIN_INPUT_LED, HIGH);
    } else {
      digitalWrite(PIN_INPUT_LED, LOW);
    }
  }

  //
  // OUTPUT LED
  //
  
  if(output_switch==2) {
    if(loops%2<1) {
        digitalWrite(PIN_OUTPUT_LED, LOW);
      } else {
        digitalWrite(PIN_OUTPUT_LED, HIGH);
      }
  } else if(output_switch==1) {
    digitalWrite(PIN_OUTPUT_LED, HIGH);
  } else {
    digitalWrite(PIN_OUTPUT_LED, LOW);
  }

  //
  // SELECTOR SWITCH 1
  //

  if(selector_switch_1) {
    digitalWrite(PIN_SELECTOR_SWITCH_1_LED, HIGH);
  } else {
    digitalWrite(PIN_SELECTOR_SWITCH_1_LED, LOW);
  }

  //
  // SELECTOR SWITCH 2
  //

  if(selector_switch_2) {
    digitalWrite(PIN_SELECTOR_SWITCH_2_LED, HIGH);
  } else {
    digitalWrite(PIN_SELECTOR_SWITCH_2_LED, LOW);
  }
  
  delay(50);

  loops++;
  if(loops > 7) {
    loops = 0;
  }
}