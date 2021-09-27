#include <Control_Surface.h>

#include "Arduino.h"
#include "PCF8574.h"


// To use in low memory mode and prevent use of 7byte you must decomment the line
#define PCF8574_LOW_MEMORY
// in the library

// Function interrupt
void IRQ1();
void IRQ2();

//USBDebugMIDI_Interface midi;
//USBMIDI_Interface midi;
HairlessMIDI_Interface midi;

// The velocity of the note events
const uint8_t velocity = 0x7F;

PCF8574 IOBank [8] = {PCF8574(0x3F, 2, IRQ1), PCF8574(0x3B, 3, IRQ2), PCF8574(0x3D, 2, IRQ1), PCF8574(0x39, 3, IRQ2), PCF8574(0x3E, 2, IRQ1), PCF8574(0x3A, 3, IRQ2), PCF8574(0x24, 2, IRQ1), PCF8574(0x20, 3, IRQ2)};
int pcfPorts [8] = {0, 1, 2, 3, 4 ,5, 6, 7};
int manyPcf = 5;

int IRQ1Changed = false;
int IRQ2Changed = false;

int midishift = 41;

int pfc [] = {};

int port[8][8];
int currentport[8][8];

void setup()
{
//	Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  midi.begin();
	delay(1000);
  for (int i = 0; i < 8; i++)
  {
    int irq = (i %2) ? 2 : 1;

    for (int p = 0; p < 8; p++){
      IOBank[i].pinMode(pcfPorts[p], INPUT);
    }
    if (IOBank[i].begin()){
    } else {
//      Serial.println(" Failed");
    }
  }
}

bool keyChanged = false;
void loop()
{

  float pot1 = analogRead(A0) * (5.0 / 1023.0);
  float pot2 = analogRead(A1) * (5.0 / 1023.0);
  float pot3 = analogRead(A2) * (5.0 / 1023.0);
  
//  delay(5);    
	if (IRQ1Changed){
//      IRQ1Changed= false;
//      delay(5);
        for (int i = 0; i < manyPcf; i++){
          int check = (i %2) ? false : true;
          if (check) {
            PCF8574::DigitalInput  di = IOBank[i].digitalReadAll();
//            for (int p = 0; p < 8; p++){
//              currentport[i][p] = port[i][p];
//            }
            port[i][0] = !(di.p0);
            port[i][1] = !(di.p1);
            port[i][2] = !(di.p2);
            port[i][3] = !(di.p3);
            port[i][4] = !(di.p4);
            port[i][5] = !(di.p5);
            port[i][6] = !(di.p6);
            port[i][7] = !(di.p7);
          }
        }
        Change();
	}
  if (IRQ2Changed){
//      IRQ2Changed= false;
//      delay(5);
       for (int i = 0; i < manyPcf; i++){
          int check = (i %2) ? true : false;
          if (check) {
            PCF8574::DigitalInput  di = IOBank[i].digitalReadAll();
           
//            for (int p = 0; p < 8; p++){
//              currentport[i][p] = port[i][p];
//            }       
            port[i][0] = !(di.p0);
            port[i][1] = !(di.p1);
            port[i][2] = !(di.p2);
            port[i][3] = !(di.p3);
            port[i][4] = !(di.p4);
            port[i][5] = !(di.p5);
            port[i][6] = !(di.p6);
            port[i][7] = !(di.p7);
          }
        }
        Change();    
  }
}

void Change(){
  for (int i = 0; i < manyPcf; i++){
    for (int p = 0; p < 8; p++){
      if (currentport[i][p] != port[i][p]){
        int key = (i*8 + p + midishift);
        if (port[i][p]){
          midi.sendNoteOn(key, velocity);         // send a note on event
          currentport[i][p] = port[i][p];
        } else {
        midi.sendNoteOff(key, velocity);        // send a note off event
        currentport[i][p] = port[i][p];
        }
      }
    }
  }
}

void IRQ1(){
	// Interrupt called (No Serial no read no wire in this function, and DEBUG disabled on PCF library)
	 IRQ1Changed = true;
}
void IRQ2(){
  // Interrupt called (No Serial no read no wire in this function, and DEBUG disabled on PCF library)
   IRQ2Changed = true;
}
