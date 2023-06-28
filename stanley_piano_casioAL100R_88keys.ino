#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

// Column Mux
#define S0_PIN 2
#define S1_PIN 3
#define S2_PIN 4
#define COLUMN_CONTROL_PIN 13

// Row Mux 0, 1, 2
#define ROW_S0_PIN 5
#define ROW_S1_PIN 6
#define ROW_S2_PIN 7

#define MUX_1_OUT_PIN 8
#define MUX_2_OUT_PIN 9
#define MUX_3_OUT_PIN 10

bool debug = 0;
int maxPressTime = 100;
int minVelocity = 8;
int maxVelocity = 127;
int midiChannel = 1;

const int numRows = 22;
const int numCols = 8;  // Now we have 8 columns

bool pressed[88] = {false};
unsigned long pressStart[88] = {0};

void setMux(int channel) { // TODO - rename to setColumnMux
  // A = least significant bit
  // bitRead also starts from least significant bit
  digitalWrite(S0_PIN, bitRead(channel, 0));
  digitalWrite(S1_PIN, bitRead(channel, 1));
  digitalWrite(S2_PIN, bitRead(channel, 2));
}

void setRowMux(int channel) {
  // A = least significant bit
  // bitRead also starts from least significant bit
  digitalWrite(ROW_S0_PIN, bitRead(channel, 0));
  digitalWrite(ROW_S1_PIN, bitRead(channel, 1));
  digitalWrite(ROW_S2_PIN, bitRead(channel, 2));
}

int readMux(int row) {
   setRowMux(row % 8);
   int val;

  if (row < 8) { // rows 0-7 on MUX1
    val = digitalRead(MUX_1_OUT_PIN);
//    Serial.print("Reading row ");
//    Serial.print(row);
//    Serial.print(": ");
//    Serial.println(val);
//    delay(200);
  } else if (row < 16) { // rows 8-15 on MUX2
    val = digitalRead(MUX_2_OUT_PIN);
  } else { // rows 16-21 on MUX3
    val = digitalRead(MUX_3_OUT_PIN);
  }
  
  return val;
}

int midiFromColumnRow(int c, int r) {
  // subtract by one for SI
  int midi = (numCols * (r/2)) + c;
  return midi;
}


void setup() {
  if(debug) {
    Serial.begin(9600);
  }
  else {
      MIDI.begin(4);                      // Launch MIDI and listen to channel 4
  }

  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(COLUMN_CONTROL_PIN, OUTPUT);
  digitalWrite(COLUMN_CONTROL_PIN, HIGH);  // set inactive state to HIGH (deactivates column)

  pinMode(ROW_S0_PIN, OUTPUT);
  pinMode(ROW_S1_PIN, OUTPUT);
  pinMode(ROW_S2_PIN, OUTPUT);

  pinMode(MUX_1_OUT_PIN, INPUT_PULLUP);
  pinMode(MUX_2_OUT_PIN, INPUT_PULLUP);
  pinMode(MUX_3_OUT_PIN, INPUT_PULLUP);
}

void loop() {
  for (int c = 0; c < numCols; ++c) {
  //    Serial.print("Reading column ");
  //    Serial.print(c);
  //    Serial.println(": ");
      setMux(c);  // select the current column with the multiplexer
      digitalWrite(COLUMN_CONTROL_PIN, LOW);  // set selected column to LOW

    for (int r = 0; r < numRows; ++r) {
//      bool isConnected = digitalRead(rows[r]) == LOW;
      bool isConnected = readMux(r) == LOW;
//      if(debug && isConnected) {
//        Serial.print("Key connected: ");
//        Serial.print(c);
//        Serial.print("_");
//        Serial.print(r);
//        Serial.println(" ");
//      }
      bool isDown = r % 2 == 1;  // if r is odd, it's an SI (second input).  Remember that we start at 0
      bool isPressed = pressed[(numCols * (r/2)) + c] == true;

      // keypress starts

      if (isConnected && !isPressed && !isDown && !pressStart[midiFromColumnRow(c, r)]) {
        if (debug) {
          Serial.print("start pressing ");
          Serial.println(r);
          Serial.print("... calculating (numCols * (r/2) + c: ");
          Serial.println(midiFromColumnRow(c, r));
        }
        pressStart[midiFromColumnRow(c, r)] = millis();
      }

      // key is now fully pressed
      if (isConnected && !pressed[midiFromColumnRow(c, r-1)] && isDown) {
        unsigned long pressTime = millis() - pressStart[midiFromColumnRow(c, r-1)]; //fetch value from previous row
        float f = (1 - (float)pressTime / (float)maxPressTime);
        int velocity = f * (maxVelocity - minVelocity) + minVelocity;
        if (debug) {
          Serial.print("done pressing, press time: ");
          Serial.println(pressTime);
          Serial.print("f: ");
          Serial.println(f);
          Serial.print("... calculating (fully pressed)");
          Serial.println(midiFromColumnRow(c, r-1));
          
        }
        pressed[midiFromColumnRow(c, r-1)] = true; // subtract one from the row value so that we only use the FI rows.  The SI rows (even numbered) are therefore unused.  There must be a better way to do this.
        if (pressTime <= maxPressTime) { // min velocity?
          int midi = midiFromColumnRow(c, r-1) + 21; // Lowest note should be 21 (A0)?

          noteOn(midiChannel, midi, velocity);
//          delay(20);
//          MidiUSB.flush();
          if (debug) {
            Serial.print("Row: ");
            Serial.println(r);
            Serial.print("on ");
            Serial.print(midi );
            Serial.print(" , velocity: ");
            Serial.println(velocity);

          }
        }
      }

      // key is now fully unpressed
      if (!isConnected && isPressed && !isDown) {
        pressed[midiFromColumnRow(c, r)] = false;
        int midi = midiFromColumnRow(c, r) + 21; // Lowest note should be 21 (A0)?  // probably shouldn't declare the same variable twice, to save memory

        noteOff(midiChannel, midi, 0);
//        delay(2000);
//        MidiUSB.flush();
        if (debug) {
          Serial.print("Row: ");
          Serial.println(r);
          Serial.print("off ");
          Serial.print(midi);
          Serial.println(" ");
        }
      }
          // key is unpressed => may include keypresses that have started but did not go down
      if (!isConnected && !isDown) {
        pressStart[midiFromColumnRow(c, r)] = 0; // reset time when key is released completely
      }
    }
    // Don't need the below line because the mux is controlling our column pins
//    digitalWrite(cols[c], HIGH); // Deactivate the current column before moving to next
    digitalWrite(COLUMN_CONTROL_PIN, HIGH);  // set selected column back to HIGH before moving to the next
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
//  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
//  MidiUSB.sendMIDI(noteOn);
    if( pitch >= 21) {
          MIDI.sendNoteOn(pitch, velocity, channel);
    }
}

void noteOff(byte channel, byte pitch, byte velocity) {
//  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
//  MidiUSB.sendMIDI(noteOff);
    MIDI.sendNoteOff(pitch, 0, channel);
}
