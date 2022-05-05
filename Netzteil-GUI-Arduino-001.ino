/* ************************************************

    Display control for power supply unit.

    Version : 1.0.0
    Datum   : 2018-05-28
    Autor   : Carsten Nichte (master of desaster)
    Hardware: Maximilian Nichte

    Implements a main screen with three sub-screens
    with touch display support, and performs actions on the PSU hardware.

    class Max_Hardware
    struct Max_RGBFarbe
    class Max_Button
    x class Max_Textfeld
    x class Max_Schieberegler
    class Max_ButtonGroup
    class Max_Layout_Item
    class Max_LayoutManager
    class Max_Screen

    class Main_Screen
    class Fest_Screen
    class Signal_Screen
    class Setup_Screen

Creative-Commons License (CC BY-NC-SA 4.0)

Copyright (c) 2018 Carsten Nichte

## Licence

* Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
* https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
* https://creativecommons.org/licenses/by-nc-sa/4.0/?ref=chooser-v1

### Licence in short

* Do not sell the code.
* Learn, code, create, and have fun.

 ************************************************ */
#include <Arduino.h>
#include <UTFT.h>
#include <URTouch.h>
#include "QList.h"

// Adjust parameters according to the display/shield model used
UTFT    myGLCD(CTE70CPLD, 38, 39, 40, 41);
URTouch  myTouch( 6, 5, 4, 3, 2);

extern uint8_t SmallFont[];
extern uint8_t BigFont[];

extern uint8_t SevenSegNumFont[];
extern uint8_t SevenSegmentFull[];
extern uint8_t SixteenSegment128x192Num[];
extern uint8_t DotMatrix_M_Slash[];
extern uint8_t GroteskBold16x32[];

int x, y; // Speicher für die Touch Koordinaten
char currentScreen = '0'; // Der aktuelle Bildschirm (0,1,2,3)

int bl = 2; // Speicher für Backlight Brightness (test)


/* ************************************************
   An object for communication with the hardware...
   Currently at the bottom of the Perform functions.
 * *********************************************** */
class Max_Hardware {
  private:

  public:

    // ardu pinout
    int latchPin =  9;
    int clockPin = 11;
    int dataPin  = 12;
    int resetPin = 10;
    int floatPin =  8;

    int Buttonstate = 0;

    // default setup
    int mainData = 0b0000011001100110;

    // mask init
    //   int T_Latch   =  9;
    //   int T_Clock   = 11;
    //   int Ampli_CS  = 10;
    //   int MOSI_P    = 12;

    // int mask;
    // int maskClear  = 2;
    // int maskToogle = 3;
    // int maskSet    = 1;

    int switchData  [6];

    // Constructor
    Max_Hardware() {
      pinMode(floatPin, OUTPUT);
      digitalWrite(floatPin, HIGH);

      pinMode(latchPin, OUTPUT);
      pinMode(clockPin, OUTPUT);
      pinMode(dataPin,  OUTPUT);
      pinMode(resetPin, OUTPUT);

      digitalWrite(clockPin, LOW);
      //init mainLatch
      digitalWrite(latchPin, LOW);
      digitalWrite(resetPin, HIGH);
      digitalWrite(resetPin, LOW);
      digitalWrite(resetPin, HIGH);
      digitalWrite(latchPin, HIGH);
      digitalWrite(floatPin, LOW);

      // datasets of the T_latch
      switchData [0] = 0b00000000;
      switchData [1] = 0b10000000;
      switchData [2] = 0b11000000;
      switchData [3] = 0b00100000;
      switchData [4] = 0b00010000;
      switchData [5] = 0b00001000;
      switchData [6] = 0b00000100;

      int initData [5];
      initData[0] = 0b0000010001101111;
      initData[1] = 0b0000010001101001;
      initData[2] = 0b0000010001101111;
      initData[3] = 0b0000011001101111;
      initData[4] = 0b0000010001100110;

      for (int i = 0; i < 5; i++) {
        digitalWrite(latchPin, LOW);

        shiftOut(dataPin, clockPin, MSBFIRST, (initData[i]) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  initData[i]);

        digitalWrite(latchPin, HIGH);
        //  Serial.println("Latch HIGH");
      }
    }

    /*

    */
    void shiftOut_T(int state) {
      BinaryPrint("OUT_T a: ", mainData);
      // mask init
      int T_Latch   =  9;
      int T_Clock   = 11;
      int Ampli_CS  = 10;
      int MOSI_P    = 12;

      int mask;
      int maskClear  = 2;
      int maskToogle = 3;
      int maskSet    = 1;

      mask = maskInit(maskClear, T_Clock);
      mainData = mainData & mask;

      mask = maskInit(maskClear, T_Latch);
      mainData = mainData & mask;

      // getData -- Splits the selected dataset to be output at the T_Latch into individual bits.
      for (int i = 0; i < 8; i++) {
        int myBit;
        myBit = bitRead(switchData[state], i);

        // ImplementData --  Writes the extracted bit into the output value (digit 4) of the Main_latch.
        // which represents the data input of the T_latch
        bitWrite(mainData, 4, myBit);

        // T_latch create
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        // T_latch CLock High
        mask = maskInit(maskToogle, T_Clock);
        mainData = mainData ^ mask;



        // T_latch read in
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        mainData = mainData ^ mask;
      }

      // T_latch Output
      mask = maskInit(maskSet, T_Latch);
      mainData = mainData | mask;

      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
      shiftOut(dataPin, clockPin, MSBFIRST, mainData);
      digitalWrite(latchPin, HIGH);
      BinaryPrint("OUT_T e: ", mainData);
    }


    /*
       Currently only designed for one poti.
    */
    void shiftOut_P(int value) {
      int adress = 0x00;
      value = map (value, 0, 100, 0, 255);
      Serial.println(value);
      BinaryPrint("OUT_P a: ", mainData);
      // mask init
      int T_Latch   =  9;
      int T_Clock   = 11;
      int Ampli_CS  = 10;
      int MOSI_P    = 12;

      int mask;
      int maskClear  = 2;
      int maskToogle = 3;
      int maskSet    = 1;

      mask = maskInit(maskClear, T_Clock);
      mainData = mainData & mask;

      mask = maskInit(maskClear, Ampli_CS);
      mainData = mainData & mask;

      // getadress -- Splits the selected data set to be output at the potentiometer into individual bits.
      for (int i = 0; i < 8; i++) {
        int myBit;
        myBit = bitRead(adress, i);

        // ImplementData --  Writes the extracted bit into the output value (digit 12) of the main_latch.
        // welche den dateneingang des T_latch representiert
        bitWrite(mainData, 12, myBit);

        // T_latch create
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        // T_latch CLock High
        mask = maskInit(maskToogle, T_Clock);
        mainData = mainData ^ mask;

        // T_latch read in
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        mainData = mainData ^ mask;
      }

      // getData -- Splits the selected data set to be output at the potentiometer into individual bits.
      for (int i = 0; i < 8; i++) {
        int myBit;
        myBit = bitRead(value, i);

        // ImplementData --  Writes the extracted bit into the output value (digit 12) of the main_latch.
        // which represents the data input of the T_latch
        bitWrite(mainData, 12, myBit);

        // T_latch anlegen
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        // T_latch CLock High
        mask = maskInit(maskToogle, T_Clock);
        mainData = mainData ^ mask;

        // T_latch read in
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
        shiftOut(dataPin, clockPin, MSBFIRST,  mainData);
        digitalWrite(latchPin, HIGH);

        mainData = mainData ^ mask;
      }

      // T_latch Output
      mask = maskInit(maskSet, Ampli_CS);
      mainData = mainData | mask;

      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, (mainData) >> 8);
      shiftOut(dataPin, clockPin, MSBFIRST, mainData);
      digitalWrite(latchPin, HIGH);
      BinaryPrint("OUT_P e: ", mainData);
    }

    /*

    */
    int maskInit(int process, int maskBit) {

      int Set    = 0b0000000000000000;
      int Clear  = 0b1111111111111111;
      int Toogle = 0b0000000000000000;

      switch (process) {
        case 1: bitWrite(Set, maskBit, 1); return Set;
        case 2: bitWrite(Clear, maskBit, 0); return Clear;
        case 3: bitWrite(Toogle, maskBit, 1); return Toogle;
        default: break;
      }
      return;
    }

    // Currentsensor at Pin 5...
    int getCurrentSensor(int sensorPin) {
      int sensorValue = 0;
      sensorValue = analogRead(sensorPin);
      sensorValue =  map(sensorValue, 0, 255, -500, 500);
      return sensorValue;

    }

    int getSensor(int sensorPin) {
      float sensorValue = 0;
      sensorValue = analogRead(sensorPin);
      return sensorValue;
    }

    // Binary output for evaluation
    void BinaryPrint(String label, int nummer) {

      Serial.print(label);

      for (int i = 15; i > -1; i--) {
        int Bit;
        Bit = bitRead(nummer, i);
        Serial.print(Bit);
        if (i == 8) {
          Serial.print(" ");
        }
      }

      Serial.println("");

    }

}; // Max_Hardware



/*
   Auxiliary construction to save RGB colour values
*/
struct Max_RGBFarbe {
  int R;
  int G;
  int B;
};

struct Max_Ausgang {
  Max_RGBFarbe farbe;
  int x = 0;
  int y = 0;
};

/* ************************************************
   One button as an object for all buttons.
   Works as a button or switch.

   An object consists of properties and methods (functions).

   Properties:
      name
      x1, y1, x2, y2
      fontkorreturX, fontkorreturY
      buttonFarbe, buttonFarbeFrame, buttonFarbePressed
   Methods:
      drawButton()
      drawButtonPressed()
      bool isPressed(int x, int y)
 * ************************************************ */
class Max_Button {
  public:
    // Internal coordinates:
    int x1 = 0; // Upper left corner in pixels
    int y1 = 0;
    int x2 = 0; // lower right corner in pixels
    int y2 = 0;

    String name = "Button-Name";

    int mode = 0; // 0:Push-button, 1: Switch, 2: Controller, 3: Display text, 4: Display outputs

    // Position and size in pixels.
    int button_x = 10;
    int button_y = 10;
    int button_width = 100;
    int button_height = 100;

    int fontkorreturX = 0;
    int fontkorreturY = 0;

    // Value of the switch (mode 1)
    bool stateSchalter = false; // Switch on or off.

    //  Parameter of the controller(mode 2)
    float regler_min = 0.0;
    float regler_max = 100.0;
    float regler_value = 0.0;

    int touch_x_last = button_x;
    int touch_x = button_x; // the current touch coordinates
    int touch_y = button_x;

    // Status display outputs
    int anzeige_ausg = 0; // 1, 2, 3, 4

    Max_Ausgang anzeige_ausg1;
    Max_Ausgang anzeige_ausg2;
    Max_Ausgang anzeige_ausg3;
    Max_Ausgang anzeige_ausg4;

    // Struct for RGB colour values
    Max_RGBFarbe buttonFarbe;
    Max_RGBFarbe buttonFarbeFrame;
    Max_RGBFarbe buttonFarbePressed;
    Max_RGBFarbe buttonFarbeText;

    Max_RGBFarbe buttonFarbeRegler;
    /*
        Das ist der Konstruktor des Objekts.
    */
    Max_Button() {
      // Structs have to be initialised in the Constructor.
      buttonFarbeRegler.R = 150;
      buttonFarbeRegler.G = 150;
      buttonFarbeRegler.B = 238;

      buttonFarbe.R = 0; // Blue
      buttonFarbe.G = 0;
      buttonFarbe.B = 238;


      buttonFarbeFrame.R = 255; // White
      buttonFarbeFrame.G = 255;
      buttonFarbeFrame.B = 255;

      buttonFarbeText.R = 255; // White
      buttonFarbeText.G = 255;
      buttonFarbeText.B = 255;

      buttonFarbePressed.R = 255; // Red
      buttonFarbePressed.G = 0;
      buttonFarbePressed.B = 0;

      anzeige_ausg1.x = 65;
      anzeige_ausg1.y = 65;
      anzeige_ausg1.farbe.R = 255;
      anzeige_ausg1.farbe.G = 0;
      anzeige_ausg1.farbe.B = 0;

      anzeige_ausg2.x = 65;
      anzeige_ausg2.y = 65 + 110;
      anzeige_ausg2.farbe.R = 255;
      anzeige_ausg2.farbe.G = 0;
      anzeige_ausg2.farbe.B = 0;

      anzeige_ausg3.x = 65;
      anzeige_ausg3.y = 65 + 2 * 110;
      anzeige_ausg3.farbe.R = 255; // Since it is black like the background, we draw white circles here.
      anzeige_ausg3.farbe.G = 255;
      anzeige_ausg3.farbe.B = 255;

      anzeige_ausg4.x = 65;
      anzeige_ausg4.y = 65 + 3 * 110;
      anzeige_ausg4.farbe.R = 0;
      anzeige_ausg4.farbe.G = 0;
      anzeige_ausg4.farbe.B = 255;

    }

    /*
       This method checks whether the button has been pressed.
       Receives the x,y coordinates of the input device (mouse or touchpad).
       It is checked whether the coordinates lie within the button,
       If the button was pressed, it is drawn. 
       
    */
    bool isPressed(int x, int y) {
      bool result = false;

      this->touch_x = x;
      this->touch_y = y;

      this->x1 = this->button_x;
      this->y1 = this->button_y;

      x2 = button_x + button_width;
      y2 = button_y + button_height;

      result = (x >= this->x1) && (x <= this->x2) && (y >= this->y1) && (y <= this->y2);

      if(result){
          drawButtonPressed();   
      }

      return result;
    }

    /*
        Method for drawing the (unpressed) button.
    */
    void drawButton() {

      this->x1 = this->button_x;
      this->y1 = this->button_y;

      x2 = button_x + button_width;
      y2 = button_y + button_height;

      // 0:Push-button, 1: Switch, 2: Controller, 3: Display
      if (mode == 2) {

        paintRegler();

      } else if (mode == 3) {

        paintAnzeige(); // Text

      } else if (mode == 4) {

        paintAnzeigeAusgang();

      } else {

        paintButton();

      }

    }

    /*
        Draws the button when it is pressed.
    */
    void drawButtonPressed() {
      this->x1 = this->button_x;
      this->y1 = this->button_y;

      x2 = button_x + button_width;
      y2 = button_y + button_height;

      // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      if (mode == 2) {

        paintReglerPressed();

      } else if (mode == 3) {

        // do nothing

      } else if (mode == 1) {

        // Button pressed...
        paintButtonPressed();

        if (stateSchalter) {

          paintButton();
          stateSchalter = false;

        } else {

          stateSchalter = true;

        }

      } else if (mode == 0) {

        // Button pressed...
        paintButtonPressed();

        // As long as the keyboard is pressed, it hangs in the loop and waits.
        // It reads the values at each loop pass and does not use them any further.
        // It only continues when the key is released and myTouch.dataAvailable() returns FALSE.
        while (myTouch.dataAvailable()) {
          myTouch.read();
        }

        paintButton();

      }
    }



    void paintButton() {
      // Filled rectangle with rounded corners
      myGLCD.setColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B);
      myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Frame without infill
      myGLCD.setColor(buttonFarbeFrame.R, buttonFarbeFrame.G, buttonFarbeFrame.B);
      myGLCD.drawRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Text - The text is output centred in the button
      myGLCD.setColor(buttonFarbeText.R, buttonFarbeText.G, buttonFarbeText.B);
      myGLCD.setFont(BigFont); // SevenSegNumFont, BigFont
      myGLCD.setBackColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B); // Font background colour
      myGLCD.print(this->name, this->x1 + (this->x2 - this->x1) / 2 - fontkorreturX, this->y1 + (this->y2 - this->y1) / 2 - fontkorreturY);
    }

    void paintButtonPressed() {
      // Filled rectangle with rounded corners
      myGLCD.setColor(buttonFarbePressed.R, buttonFarbePressed.G, buttonFarbePressed.B);
      myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Frame without infill
      myGLCD.setColor(buttonFarbeFrame.R, buttonFarbeFrame.G, buttonFarbeFrame.B);
      myGLCD.drawRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Text - The text is output centred in the button
      myGLCD.setFont(BigFont);
      myGLCD.setColor(buttonFarbeText.R, buttonFarbeText.G, buttonFarbeText.B);
      myGLCD.setBackColor(buttonFarbePressed.R, buttonFarbePressed.G, buttonFarbePressed.B); // Font background colour
      myGLCD.print(this->name, this->x1 + (this->x2 - this->x1) / 2 - fontkorreturX, this->y1 + (this->y2 - this->y1) / 2 - fontkorreturY);
    }


    void paintAnzeige() {
      buttonFarbeFrame.R = buttonFarbe.R;
      buttonFarbeFrame.G = buttonFarbe.G;
      buttonFarbeFrame.B = buttonFarbe.B;

      // Filled rectangle with rounded corners
      myGLCD.setColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B);
      myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Frame without infill
      myGLCD.setColor(buttonFarbeFrame.R, buttonFarbeFrame.G, buttonFarbeFrame.B);
      myGLCD.drawRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Text - The text is output centred in the button
      myGLCD.setColor(buttonFarbeText.R, buttonFarbeText.G, buttonFarbeText.B);
      myGLCD.setFont(DotMatrix_M_Slash); // SevenSegmentFull, SevenSegNumFont, SixteenSegment128x192Num, DotMatrix_M_Slash, GroteskBold16x32
      myGLCD.setBackColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B); // Hintergrundfarbe des Fonts
      myGLCD.print(this->name, this->x1 + (this->x2 - this->x1) / 2 - fontkorreturX, this->y1 + (this->y2 - this->y1) / 2 - fontkorreturY);
    }

    void paintAnzeigeAusgang() {

      // Filled rectangle with rounded corners
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Paint circles
      if (anzeige_ausg == 1) {
        // 2
        myGLCD.setColor(anzeige_ausg2.farbe.R, anzeige_ausg2.farbe.G, anzeige_ausg2.farbe.B);
        myGLCD.fillCircle(this->x1 + anzeige_ausg2.x, this->y1 + anzeige_ausg2.y, 25 );
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillCircle(this->x1 + anzeige_ausg2.x, this->y1 + anzeige_ausg2.y, 13 );
        // 3
        myGLCD.setColor(anzeige_ausg3.farbe.R, anzeige_ausg3.farbe.G, anzeige_ausg3.farbe.B);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 25 );
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 13 );

      } else if (anzeige_ausg == 2) {
        // 3
        myGLCD.setColor(anzeige_ausg3.farbe.R, anzeige_ausg3.farbe.G, anzeige_ausg3.farbe.B);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 25 );
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 13 );
        // 4
        myGLCD.setColor(anzeige_ausg4.farbe.R, anzeige_ausg4.farbe.G, anzeige_ausg4.farbe.B);
        myGLCD.fillCircle(this->x1 + anzeige_ausg4.x, this->y1 + anzeige_ausg4.y, 25 );
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillCircle(this->x1 + anzeige_ausg4.x, this->y1 + anzeige_ausg4.y, 13 );

      } else if (anzeige_ausg == 3) {
        // 1
        myGLCD.setColor(anzeige_ausg1.farbe.R, anzeige_ausg1.farbe.G, anzeige_ausg1.farbe.B);
        myGLCD.fillCircle(this->x1 + anzeige_ausg1.x, this->y1 + anzeige_ausg1.y, 25 );
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillCircle(this->x1 + anzeige_ausg1.x, this->y1 + anzeige_ausg1.y, 13 );

        // 3
        myGLCD.setColor(anzeige_ausg3.farbe.R, anzeige_ausg3.farbe.G, anzeige_ausg3.farbe.B);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 25 );
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 13 );

      } else if (anzeige_ausg == 4) {

        // 2
        myGLCD.setColor(anzeige_ausg2.farbe.R, anzeige_ausg2.farbe.G, anzeige_ausg2.farbe.B);
        myGLCD.fillCircle(this->x1 + anzeige_ausg2.x, this->y1 + anzeige_ausg2.y, 25 );
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillCircle(this->x1 + anzeige_ausg2.x, this->y1 + anzeige_ausg2.y, 13 );
        // 3
        myGLCD.setColor(anzeige_ausg3.farbe.R, anzeige_ausg3.farbe.G, anzeige_ausg3.farbe.B);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 25 );
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawCircle(this->x1 + anzeige_ausg3.x, this->y1 + anzeige_ausg3.y, 13 );
        // 4
        myGLCD.setColor(anzeige_ausg4.farbe.R, anzeige_ausg4.farbe.G, anzeige_ausg4.farbe.B);
        myGLCD.fillCircle(this->x1 + anzeige_ausg4.x, this->y1 + anzeige_ausg4.y, 25 );
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillCircle(this->x1 + anzeige_ausg4.x, this->y1 + anzeige_ausg4.y, 13 );

      } else { // 0

      }

    }

    void paintRegler() {

      // Button
      myGLCD.setColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B);
      myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

      // Frame
      myGLCD.setColor(buttonFarbeFrame.R, buttonFarbeFrame.G, buttonFarbeFrame.B);
      myGLCD.drawRoundRect (this->x1, this->y1, this->x2, this->y2);

      // filled rectangle for the stand


      // -------------------------------
      // The controller
      int sx1 = button_x;
      int sy1 = button_y;
      int sx2 = button_x + button_width * regler_value / regler_max; // or simply x_touch
      int sy2 = button_y + button_height;

      myGLCD.setColor(buttonFarbeRegler.R, buttonFarbeRegler.G, buttonFarbeRegler.B);
      myGLCD.fillRoundRect (sx1, sy1, sx2, sy2);
      // -------------------------------

      // Text - The text is output centred in the button
      myGLCD.setColor(buttonFarbeText.R, buttonFarbeText.G, buttonFarbeText.B);
      myGLCD.setFont(BigFont); // SevenSegNumFont, BigFont
      myGLCD.setBackColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B); // Font background colour
      myGLCD.print(this->name, this->x1 + (this->x2 - this->x1) / 2 - fontkorreturX, this->y1 + (this->y2 - this->y1) / 2 - fontkorreturY);
    }


    void paintReglerPressed() {

      if (touch_x !=  touch_x_last) {
        touch_x_last = touch_x;

        // Button
        myGLCD.setColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B);
        myGLCD.fillRoundRect (this->x1, this->y1, this->x2, this->y2);

        // Frame
        myGLCD.setColor(buttonFarbeFrame.R, buttonFarbeFrame.G, buttonFarbeFrame.B);
        myGLCD.drawRoundRect (this->x1, this->y1, this->x2, this->y2);

        // -------------------------------
        // The controller

        int sx1 = button_x;
        int sy1 = button_y;
        int sx2 = touch_x;
        int sy2 = button_y + button_height;

        regler_value = 100.00 * ((float)touch_x - (float)button_x) / (float)button_width;

        Serial.print(regler_value);
        Serial.print(" = 100* (");
        Serial.print(touch_x);
        Serial.print("-");
        Serial.print(button_x);
        Serial.print(" ) /");
        Serial.println(button_width);

        myGLCD.setColor(buttonFarbeRegler.R, buttonFarbeRegler.G, buttonFarbeRegler.B);
        myGLCD.fillRoundRect (sx1, sy1, sx2, sy2);

        // -------------------------------

        // Text - The text is output centred in the button
        myGLCD.setFont(BigFont);
        myGLCD.setColor(buttonFarbeText.R, buttonFarbeText.G, buttonFarbeText.B);
        myGLCD.setBackColor(buttonFarbe.R, buttonFarbe.G, buttonFarbe.B); // Hintergrundfarbe des Fonts
        myGLCD.print(this->name, this->x1 + (this->x2 - this->x1) / 2 - fontkorreturX, this->y1 + (this->y2 - this->y1) / 2 - fontkorreturY);

      }
    }

}; // End Button Object



/* ************************************************
   A text field as an object.

   Is a button that is disabled and has a different colour.

 * ************************************************ */
class Max_Textfeld : public Max_Button {
  public:
    // name = "Names des Textfeldes";
    // enabled = false;

    // Kontruktor
    Max_Textfeld::Max_Textfeld() : Max_Button() {
      /*
        buttonFarbe.R = 125; // Grau
        buttonFarbe.G = 125;
        buttonFarbe.B = 125;

        buttonFarbeFrame.R = 125; // Weiß
        buttonFarbeFrame.G = 125;
        buttonFarbeFrame.B = 125;
      */
    }

    // Methoden...
};

/* ************************************************
   A slider as an object.
   Since we need several sliders, this also makes sense...

   An object consists of properties and methods (functions).

   Properties:

   Methods:

 * ************************************************ */
class Max_Schieberegler {
  public:
    String name = "Names des Reglers";

    // Constructor
    Max_Max_Schieberegler() {

    }

    // Methoden...
};


/*
   A group of switches
   One may only be active.
*/
class Max_ButtonGroup {
  public:
    QList<Max_Button*> *buttons = new QList<Max_Button*>();

    int mode = 0; // 0: one out of all

    Max_ButtonGroup() { }

    /*

    */
    void addButton(Max_Button *btn) {
      buttons->push_back(btn);
    }

    /*
       All except the transferred one are switched off
    */
    void selectedButton(Max_Button *btn) {

      for (int i = 0; i < buttons->size(); i++) {
        if (buttons->at(i) != btn) {
          if (buttons->at(i)->mode == 1) {

          }

          // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
          buttons->at(i)->drawButton();
          buttons->at(i)->stateSchalter = false;

        }
      }
    }
};

/*
   Saves a button, or object to be displayed, and where it is to be drawn in the grid.
   and other properties. Auxiliary class of Max_LayoutManager.
*/
class Max_Layout_Item {
  public:
    // Position in the grid
    int col = 2;
    int row = 1;

    // Overspan x Raster
    int span_x = 3;
    int span_y = 1;

    // Fill out the grid
    boolean fill_x = true;
    boolean fill_y = true;

    int margin_left = 10;
    int margin_right = 10;
    int margin_top = 10;
    int margin_bottom = 10;

    Max_Button *btn;

    Max_Layout_Item() { }
};

/*
   A grid layout manager.
   Holds a list of buttons, and the definition of the grid.
   Calculates the pixel position for each button from the respective grid coordinates.
*/
class Max_LayoutManager {
  public:

    QList<Max_Layout_Item> layout_Items;
    // layout_Items.clear(); // Clear all items in table

    // Screen dimensions
    int width = myGLCD.getDisplayXSize();
    int height = myGLCD.getDisplayYSize();

    // Grid with this number of columns and rows
    int rasterRows = 3;
    int rasterCols = 5;

    int margin_left = 10;
    int margin_right = 10;
    int margin_top = 50;
    int margin_bottom = 60;

    // Constructor
    Max_LayoutManager() {

    }

    void addButton(Max_Button *btn, int posx, int posy) {
      Max_Layout_Item li;
      li.btn = btn;
      li.col = posx;
      li.row = posy;
      layout_Items.push_back(li);
    }

    void addButton(Max_Button *btn, int posx, int posy, int span_x, int span_y) {
      Max_Layout_Item li;
      li.btn = btn;
      li.col = posx;
      li.row = posy;
      li.span_x = span_x;
      li.span_y = span_y;
      layout_Items.push_back(li);
    }

    void rasterize() {
      //      Serial.println("----- rasterize start");
      //      Serial.print("raster-x: "); Serial.print(rasterCols);
      //      Serial.print(", raster-y: "); Serial.println(rasterRows);
      //      Serial.print("screen-x: "); Serial.print(width);
      //      Serial.print(", screen-y: "); Serial.println(height);

      // Für alle Buttons in der Liste
      for (int i = 0; i < layout_Items.size(); i++) {
        //        Serial.print("xv: ");
        //        Serial.print(layout_Items.at(i).btn->button_x);
        //        Serial.print(", yv: ");
        //        Serial.print(layout_Items.at(i).btn->button_y);
        //        Serial.println("");



        // Where should the button appear in the grid?
        // Align the button with the grid
        layout_Items.at(i).btn->button_x = margin_left + layout_Items.at(i).margin_left + ( (width - margin_left - margin_right)  / rasterCols) * layout_Items.at(i).col;
        layout_Items.at(i).btn->button_y = margin_top + layout_Items.at(i).margin_top + ( (height - margin_top - margin_bottom) / rasterRows) * layout_Items.at(i).row;

        if (layout_Items.at(i).fill_x) {
          // Adjust the width of the button...
          layout_Items.at(i).btn->button_width = ((width - margin_left - margin_right) / rasterCols) * layout_Items.at(i).span_x - layout_Items.at(i).margin_right;
        }

        if (layout_Items.at(i).fill_y) {
          // Calculate the height of the button
          layout_Items.at(i).btn->button_height = ( (height - margin_top - margin_bottom) / rasterRows) * layout_Items.at(i).span_y - layout_Items.at(i).margin_bottom;
        }

        //        Serial.print("xn: ");
        //        Serial.print(layout_Items.at(i).btn->button_x); Serial.print(" = "); Serial.print("("); Serial.print(width); Serial.print("/"); Serial.print(rasterCols); Serial.print(")"); Serial.print("*"); Serial.println(layout_Items.at(i).col);
        //        Serial.print("yn: ");
        //        Serial.print(layout_Items.at(i).btn->button_y); Serial.print(" = "); Serial.print("("); Serial.print(height); Serial.print("/"); Serial.print(rasterRows); Serial.print(")"); Serial.print("*"); Serial.println(layout_Items.at(i).row);

        // Weitere Layout-Eigenschaften verarbeiten.
        // ...
        //        Serial.println("-----");

      }
      //      Serial.println("----- rasterize end");
    }
};

/* ************************************************

   A Screen-Object (base class)

   A Screen-Object contains buttons and other
   display & control elements
   and represents a view.

 * ************************************************ */
class Max_Screen {
  public:
    String name = "Names des Bildschirms";
    String info = "Created by Maximilian Nichte @ HET 13";

    int headerHeight = 32;
    int footerHeight = 32;

    Max_RGBFarbe backgroundFarbe;
    Max_RGBFarbe textFarbe;
    Max_RGBFarbe headerLinieFarbe;

    // Screen dimensions
    int width = myGLCD.getDisplayXSize();
    int height = myGLCD.getDisplayYSize();

    Max_LayoutManager layoutManager;
    Max_ButtonGroup buttonGroup;

    // other screen properties...
    // ...

    // Constructor
    Max_Screen() {
      backgroundFarbe.R = 0;
      backgroundFarbe.G = 0;
      backgroundFarbe.B = 0;

      textFarbe.R = 255;
      textFarbe.G = 255;
      textFarbe.B = 255;

      headerLinieFarbe.R = 255;
      headerLinieFarbe.G = 0;
      headerLinieFarbe.B = 0;
    }

    void setBackgroundColor(int r, int g, int b) {
      backgroundFarbe.R = r;
      backgroundFarbe.G = g;
      backgroundFarbe.B = b;
    }

    /*
       Draws the screen:
       Header, all elements and the footer.
    */
    void drawScreen() {
      myGLCD.clrScr();
      // myGLCD.fillScr(backgroundFarbe.R, backgroundFarbe.G, backgroundFarbe.B);

      // Renders all buttons on the screen
      for (int i = 0; i < layoutManager.layout_Items.size(); i++) {
        layoutManager.layout_Items.at(i).btn->drawButton();
      }

      // Background colour text
      myGLCD.setBackColor(backgroundFarbe.R, backgroundFarbe.G, backgroundFarbe.B);

      // Header Headline
      myGLCD.setColor(255, 255, 255); // Colour font white
      myGLCD.setFont(BigFont);
      myGLCD.print(name, CENTER, headerHeight - 27);

      // Footer
      myGLCD.setFont(BigFont);
      myGLCD.print(info, CENTER, height - footerHeight + 14);

      // Draw lines
      myGLCD.setColor(255, 0, 0); // Colour red
      myGLCD.drawLine(0, headerHeight, width, headerHeight);
      myGLCD.drawLine(0, height - footerHeight, width, height - footerHeight);
    }
};

/*
   The main screen
*/
class Main_Screen : public Max_Screen {
  public:
    // The buttons on the screen
    Max_Button btn_fest;
    Max_Button btn_signal;
    Max_Button btn_setup;
    Max_Button btn_hv;

    // Constructor
    Main_Screen() {
      setBackgroundColor(0, 0, 0);
      this->name = "Digitales Netzteil 1.0.0";

      btn_fest.name = "Festspannung";
      btn_fest.fontkorreturX = 100;
      btn_fest.fontkorreturY = 8;

      btn_signal.name = "Signalgenerator";
      btn_signal.fontkorreturX = 100;
      btn_signal.fontkorreturY = 8;

      btn_setup.name = "Einstellungen";
      btn_setup.fontkorreturX = 90;
      btn_setup.fontkorreturY = 8;

      btn_hv.name = "HV";
      btn_hv.fontkorreturX = 20;
      btn_hv.fontkorreturY = 8;

      // The layout manager determines the positions of the buttons on the screen
      // using a grid...
      layoutManager.rasterRows = 3; // Raster
      layoutManager.rasterCols = 5;

      // The buttons and their positions in the grid
      // Parameter: btn, x, y, (span_x, span_y)
      layoutManager.addButton( &btn_fest  , 1, 0, 2, 1);
      layoutManager.addButton( &btn_signal, 1, 1, 3, 1);
      layoutManager.addButton( &btn_setup , 1, 2, 3, 1);
      layoutManager.addButton( &btn_hv    , 3, 0, 1, 1);

      // Calculate the actual screen coordinates of all buttons using the grid.
      // and assign them to the buttons so that they later appear at the desired positions.
      layoutManager.rasterize();
    }
};

/*
   Screen 'fixed voltage'
*/
class Fest_Screen : public Max_Screen {
  public:
    // The buttons on the screen
    Max_Button btn_back;
    Max_Button btn_fest_5;
    Max_Button btn_fest_12;
    Max_Button btn_fest_m12;

    Max_Button txt_anzeige_5I;
    Max_Button txt_anzeige_12I;
    Max_Button txt_anzeige_m12I;

    Max_Button anz_ausgang;

    // Constructor
    Fest_Screen() {
      setBackgroundColor(0, 0, 0);

      layoutManager.margin_left = 0;
      layoutManager.margin_right = 0;
      layoutManager.margin_top = 0;
      layoutManager.margin_bottom = 0;

      this->name = "Festspannung";

      btn_back.name = "Close";
      btn_back.fontkorreturX = 40;
      btn_back.fontkorreturY = 8;

      btn_fest_5.name = "5V";
      btn_fest_5.mode = 1; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_fest_5.fontkorreturX = 20;
      btn_fest_5.fontkorreturY = 8;

      btn_fest_12.name = "12V";
      btn_fest_12.mode = 1; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_fest_12.fontkorreturX = 22;
      btn_fest_12.fontkorreturY = 8;

      btn_fest_m12.name = "-12V";
      btn_fest_m12.mode = 1; // // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_fest_m12.fontkorreturX = 30;
      btn_fest_m12.fontkorreturY = 8;


      txt_anzeige_5I.name = "000 mA";
      txt_anzeige_5I.mode = 3; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      txt_anzeige_5I.fontkorreturX = 50;
      txt_anzeige_5I.fontkorreturY = 10;

      txt_anzeige_12I.name = "000 mA";
      txt_anzeige_12I.mode = 3; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      txt_anzeige_12I.fontkorreturX = 50;
      txt_anzeige_12I.fontkorreturY = 10;

      txt_anzeige_m12I.name = "000 mA";
      txt_anzeige_m12I.mode = 3; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      txt_anzeige_m12I.fontkorreturX = 50;
      txt_anzeige_m12I.fontkorreturY = 10;

      anz_ausgang.name = ""; // active outputs
      anz_ausgang.mode = 4; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige, 4:Anzeige Ausgänge

      buttonGroup.addButton(&btn_fest_5);
      buttonGroup.addButton(&btn_fest_12);
      buttonGroup.addButton(&btn_fest_m12);

      layoutManager.rasterRows = 5; // Raster
      layoutManager.rasterCols = 6;

      // The buttons and their positions in the grid
      // Parameter: btn, x, y, span_x, span_y
      layoutManager.addButton( &btn_back    , 1, 1, 1, 3);

      layoutManager.addButton( &btn_fest_5  , 2, 1, 1, 1);
      layoutManager.addButton( &btn_fest_12 , 2, 2, 1, 1);
      layoutManager.addButton( &btn_fest_m12, 2, 3, 1, 1);

      layoutManager.addButton( &txt_anzeige_5I, 3, 1, 2, 1);
      layoutManager.addButton( &txt_anzeige_12I, 3, 2, 2, 1);
      layoutManager.addButton( &txt_anzeige_m12I, 3, 3, 2, 1);

      layoutManager.addButton(&anz_ausgang , 5, 0, 1, 5);

      // Calculate the actual screen coordinates of all buttons using the grid.
      // and assign them to the buttons so that they later appear at the desired positions.
      layoutManager.rasterize();
    }

};

/*
   Screen 'Signal'
*/
class Signal_Screen : public Max_Screen {
  public:
    // The buttons on the screen
    Max_Button btn_back;
    Max_Button btn_regler_amplitude;
    Max_Button btn_regler_frequenz;

    Max_Button btn_gleich;
    Max_Button btn_sinus;
    Max_Button txt_strom;

    Max_Button anz_ausgang;

    // Constructor
    Signal_Screen() {
      setBackgroundColor(0, 0, 0);
      this->name = "Signalgenerator";

      layoutManager.margin_left = 0;
      layoutManager.margin_right = 0;
      layoutManager.margin_top = 0;
      layoutManager.margin_bottom = 0;

      btn_back.name = "Close";
      btn_back.fontkorreturX = 40;
      btn_back.fontkorreturY = 8;

      btn_regler_amplitude.name = "Amplitude";
      btn_regler_amplitude.mode = 2; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_regler_amplitude.fontkorreturX = 60;
      btn_regler_amplitude.fontkorreturY = 8;

      btn_regler_frequenz.name = "Frequenz";
      btn_regler_frequenz.mode = 2; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_regler_frequenz.fontkorreturX = 60;
      btn_regler_frequenz.fontkorreturY = 8;

      btn_gleich.name = "=";
      btn_gleich.mode = 1; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_gleich.fontkorreturX = 5;
      btn_gleich.fontkorreturY = 8;

      btn_sinus.name = "sin";
      btn_sinus.mode = 1; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      btn_sinus.fontkorreturX = 20;
      btn_sinus.fontkorreturY = 8;

      txt_strom.name = "000mA";
      txt_strom.mode = 3; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      txt_strom.fontkorreturX = 40;
      txt_strom.fontkorreturY = 8;

      anz_ausgang.name = ""; // active outputs
      anz_ausgang.mode = 4; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige, 4:Anzeige Ausgänge

      // These buttons are locked against each other
      buttonGroup.addButton(&btn_gleich);
      buttonGroup.addButton(&btn_sinus);

      layoutManager.rasterRows = 5; // Raster
      layoutManager.rasterCols = 6;

      // The buttons and their positions in the grid
      // Parameter: btn, x, y, span_x, span_y
      layoutManager.addButton(&btn_back  , 1, 1, 1, 3);

      layoutManager.addButton(&btn_gleich, 2, 1, 1, 1);
      layoutManager.addButton(&btn_sinus , 3, 1, 1, 1);
      layoutManager.addButton(&txt_strom , 4, 1, 1, 1);

      layoutManager.addButton(&btn_regler_amplitude, 2, 2, 3, 1);
      layoutManager.addButton(&btn_regler_frequenz , 2, 3, 3, 1);

      layoutManager.addButton(&anz_ausgang , 5, 0, 1, 5);

      // Calculate the actual screen coordinates of all buttons using the grid.
      // and assign them to the buttons so that they later appear at the desired positions.
      layoutManager.rasterize();
    }

};

class Setup_Screen : public Max_Screen {
  public:
    // The buttons on the screen
    Max_Button btn_back;
    Max_Button txt_anzeige;

    // Constructor
    Setup_Screen() {
      setBackgroundColor(0, 0, 0);
      this->name = "Einstellungen";

      btn_back.name = "Close";
      btn_back.fontkorreturX = 40;
      btn_back.fontkorreturY = 8;

      txt_anzeige.name = "+012";
      txt_anzeige.mode = 3; // 0:Taster, 1: Schalter, 2: Regler, 3: Anzeige
      txt_anzeige.fontkorreturX = 70;
      txt_anzeige.fontkorreturY = 25;

      txt_anzeige.buttonFarbeFrame.R = txt_anzeige.buttonFarbe.R;
      txt_anzeige.buttonFarbeFrame.G = txt_anzeige.buttonFarbe.G;
      txt_anzeige.buttonFarbeFrame.B = txt_anzeige.buttonFarbe.B;

      layoutManager.rasterRows = 5; // Raster
      layoutManager.rasterCols = 6;

      // The buttons and their positions in the grid
      // Parameter: btn, x, y, span_x, span_y
      layoutManager.addButton( &btn_back, 1, 1, 1, 3);
      layoutManager.addButton( &txt_anzeige , 2, 1, 1, 1);

      // Calculate the actual screen coordinates of all buttons using the grid.
      // and assign them to the buttons so that they later appear at the desired positions.
      layoutManager.rasterize();
    }

};

//-----------------------
// VARIABLEN


// All screens
Main_Screen main_screen;
Fest_Screen fest_screen;
Signal_Screen signal_screen;
Setup_Screen setup_screen;

Max_Hardware max_hardware;



/* ************************************************
   Arduino Setup
 * ************************************************ */
void setup() {

  // For possible output of debug info with ...
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //...

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_EXTREME); // PREC_MEDIUM, PREC_HI, PREC_EXTREME
  myGLCD.setBrightness(bl);

  currentScreen = '0';
  main_screen.drawScreen();

} // Arduino setup() ende



/* ************************************************
   Arduino Loop
 * ************************************************ */
void loop() {

  // -------------------------------------------------------
  // 1.) If we are on the main page (level 1)...
  // We check whether a button has been pressed there...
  // -------------------------------------------------------
  if (currentScreen == '0') {

    // Is someone pressing around on the touch display?
    if (myTouch.dataAvailable()) {

      // Determine touch coordinates
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      // Was HV elected?
      if (main_screen.btn_hv.isPressed(x, y)) {
        currentScreen = '0';
        main_screen.btn_hv.drawButtonPressed();
        // hv_screen.drawScreen();
      }

      // Has a fixed voltage regulator been selected?
      if (main_screen.btn_fest.isPressed(x, y)) {
        currentScreen = '1';
        main_screen.btn_fest.drawButtonPressed();
        fest_screen.drawScreen();

      }

      // Was signal generator selected?
      if (main_screen.btn_signal.isPressed(x, y)) {
        currentScreen = '2';
        main_screen.btn_signal.drawButtonPressed();
        signal_screen.drawScreen();

      }

      // Was Setup chosen?
      if (main_screen.btn_setup.isPressed(x, y)) {
        currentScreen = '3';
        main_screen.btn_setup.drawButtonPressed();
        setup_screen.drawScreen();
      }

    } // if touch availible

  } // if currentScreen==0

  // -------------------------------------------------------
  // 2.) Depending on the current screen, execute functions on level 2...
  // Here, the hardware is controlled (via buttons and sliders).
  // but also read out hardware and display values.
  // The perform functions are at the very end of this file.
  // -------------------------------------------------------

  if (currentScreen == '3') {
    perform_Setup();
  }

  if (currentScreen == '2') {
    performn_Signal();
  }

  if (currentScreen == '1') {
    perform_Fest();
  }

} // Arduino loop() ende


/* ************************************************
   Functions that access hardware.
   These are later built into objects as methods.

   Here, the buttons or sliders that are pressed are determined
   and the hardware is controlled accordingly,
   but also read out hardware and display values.

 * ************************************************ */


int counter = -999;

/*
   Perform functions on the setup screen.
*/
void perform_Setup() {

  // Evaluate more buttons
  setup_screen.txt_anzeige.name = String(counter) + " mA";
  setup_screen.txt_anzeige.drawButton();

  if (counter < 1000) {
    counter = counter + 1;
  } else {
    counter = 0;
  }

  if (myTouch.dataAvailable()) {

    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();

    if (setup_screen.btn_back.isPressed(x, y)) {
      setup_screen.btn_back.drawButtonPressed();

      // Reset hardware.
      max_hardware.shiftOut_T(0);

      currentScreen = '0';
      main_screen.drawScreen();
    }
  }

}


/*
   Funktionen auf dem Signal-Screen ausführen.
*/
void performn_Signal() {

  // ...
  // touching on the Touchscreen
  if (myTouch.dataAvailable()) {

    // Determine touch coordinates
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();


    if (signal_screen.btn_gleich.isPressed(x, y)) {

      signal_screen.buttonGroup.selectedButton(&signal_screen.btn_gleich);

      if (signal_screen.btn_gleich.stateSchalter) {

        signal_screen.anz_ausgang.anzeige_ausg = 4;
        signal_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(4);

      } else {
        signal_screen.anz_ausgang.anzeige_ausg = 0;
        signal_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(0);
      }

    }

    if (signal_screen.btn_sinus.isPressed(x, y)) {

      if (signal_screen.btn_sinus.stateSchalter) {
        // signal_screen.anz_ausgang.anzeige_ausg = 5;
        signal_screen.anz_ausgang.anzeige_ausg = 0;
        signal_screen.anz_ausgang.drawButton();


      } else {
        signal_screen.anz_ausgang.anzeige_ausg = 0;
        signal_screen.anz_ausgang.drawButton();


      }

      signal_screen.buttonGroup.selectedButton(&signal_screen.btn_sinus);

    }

    if (signal_screen.btn_regler_amplitude.isPressed(x, y)) {

      max_hardware.shiftOut_P(  (int)signal_screen.btn_regler_amplitude.regler_value);

    }

    if (signal_screen.btn_regler_frequenz.isPressed(x, y)) {
      //... Get values from controller
      Serial.print("Frequenz: ");  Serial.println(signal_screen.btn_regler_frequenz.regler_value);


    }

    if (signal_screen.btn_back.isPressed(x, y)) {

      signal_screen.anz_ausgang.anzeige_ausg = 0;

      // Reset hardware.
      max_hardware.shiftOut_T(0);

      currentScreen = '0';
      main_screen.drawScreen();
    }

  }
}

/*
   Perform functions on the fixed screen.
*/
void perform_Fest() {

  // Pressing the Touch
  if (myTouch.dataAvailable()) {

    // Determine touch coordinates
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();


    // Was 5V selected?
    if (fest_screen.btn_fest_5.isPressed(x, y)) {

      fest_screen.buttonGroup.selectedButton(&fest_screen.btn_fest_5);

      if (fest_screen.btn_fest_5.stateSchalter) {

        fest_screen.anz_ausgang.anzeige_ausg = 3;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(2); //
      } else {

        fest_screen.anz_ausgang.anzeige_ausg = 0;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(0);
      }

    }

    // Wurde 12V gewählt?
    if (fest_screen.btn_fest_12.isPressed(x, y)) {

      fest_screen.buttonGroup.selectedButton(&fest_screen.btn_fest_12);

      if (fest_screen.btn_fest_12.stateSchalter) {

        fest_screen.anz_ausgang.anzeige_ausg = 1;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(1);

      } else {
        fest_screen.anz_ausgang.anzeige_ausg = 0;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(0);

      }

    }

    // Was -12V selected?
    if (fest_screen.btn_fest_m12.isPressed(x, y)) {

      fest_screen.buttonGroup.selectedButton(&fest_screen.btn_fest_m12);


      if (fest_screen.btn_fest_m12.stateSchalter) {
        fest_screen.anz_ausgang.anzeige_ausg = 2;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(3);
      } else {
        fest_screen.anz_ausgang.anzeige_ausg = 0;
        fest_screen.anz_ausgang.drawButton();

        max_hardware.shiftOut_T(0);
      }

    }


    if (fest_screen.btn_back.isPressed(x, y)) {

      fest_screen.anz_ausgang.anzeige_ausg = 0;
      // Reset hardware.
      max_hardware.shiftOut_T(0);

      currentScreen = '0';
      main_screen.drawScreen();
    }

  }

}


