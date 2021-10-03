#include <Control_Surface.h>

#include "Arduino.h"
#include "PCF8574.h"

void IRQ1();
void IRQ2();

void TurnLed(boolean state, byte note);

//USBDebugMIDI_Interface midi;
USBMIDI_Interface midi;
//HairlessMIDI_Interface midi;



PCF8574 IOBank [7] = {PCF8574(0x3F, 2, IRQ1), PCF8574(0x3B, 3, IRQ2), PCF8574(0x3D, 2, IRQ1), PCF8574(0x39, 3, IRQ2), PCF8574(0x3E, 2, IRQ1), PCF8574(0x3A, 3, IRQ2), PCF8574(0x24, 2, IRQ1)};

//const uint8_t ledarray [37][2] = {{3,6},{6,6},{6,9},{9,10},{10,12},{12,13},{13,16},{16,19},{19,20},{20,21},{21,22},{22,25},{26,28},{28,29},{29,32},{32,33},{33,36},{36,37},{37,40},{40,43},{43,44},{44,46},{46,47},{47,50},{51,53},{53,54},{54,57},{57,57},{57,60},{60,61},{61,63},{64,67},{67,67},{67,70},{70,71},{71,73},{74,77}};

const uint8_t manyPcf = 7;

boolean IRQ1Changed = true;
boolean IRQ2Changed = true;

uint8_t port[7][8];
uint8_t currentport[7][8];
boolean CClatchControl[6];

const int midishift = 29;
uint8_t velocity = 0x7F;

unsigned long last_tick = 0UL;

const byte light = 4;
uint8_t lightIs = LOW;
unsigned long lighTime = 0;
int lightInterval = 400;

uint8_t keySplit = false;
uint8_t activeBank[3];
const CS::Channel chnames [6] = {CHANNEL_1, CHANNEL_2, CHANNEL_3, CHANNEL_4, CHANNEL_5, CHANNEL_6};

CCPotentiometer potentiometer[] {
  {A0, MIDI_CC::General_Purpose_Controller_1},
  {A1, MIDI_CC::General_Purpose_Controller_2},
  {A2, MIDI_CC::General_Purpose_Controller_3}
};


constexpr analog_t minimumValue = 255;
constexpr analog_t maximumValue = 16383 - 255;

analog_t mappingFunction(analog_t raw) {
  raw = constrain(raw, minimumValue, maximumValue);
  return map(raw, minimumValue, maximumValue, 0, 16383);
}

void setup()
{ 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  potentiometer[0].map(mappingFunction);
  potentiometer[1].map(mappingFunction);
  potentiometer[2].map(mappingFunction);

  Control_Surface.begin(); // Initialize Control Surface
  
  midi.begin();
  
  delay(1000);
    
  for (int i = 0; i < manyPcf; i++)
  {
    int irq = (i %2) ? 2 : 1;
    for (int p = 0; p < 8; p++){
      IOBank[i].pinMode(p, INPUT);
    } 
    IOBank[i].begin();
  }

}

void loop()
{  
  midi.update();

  int bpm = map(analogRead(A0), 0, 1023, 20, 280); 
  lightInterval = 60000 / (bpm * 4);

  unsigned long currentMillis = millis();
   
   if(currentMillis - lighTime >= lightInterval){
    lighTime = currentMillis;
    if (lightIs == LOW){
      lightIs = HIGH;
    } else {
      lightIs = LOW;
    }
    digitalWrite(light, lightIs);
       digitalWrite(LED_BUILTIN, lightIs);
  }
  
  unsigned long us_per_tick = (unsigned long) (1e6 / (bpm * 24.0 / 60.0)) ;
  if ((micros () - last_tick) >= us_per_tick)
  {
    last_tick += us_per_tick ; // schedule next tick
    midi.sendTimingClock();
  }
  
  Control_Surface.loop();

	if (IRQ1Changed){
      IRQ1Changed= false;

        for (uint8_t i = 0; i < manyPcf; i++){
          int check = (i %2) ? false : true;
          if (check) {
            PCF8574::DigitalInput  di = IOBank[i].digitalReadAll();
            uint8_t* p = (uint8_t*) &di;
            for (int ii = 0; ii < 8; ii++) {
              port[i][ii] = !(p[ii]);
            }
          }
        }
        Change();
	}
 
  if (IRQ2Changed){
      IRQ2Changed= false;

       for (uint8_t i = 0; i < manyPcf; i++){
          int check = (i %2) ? true : false;
          if (check) {
            PCF8574::DigitalInput  di = IOBank[i].digitalReadAll();
            uint8_t* p = (uint8_t*) &di;
            for (int ii = 0; ii < 8; ii++) {
              port[i][ii] = !(p[ii]);
            }
          }
        }
        Change();   
  }
}

void Change(){
  for (int i = 0; i < manyPcf; i++){
    for (int p = 0; p < 8; p++){
      if (currentport[i][p] != port[i][p]){
        currentport[i][p] = port[i][p];
        int key = (i*8 + p);
        int note = key + midishift;       
          // 37 Regular Keyboard keys
          if (key <= 37){
            for (int b = 0; b < 3; b++){       
              if (activeBank[b] == true ){                  
                  int c = b;
                  if (keySplit){
                    if (key >= 19){
                      c = b + 3;
                    } else {
                      note = key + midishift + 12;
                    }
                  }
                  if (port[i][p]){
                    midi.sendNoteOn({note, chnames[c] }, velocity);         // send a note on event                  
                  } else {
                    midi.sendNoteOff({note, chnames[c] }, velocity);        // send a note off event
                  }                     
              }  
            }
            
          }
          
          // Other Keys
          if (key >= 38){           
            // Hardwarebuttons Vibrato - String
            if (key <= 43  && key >= 41) {              
                if (port[i][p]){
                  activeBank[key - 41 ] = true;
                } else {
                  activeBank[key - 41 ] = false;
                }              
            }
            
            // Split keyboard
            if (key == 40) {
              if (port[i][p]){
                keySplit = true;
              } else {
                keySplit = false;
              }
            }
            
            // Button (On/Off switch)
            if (key == 54) {
              if (port[i][p]){                
                midi.sendStart();
                midi.sendNoteOff({key-48, CHANNEL_16 }, velocity);
//              midi.sendContinue();
              } else {
                midi.sendStop(); 
              }
            }
  
            // Buttons 1-6 Waltz > Latin
            if (key >= 48 && key <= 53) {
              if (port[i][p]){
                if (CClatchControl[key-48]){                  
                  MIDIAddress controller = {64 + key - 48, CHANNEL_16};
                  midi.sendControlChange(controller, 0);
                  midi.sendNoteOn({key-48, CHANNEL_16 }, velocity);
                  midi.sendNoteOff({key-48, CHANNEL_16 }, velocity);
                  CClatchControl[key-48] = false;
                } else {
                  MIDIAddress controller = {64 + key - 48, CHANNEL_16};
                  midi.sendControlChange(controller, 127);
                  midi.sendNoteOn({key-48, CHANNEL_16 }, velocity);
                  midi.sendNoteOff({key-48, CHANNEL_16 }, velocity);
                  CClatchControl[key-48] = true;
                }   
              }
            }                 
        }
      }
    }
  }
}

void IRQ1(){
	 IRQ1Changed = true;
}
void IRQ2(){
   IRQ2Changed = true;
}
