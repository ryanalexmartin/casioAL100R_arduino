// Right now the velocity response curve is very linear and doesn't feel good, but I'll wait until the piano is put back together and I can use the real keys to test that.
// Also, I have an issue with gibberish serial information.  what's the deal?

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();


bool debug = 0;  // If you're getting gibberish make sure you disable this.  Also, make sure you're using the correct MIDI library.
int maxPressTime = 200;
int minVelocity = 10;
int maxVelocity = 127;
int midiChannel = 1;

const int numRows = 2;
const int numCols = 2;
int rows[numRows] = {7, 6}; // Change these values to the actual pin numbers where your rows are connected
int cols[numCols] = {8, 9}; // Change these values to the actual pin numbers where your columns are connected

bool pressed[numCols][numRows] = {{false}};

unsigned long pressStart[numCols][numRows] = {{0}};

void setup() {
  if(debug) {
    Serial.begin(9600);
  }
  else {
      MIDI.begin(4);                      // Launch MIDI and listen to channel 4
  }

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
//      int midi = r * numCols + c + 21; // Lowest note should be 21 (A0)?
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
        if (pressTime <= maxPressTime) { // min velocity?
          int midi = (r - 1) * numCols + c + 22; // Lowest note should be 21 (A0)?

          noteOn(midiChannel, midi, velocity);
          delay(3);
//          MidiUSB.flush();
          if (debug) {
            Serial.print("on ");
            Serial.print(midi );
            Serial.print(", ");
            Serial.println(velocity);

          }
        }
      }

      // key is now fully unpressed
      if (!isConnected && isPressed && !isDown) {
        pressed[c][r] = false;
        int midi = r * numCols + c + 22; // Lowest note should be 21 (A0)?  // probably shouldn't declare the same variable twice, to save memory

        noteOff(midiChannel, midi, 0);
        delay(3);
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
    if( pitch > 21) {
          MIDI.sendNoteOn(pitch, velocity, channel);
    }
}

void noteOff(byte channel, byte pitch, byte velocity) {
//  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
//  MidiUSB.sendMIDI(noteOff);
    MIDI.sendNoteOff(pitch, 0, channel);
}
