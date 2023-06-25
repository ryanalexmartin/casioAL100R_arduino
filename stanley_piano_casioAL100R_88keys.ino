// This code is working for 1 column and 2 rows.  I'll slowly start to add more rows and columns, and then soon add a mux.  And then two more muxes.
// Need to fix the MIDI stuff to use MIDI.h instead of MIDIUSB.h.  That way, we can use MOCOLUFA to write simple Arduino code while still having MIDIUSB functionality (direct to USB)
// I only have two muxes on me right now.  The shop down the way ordered some, but I can get them at HO HUA if it's urgent.

// Right now the velocity response curve is very linear and doesn't feel good, but I'll wait until the piano is put back together and I can use the real keys to test that.


#include "MIDI.h"

bool debug = 1;
int maxPressTime = 250;
int minVelocity = 10;
int maxVelocity = 90;
int midiChannel = 0;

const int numRows = 2;
const int numCols = 1;
int rows[numRows] = {7, 6}; // Change these values to the actual pin numbers where your rows are connected
int cols[numCols] = {8}; // Change these values to the actual pin numbers where your columns are connected

bool pressed[numCols][numRows] = {{false}};

unsigned long pressStart[numCols][numRows] = {{0}};

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numRows; ++i) {
    pinMode(rows[i], INPUT_PULLUP);
  }
  for (int i = 0; i < numCols; ++i) {
    pinMode(cols[i], OUTPUT);
    digitalWrite(cols[i], HIGH); // Ensure cols are HIGH
  }
}

void loop() {
  for (int c = 0; c < numCols; ++c) {
    digitalWrite(cols[c], LOW); // Activate the current column
    for (int r = 0; r < numRows; ++r) {
      int midi = r * numCols + c + 38;
      bool isConnected = digitalRead(rows[r]) == LOW;
      bool isDown = r % 2 == 1;  // if r is odd, it's an SI (second input).  Remember that we start at 0
      bool isPressed = pressed[c][r] == true;

      // keypress starts
      if (isConnected && !isPressed && !isDown && !pressStart[c][r]) {
        if (debug) {
          Serial.print("start pressing ");
          Serial.println(r);
        }
        pressStart[c][r] = millis();
      char buffer[100];
      sprintf(buffer, "pressStart[%d][%d] was set to ", c, r);
      Serial.print(buffer);
      Serial.println(pressStart[c][r]);
      //        pressed[c][r] = true;
      }

      // key is now fully pressed
      if (isConnected && !pressed[c][r - 1] && isDown) {
        unsigned long pressTime = millis() - pressStart[c][r - 1]; //fetch value from previous row
        float f = (1 - (float)pressTime / (float)maxPressTime);
        int velocity = f * (maxVelocity - minVelocity) + minVelocity;
        if (debug) {
          Serial.print("done pressing ");
          Serial.println(pressTime);
          Serial.println(f);
          
        }
        pressed[c][r - 1] = true; // subtract one from the row value so that we only use the FI rows.  The SI rows (even numbered) are therefore unused.  There must be a better way to do this.
        if (pressTime <= maxPressTime && velocity > 0.01) { // min velocity?
          noteOn(midiChannel, midi, velocity);
//          MidiUSB.flush();
          if (debug) {
            Serial.print("on ");
            Serial.print(midi );
            Serial.println(velocity);

          }
        }
      }

      // key is now fully unpressed
      if (!isConnected && isPressed && !isDown) {
        pressed[c][r] = false;
        noteOff(midiChannel, midi, 0);
//        MidiUSB.flush();
        if (debug) {
          Serial.print("off ");
          Serial.print(midi);
          Serial.println(" ");
        }
      }
          // key is unpressed => may include keypresses that have started but did not go down
      if (!isConnected && !isDown) {
        pressStart[c][r] = 0; // reset time when key is released completely
      }
    }
    digitalWrite(cols[c], HIGH); // Deactivate the current column before moving to next
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
//  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
//  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
//  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
//  MidiUSB.sendMIDI(noteOff);
}
