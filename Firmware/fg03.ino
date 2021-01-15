#include <UTFT.h>
#include <UTouch.h>
//
// Include All Peripheral Libraries Used By LINX and function generator library.
//
// Due to limitations in the Arduino environment, SPI.h must be included both
// in the library which uses it *and* any sketch using that library.
//
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
//
// Include Device Sepcific Header From Sketch>>Import Library (In This Case LinxArduinoMega2560.h)
// Also Include Desired LINX Listener From Sketch>>Import Library (In This Case LinxSerialListener.h)
//
#include <LinxArduinoMega2560.h>
#include <LinxSerialListener.h>
//
//  Include settings header.  Includes reference to SparkFun_MiniGen.h
//
#include "settings.h"
//
//  Create A Pointer To The LINX Device Object We Instantiate In Setup()
//
LinxArduinoMega2560* LinxDevice;
//
// Library inclusion, pin declaration and initialisation:
//
#include <Q2HX711.h>
//
//  Setup pins for load cell.
//
const byte hx711_data_pin = 31; // HX711 data line connected to arduino D3
const byte hx711_clock_pin =  30; // HX711 clock line connected to arduino D2
//
// Declare instance of Q2H71 library
//
Q2HX711 hx711(hx711_data_pin, hx711_clock_pin);
//
//  Support for LabVIEW call for collecting multiple samples in a single call
//
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
//
//  Declare screens
//
enum screen
{
    SC_MAIN = 0,
    SC_FNGEN = 1,
    SC_DCPOW = 2,
    SC_PWM = 3,
    SC_SUB4 = 4
};
//
//  Create an object to store the current settings.
//
Settings objSettings;
//
//  Create an instance of the MiniGen device.
//  Passing an argument sets the pin number for CS.  
//  Note: There is no provision for setting alternate CLK and MOSI pins.
//
MiniGen gen(48);
//
//  Set PWM pins used by function generator.
//
int PWMGain = 11;
int PWMOffset = 12;
//
// CurrentPosition1 is the current slider position.
// oldposition1 is the prior slider position
//
int gCurrentPosition1;
int gOldPosition1;

int gCurrentPosition2;
int gOldPosition2;

int gCurrentPosition3;
int gOldPosition3;

int gCurrentPosition4;
int gOldPosition4;

int gCurrentPosition5;
int gOldPosition5;
int DutyCycle=50;
int DutyCycleMin=0;
int DutyCycleMax=100;

int PinNum=44;
int PinNumMin = 44;
int PinNumMax = 46;

//LCD:  4Line  serial interface      SDI  SCL  /CS  /RST  D/C    NOTE:Only support  DUE   MEGA  UNO
UTFT myGLCD(ILI9163_4L, 3, 2, 9, 10, 7);
//RTP: byte tclk, byte tcs, byte din, byte dout, byte irq
UTouch myTouch(2, 6, 3, 4, 5);

extern uint8_t BigFont[];
extern uint8_t SmallFont[];
//
// Declare variable to hold screen state
//
short int SCREEN_STATE=0;
//
//  Variable to store the current state of a screen.
//  For instance, a menu screen might display 
//  different information depending on the state
//  of the input
//
String OPTION_STATE="";
//
//  Define pins used
//
const int buttonPin = 38;    // the number of the pushbutton pin
const int buttonPin2 = 40;    // the number of the pushbutton pin
const int ledPin =  32;      // the number of the LED pin
//
//  Display information on TFT when labview calls is active
//
bool LabViewCommand3Active = false;
bool LabViewCommand4Active = false;
bool LabViewCommand5Active = false;
bool LabViewCommand6Active = false;
bool LabViewCommand7Active = false;
bool LabViewCommand8Active = false;
bool LabViewCommand9Active = false;
//
//  Variable to save state of ADCSRA so we can restore default state
//  and use Arduino based functions like readAnalog().
//
byte ADCSRA_SAVE;
// internal to counting routine
volatile unsigned long overflowCount = 0;
// Debug flag
bool gdebug = true;
//
//   Custom functions   **
//
double mapf(double val, double in_min, double in_max, double out_min, double out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void DrawSlider(int x, int y, int w, int h)
{
  int width = x + w;
  int height = y + h;

  myGLCD.setColor(VGA_GRAY);
  myGLCD.drawRect(x, y, width, height);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.fillRect(x + 2, y + 2, width - 2, height - 2);
  myGLCD.setFont(SmallFont);

  for (int i = 1; i < 15; i++)
  {
    myGLCD.setColor(VGA_BLACK);
    if (i % 5 == 0)
    {
      myGLCD.drawLine(width - 11, height - i * ((double)h / 15.0), width - 2, height - i * ((double)h / 15.0));
    }
    else
    {
      myGLCD.drawLine(width - 11, height - i * ((double)h / 15.0), width - 6, height - i * ((double)h / 15.0));
    }
  }
}

void DrawSliderHorz(int x, int y, int w, int h)
{
  int width = x + w;
  int height = y + h;

  myGLCD.setColor(VGA_GRAY);
  myGLCD.drawRect(x, y, width, height);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.fillRect(x + 2, y + 2, width - 2, height - 2);
  myGLCD.setFont(SmallFont);

  for (int i = 1; i < 25; i++)
  {
    myGLCD.setColor(VGA_BLACK);
    if (i % 5 == 0)
    {
      myGLCD.drawLine(width - i * ((double)w / 25.0), height-11, width - i * ((double)w / 25.0),height-2);
    }
    else
    {
      myGLCD.drawLine(width - i * ((double)w / 25.0), height-7, width - i * ((double)w / 25.0),height-2);
    }
  }
}

void UpdateSlider(char type, int x, int y, int w, int h, long ScaleMin, long ScaleMax, int *old_position, int *CurrentPosition)
{
  int width = x + w;
  int height = y + h;
  int posx;
  int posy;

  if (myTouch.dataAvailable())
  {
    myTouch.read();
    posx = myTouch.getX(); // X coordinate where the screen has been pressed
    posy = myTouch.getY(); // Y coordinates where the screen has been pressed
    
    if ((posy > y-5) && (posy < height+5) && (posx > x) && (posx < width))
    {
      // Map slider y value to original value
      long ScaleValue = map(posy, height, y, ScaleMin, ScaleMax);

      if (type == 'F')
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        if (ScaleValue > objSettings.getMaxFrequency()){
            ScaleValue = objSettings.getMaxFrequency();
        }
        else if (ScaleValue < objSettings.getMinFrequency())
        {
          ScaleValue = objSettings.getMinFrequency();
        }
        objSettings.setFrequency(ScaleValue);
        myGLCD.printNumI(objSettings.getFrequency(), 22, 16, 7, ' ');

        ProcessCommand();
      }
      else if (type == 'A')
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        if (ScaleValue > objSettings.getMaxGain()){
            ScaleValue = objSettings.getMaxGain();
        }
        else if (ScaleValue < objSettings.getMinGain())
        {
          ScaleValue = objSettings.getMinGain();
        }
        objSettings.setGain(ScaleValue);
        myGLCD.printNumI(objSettings.getGain()*100/(objSettings.getMaxGain()-objSettings.getMinGain()), 102, 16, 3, ' ');

        ProcessCommand();
      }
      else if (type == 'O')
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        if (ScaleValue > objSettings.getMaxOffset()){
            ScaleValue = objSettings.getMaxOffset();
        }
        else if (ScaleValue < objSettings.getMinOffset())
        {
          ScaleValue = objSettings.getMinOffset();
        }
        objSettings.setOffset(ScaleValue);
        myGLCD.printNumI(objSettings.getOffset()*100/(objSettings.getMaxOffset()-objSettings.getMinOffset()), 131, 16, 3, ' ');

        ProcessCommand();
      }
      // To get the slider to zero they will click below the slider area.  However,
      // we do not want to display the slider outside the slider viewport.
      if (posy > height - 5)
        posy = height - 5;

      if (posy < y)
        posy = y;  

      *old_position = *CurrentPosition; //savelast Position
      *CurrentPosition = posy;
      // Reset color of prior reading before setting new value.  Do not
      // do this if original position has not been set.
      if (*old_position != 0)
      {
        myGLCD.setColor(VGA_WHITE);
        myGLCD.fillRect(x + 2, *old_position, x + 12, *old_position + 3);
      }
      myGLCD.setColor(VGA_RED);
      myGLCD.fillRect(x + 2, *CurrentPosition, x + 12, *CurrentPosition + 3);
    }
  }
}

void UpdateSliderHorz(char type, int x, int y, int w, int h, long ScaleMin, long ScaleMax, int *old_position, int *CurrentPosition)
{
  int width = x + w;
  int height = y + h;
  int posx;
  int posy;

  if (myTouch.dataAvailable())
  {
    myTouch.read();
    posx = myTouch.getX(); // X coordinate where the screen has been pressed
    posy = myTouch.getY(); // Y coordinates where the screen has been pressed

    if ((posx > x-5) && (posx < width+5) && (posy > y) && (posy < height))
    {
      // Map slider y value to original value
      long ScaleValue = map(posx, width, x, ScaleMax, ScaleMin);

      if (type == 'O')
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        if (ScaleValue > objSettings.getMaxOffset()){
            ScaleValue = objSettings.getMaxOffset();
        }
        else if (ScaleValue < objSettings.getMinOffset())
        {
          ScaleValue = objSettings.getMinOffset();
        }
        objSettings.setOffset(ScaleValue);
        myGLCD.printNumF(100*(float)objSettings.getOffset()/objSettings.getMaxOffset(),1, 51, 18, '.',6);

        ProcessCommand();
      }
      else if (type == 'P')
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        if (ScaleValue > DutyCycleMax){
            ScaleValue = DutyCycleMax;
        }
        else if (ScaleValue < DutyCycleMin)
        {
          ScaleValue = DutyCycleMin;
        }
        DutyCycle = ScaleValue;
        myGLCD.printNumF(100*(float)DutyCycle/DutyCycleMax,1, 51, 18, '.',6);
        //
        //  Output PWM signal
        //
        analogWrite(PinNum,DutyCycle*255/(DutyCycleMax-DutyCycleMin));
      }
      // To get the slider to zero they will click below the slider area.  However,
      // we do not want to display the slider outside the slider viewport.
      if (posx > width - 5)
        posx = width - 5;

      if (posx < x)
        posx = x;  

      *old_position = *CurrentPosition; //savelast Position
      *CurrentPosition = posx;
      // Reset color of prior reading before setting new value.  Do not
      // do this if original position has not been set.
      if (*old_position != 0)
      {
        myGLCD.setColor(VGA_WHITE);
        myGLCD.fillRect(*old_position, y + 2, *old_position + 3, y + 12);
      }
      myGLCD.setColor(VGA_RED);
      myGLCD.fillRect(*CurrentPosition, y + 2, *CurrentPosition + 3, y + 12);
    }
  }
}
//
//  Routine to draw user button to screen.
//
void drawButton(int x, int y, int width, int height, char* caption, int xpad, int ypad)
{
  myGLCD.setColor(VGA_WHITE);
  myGLCD.fillRoundRect(x + 1, y + 1, x + width, y + height);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.print(caption, x + xpad, y + ypad);
  myGLCD.setBackColor(VGA_BLACK);
}
//
//  Routine to update user button presses.
//
void UpdateButton(int x, int y, int width, int height, char* caption)
{
  int lwidth = x + width;
  int lheight = y + height;
  int posx;
  int posy;

  if (myTouch.dataAvailable())
  {
    myTouch.read();
    posx = myTouch.getX(); // X coordinate where the screen has been pressed
    posy = myTouch.getY(); // Y coordinates where the screen has been pressed

    if ((posy > y) && (posy < lheight) && (posx > x) && (posx < lwidth))
    {
      myGLCD.setColor(255, 0, 0);
      myGLCD.drawRoundRect(x, y, x + width, y + height);
      myGLCD.drawRoundRect(x + 1, y + 1, x + width - 1, y + height - 1);
      while (myTouch.dataAvailable())
      {
        myTouch.read();
      }
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect(x, y, x + width, y + height);
      myGLCD.drawRoundRect(x + 1, y + 1, x + width - 1, y + height - 1);

      if (caption == "SINE")
      {
        objSettings.setMode('Q');
        myGLCD.setColor(VGA_WHITE);
        myGLCD.fillRoundRect(x + 1, y + 1, x + width, y + height);
        myGLCD.setBackColor(VGA_WHITE);
        myGLCD.setColor(VGA_BLACK);
        myGLCD.print("SQUARE", x + 10, y + 2);
        myGLCD.setBackColor(VGA_BLACK);
      }
      else if (caption == "SQUARE")
      {
        objSettings.setMode('T');
        myGLCD.setColor(VGA_WHITE);
        myGLCD.fillRoundRect(x + 1, y + 1, x + width, y + height);
        myGLCD.setBackColor(VGA_WHITE);
        myGLCD.setColor(VGA_BLACK);
        myGLCD.print("SAW", x + 20, y + 2);
        myGLCD.setBackColor(VGA_BLACK);
      }
      else if (caption == "SAW")
      {
        objSettings.setMode('S');
        myGLCD.setColor(VGA_WHITE);
        myGLCD.fillRoundRect(x + 1, y + 1, x + width, y + height);
        myGLCD.setBackColor(VGA_WHITE);
        myGLCD.setColor(VGA_BLACK);
        myGLCD.print("SINE", x + 18, y + 2);
        myGLCD.setBackColor(VGA_BLACK);
      }
      else if (caption == "10-100")
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        objSettings.setFrequency(10);
        objSettings.setMinFrequency(10);
        objSettings.setMaxFrequency(100);
        myGLCD.printNumI(10, 22, 16, 7, ' ');
        DrawSlider(70, 31, 25, 94);
      }
      else if (caption == "100-1k")
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        objSettings.setFrequency(100);
        objSettings.setMinFrequency(100);
        objSettings.setMaxFrequency(1000);
        myGLCD.printNumI(100, 22, 16, 7, ' ');
        DrawSlider(70, 31, 25, 94);
      }
      else if (caption == "1k-10k")
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        objSettings.setFrequency(1000);
        objSettings.setMinFrequency(1000);
        objSettings.setMaxFrequency(10000);
        myGLCD.printNumI(1000, 22, 16, 7, ' ');
        DrawSlider(70, 31, 25, 94);
      }
      else if (caption == "10-100k")
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        objSettings.setFrequency(10000);
        objSettings.setMinFrequency(10000);
        objSettings.setMaxFrequency(100000);
        myGLCD.printNumI(10000, 22, 16, 7, ' ');
        DrawSlider(70, 31, 25, 94);
      }
      else if (caption == "100-1M")
      {
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_YELLOW);
        objSettings.setFrequency(100000);
        objSettings.setMinFrequency(100000);
        objSettings.setMaxFrequency(1000000);
        myGLCD.printNumI(100000, 22, 16, 7, ' ');
        DrawSlider(70, 31, 25, 94);
      }
      else if (caption == "+")
      {
        if (SCREEN_STATE == SC_FNGEN){
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (objSettings.getMaxFrequency() == 1000000){
            objSettings.setFrequency(objSettings.getFrequency() + 100);
          }
          else if (objSettings.getMaxFrequency() == 100000){
            objSettings.setFrequency(objSettings.getFrequency() + 10);
          }
          else
          {
            objSettings.setFrequency(objSettings.getFrequency() + 1);
          }
          myGLCD.printNumI(objSettings.getFrequency(), 22, 16, 7, ' ');
        }
        else if (SCREEN_STATE == SC_PWM){
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (DutyCycle != DutyCycleMax)
          {
            DutyCycle = DutyCycle + 1;
          }
          myGLCD.printNumF(100.0*(float)DutyCycle/DutyCycleMax,1, 51, 18, '.',6);
          //
          //  Output PWM signal
          //
          analogWrite(PinNum,DutyCycle*255/(DutyCycleMax-DutyCycleMin));
        }
        else
        {
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (objSettings.getOffset() != objSettings.getMaxOffset())
          {
            objSettings.setOffset(objSettings.getOffset() + 1);
          }
          myGLCD.printNumF(100*(float)objSettings.getOffset()/objSettings.getMaxOffset(),1, 51, 18, '.',6);
        }
      }
      else if (caption == "-")
      {
        if (SCREEN_STATE == SC_FNGEN){
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (objSettings.getMaxFrequency() == 1000000){
            objSettings.setFrequency(objSettings.getFrequency() - 100);
          }
          else if (objSettings.getMaxFrequency() == 100000){
            objSettings.setFrequency(objSettings.getFrequency() - 10);
          }
          else
          {
            objSettings.setFrequency(objSettings.getFrequency() - 1);
          }
          myGLCD.printNumI(objSettings.getFrequency(), 22, 16, 7, ' ');
        }
        else if (SCREEN_STATE == SC_PWM){
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (DutyCycle != DutyCycleMin)
          {
            DutyCycle = DutyCycle - 1;
          }
          myGLCD.printNumF(100.0*(float)DutyCycle/DutyCycleMax,1, 51, 18, '.',6);
           //
          //  Output PWM signal
          //
          analogWrite(PinNum,DutyCycle*255/(DutyCycleMax-DutyCycleMin));
        }
        else{
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (objSettings.getOffset() != objSettings.getMinOffset())
          {
            objSettings.setOffset(objSettings.getOffset() - 1);
          }
          myGLCD.printNumF(100.0*(float)objSettings.getOffset()/objSettings.getMaxOffset(),1, 51, 18, '.',6);
        }
      }
      else if (caption == "+pin")
      {
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (PinNum != PinNumMax)
          {
            PinNum = PinNum + 1;
          }
          myGLCD.printNumI(PinNum,51,82,6);
          //
          //  Output PWM signal
          //
          analogWrite(PinNum,DutyCycle*255/(DutyCycleMax-DutyCycleMin));
      }
      else if (caption == "-pin")
      {
          myGLCD.setColor(VGA_BLACK);
          myGLCD.setBackColor(VGA_YELLOW);
          if (PinNum != PinNumMin)
          {
            PinNum = PinNum - 1;
          }
          myGLCD.printNumI(PinNum, 51,82,6);
          //
          //  Output PWM signal
          //
          analogWrite(PinNum,DutyCycle*255/(DutyCycleMax-DutyCycleMin));
      }
      else if (caption == "FUNCTION GENERATOR"){
        ShowDisplay(SC_FNGEN,"");
      }
      else if (caption == " DC POWER SUPPLY "){
        ShowDisplay(SC_DCPOW,"");
      }
      else if (caption == "   PWM CONTROL  "){
        ShowDisplay(SC_PWM,"");
      }
      else if (caption == "GO BACK"){
        ShowDisplay(SC_MAIN,"");
      }
      if (SCREEN_STATE == SC_FNGEN)
      {
        ProcessCommand();
      }
      else if (SCREEN_STATE == SC_DCPOW){
        // Process current settings.
        ProcessCommand();
      }
     
    }
  }
}
//
//  Setup connection to function generator.
//
void InitializeConnection()
{
  //
  //  Set the default mode.
  //
  objSettings.setMode('S');
  objSettings.setFrequency(100);
  objSettings.setMinFrequency(100);
  objSettings.setMaxFrequency(1000);
  objSettings.setGain(128);
  objSettings.setMinGain(0);
  objSettings.set5VGain(0);
  objSettings.setMaxGain(255);
  objSettings.setOffset(125);
  objSettings.setMinOffset(0);  
  objSettings.setZeroOffset(150);
  objSettings.setMaxOffset(250);  
  //
  //  Change the PWM frequency to 31372.55Hz
  //  Pins 11 and 12 on the Mega are controlled by register TCCR1B.
  //
  TCCR1B = TCCR1B & B11111000 | B00000001; // set timer 1 divisor to 1 for PWM frequency of 31372.55 Hz
  //
  //  Set pins 11 and 12 for output.
  //
  pinMode(PWMGain, OUTPUT);
  pinMode(PWMOffset, OUTPUT);
  //
  //  This command will reset the MiniGen to its default behavior.
  //  It clears the phase offset registers, resets the frequency to 100Hz, and 
  //  disables the output, resulting in a DC voltage at approximately 1/2 the 
  //  supply voltage on the output.
  //
  gen.reset();
  delay(2000);
  //
  //  Process current command.
  //
  ProcessCommand();
}

void ProcessCommand()
{
  //
  //  Set the signal generation model.  The parameter can be one of the following values: 
  //    MiniGen::TRIANGLE
  //    MiniGen::SINE
  //    MiniGen::SQUARE
  //    MiniGen::SQUARE_2
  //  The frequency of the output will depend on the value in the selected frequency register 
  //  For the first three options, the frequency will be at the set frequency, but for the fourth, 
  //  it will be one-half the set frequency.
  //
  if (objSettings.getMode() == 'S'){
    gen.setMode(MiniGen::SINE);
  }
  else if (objSettings.getMode() == 'T'){
    gen.setMode(MiniGen::TRIANGLE);
  }
  else{
    gen.setMode(MiniGen::SQUARE);
  }
  //
  //  This needs a little explanation. The choices are FULL, COARSE, and FINE.
  //  a FULL write takes longer but writes the entire frequency word, so you
  //  can change from any frequency to any other frequency. COARSE only allows
  //  you to change the upper bits; the lower bits remain unchanged, so you
  //  can do a fast write of a large step size. FINE is the opposite; quick
  //  writes but smaller steps.
  //
  gen.setFreqAdjustMode(MiniGen::FULL);
  //
  // Convert frequency in Hz to a useful 32-bit value.
  //
  float freq = 1000;
  float Pp;
  //unsigned long freqReg = gen.freqCalc(freq);
  unsigned long freqReg = gen.freqCalc((float)objSettings.getFrequency());
  //Serial.print(freqReg);
  // Adjust the frequency. This is a full 32-bit write.
  gen.adjustFreq(MiniGen::FREQ0, freqReg);
  //
  //  For square wave always generate a 0 to 5V pulse train
  //
  if (objSettings.getMode() == 'Q')
  {
    // Set the gain to a value between 0 and 255.
    analogWrite(PWMGain,objSettings.get5VGain());
    // Set the offset to a value between 0 and 255.
    analogWrite(PWMOffset,objSettings.getZeroOffset());
  }
  else if (objSettings.getMode() == 'D')
  {
    // Set the gain to a value between 0 and 255.
    analogWrite(PWMGain,objSettings.getGain());
    // Set the offset to a value between 0 and 255.
    Pp = objSettings.getOffset();
    analogWrite(PWMOffset,(int)( (0.0006*Pp*Pp + Pp)*(255.0/294.0) ) );
  }
  else 
  {
    // Set the gain to a value between 0 and 255.
    analogWrite(PWMGain,objSettings.getGain());
    // Set the offset to a value between 0 and 255.
    analogWrite(PWMOffset,objSettings.getOffset());
  }
  
}
//
//
//
void RestoreLabViewState(){
  LabViewCommand3Active = false;
  LabViewCommand4Active = false;
  LabViewCommand5Active = false;
  LabViewCommand6Active = false;
  LabViewCommand7Active = false;
  LabViewCommand8Active = false;
  LabViewCommand9Active = false;
  //
  //  Restore default state and use Arduino based functions like readAnalog().
  //
  ADCSRA = ADCSRA_SAVE;
  //
  //  Reset counter.
  //
  //gCounter = 0;
}
//
//  Routine to display proper menu based on selections
//
void ShowDisplay(screen val, char* optionstate)
{
    //Store screen state.
    SCREEN_STATE = val;
    OPTION_STATE = optionstate;

    myGLCD.InitLCD(LANDSCAPE);
    myGLCD.clrScr();

    myTouch.InitTouch();
    myTouch.setPrecision(PREC_MEDIUM);

    myGLCD.setFont(BigFont);
    myGLCD.setBackColor(0, 0, 0);
   
    if (val == SC_MAIN)
    {
      myGLCD.setFont(SmallFont);
      myGLCD.setBackColor(VGA_PURPLE);
      myGLCD.fillScr(VGA_PURPLE);
      drawButton(2, 14, 150, 30, "FUNCTION GENERATOR", 6,10); 
      drawButton(2, 50, 150, 30, " DC POWER SUPPLY ", 10,10); 
      drawButton(2, 86, 150, 30, "   PWM CONTROL  ", 10,10); 
    }
    else if (val == SC_FNGEN)
    {
      //
      //  Restore defaults and disable labVIEW specific code
      //
      RestoreLabViewState();
      //
      //  Initialize connection to function generator.
      //
      InitializeConnection();
     
      myGLCD.setColor(VGA_GREEN);
      myGLCD.drawRect(1, 1, 97, myGLCD.getDisplayYSize());
      DrawSlider(70, 31, 25, 94);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("Freqency(Hz)", 6, 2); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumI(objSettings.getFrequency(), 22, 16, 7, ' ');
    
      myGLCD.setColor(VGA_RED);
      myGLCD.drawRect(99, 1, 127, myGLCD.getDisplayYSize());
      DrawSlider(100, 31, 25, 94);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.setBackColor(VGA_BLACK);
      myGLCD.print("Amp", 102, 2); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumI(objSettings.getGain()*100/(objSettings.getMaxGain()-objSettings.getMinGain()), 102, 16, 3, ' ');

      myGLCD.setColor(VGA_YELLOW);
      myGLCD.drawRect(129, 1, myGLCD.getDisplayXSize() - 3, myGLCD.getDisplayYSize());
      DrawSlider(130, 31, 25, 94);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.setBackColor(VGA_BLACK);
      myGLCD.print("Off", 133, 2); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumI(objSettings.getOffset()*100/(objSettings.getMaxOffset()-objSettings.getMinOffset()), 131, 16, 3, ' ');

      drawButton(5, 14, 14, 14, "-", 6,2);
      drawButton(80, 14, 14, 14, "+", 6,2);
      drawButton(5, 30, 60, 14, "SINE", 18,2);
      drawButton(5, 46, 60, 14, "10-100", 8,2);
      drawButton(5, 62, 60, 14, "100-1k", 8,2);
      drawButton(5, 78, 60, 14, "1k-10k", 8,2);
      drawButton(5, 94, 60, 14, "10-100k", 2,2);
      drawButton(5, 110, 60, 14, "100-1M", 8,2);
      
    }
    else if (val == SC_DCPOW)
    {
      //
      //  Initialize connection to function generator.  By default, after
      //  a reset the output voltage is 1/2 the max voltage.  Frequency 
      //  output is disabled.
      //
      InitializeConnection();
      //
      //  Restore defaults and disable labVIEW specific code
      //
      RestoreLabViewState();
      //
      //  Set frequency to 0 Hz for DC
      //
      objSettings.setFrequency(0);
      objSettings.setMode('D');
      
      myGLCD.setColor(VGA_GREEN);
      myGLCD.drawRect(1, 1, 158, myGLCD.getDisplayYSize());
      DrawSliderHorz(5, 31, 150, 25);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("DC Voltage (%)", 30, 2); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumF(100*(float)objSettings.getOffset()/objSettings.getMaxOffset(),1, 51, 18, '.',6);

      drawButton(31, 14, 14, 14, "-", 6,2);
      drawButton(105, 14, 14, 14, "+", 6,2);

      drawButton(45, 86, 70, 30, "GO BACK", 10,10);
    }
    else if (val == SC_PWM)
    {
      //
      //  Restore defaults and disable labVIEW specific code
      //
      RestoreLabViewState();
      myGLCD.setColor(VGA_GREEN);
      myGLCD.drawRect(1, 1, 158, myGLCD.getDisplayYSize());
      DrawSliderHorz(5, 31, 150, 25);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("Duty Cycle (%)", 27, 2); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumF(100*(float)DutyCycle/DutyCycleMax,1, 51, 17, '.',6);

      drawButton(31, 14, 14, 14, "-", 6,2);
      drawButton(105, 14, 14, 14, "+", 6,2);

      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("Pin #", 55, 65); // Prints the string on the screen
      myGLCD.setColor(VGA_BLACK);
      myGLCD.setBackColor(VGA_YELLOW);
      myGLCD.printNumI(PinNum,51,82,6);

      drawButton(31, 80, 14, 14, "-", 6,2);
      drawButton(105, 80, 14, 14, "+", 6,2);

      drawButton(45, 100, 70, 20, "GO BACK", 10,5);
    }
    else if (val == SC_SUB4)
    {
        
    }
}

void setup()
{

  //PWM on timer 5 needs to be high (31372.55)
  TCCR5B = TCCR5B & B11111000 | B00000001;
  //Instantiate The LINX Device
  LinxDevice = new LinxArduinoMega2560();
  
  //The LINX Listener Is Pre Instantiated, Call Start And Pass A Pointer To The LINX Device And The UART Channel To Listen On
  LinxSerialConnection.Start(LinxDevice, 0); 
  //
  //  Save ADC state so it can be restored later.
  //
  ADCSRA_SAVE = ADCSRA;
  //
  //  Setup led pin for output
  //
  pinMode(ledPin,OUTPUT);
  pinMode(18,OUTPUT);
  digitalWrite(18,LOW);
  pinMode(19,INPUT_PULLUP);

  //
  // Initial setup
  //
  //Serial.begin(115200);
  //
  // initialize the pushbutton pin as an input:
  //
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin2,INPUT);
  //
  // Show main menu when Arduino starts
  // for the 1st time or on reset.
  //
  ShowDisplay(SC_MAIN, "");
}

void (* resetFunc)(void)=0;

void loop()
{
  // unsigned char numInputBytes;
  // unsigned char* input;
  // unsigned char* numResponseBytes;
  // unsigned char* response;
   //Listen For New Packets From LabVIEW
  LinxSerialConnection.CheckForCommands();
  //
  // Put custom code here...It will Slow Down The Connection With LabVIEW
  // Custom commands should have number 0-15 
  // Custom command of 3 was chosen ==> = hex '3'
  LinxSerialConnection.AttachCustomCommand(0x3, readout_AnalogReadBurstDiff);
  // Custom command of 4 was chosen ==> = hex '4'
  LinxSerialConnection.AttachCustomCommand(0x4, readout_AnalogReadBurst);
  // Custom command of 5 was chosen ==> = hex '5'
  LinxSerialConnection.AttachCustomCommand(0x5, readout_PulseGen);
    // Custom command of 6 was chosen ==> = hex '6'
  LinxSerialConnection.AttachCustomCommand(0x6, readout_PulseCount);
  // if (gdebug == true){
  //   readout_PulseCount(numInputBytes,input,numResponseBytes,response);
  //   gdebug = false;
  // }
  // Custom command of 7 was chosen ==> = hex '7'
  LinxSerialConnection.AttachCustomCommand(0x7, readout_PortSet);
  // Custom command of 8 was chosen ==> = hex '8'
  LinxSerialConnection.AttachCustomCommand(0x8, readout_Diff); 
  // Custom command of 9 was chosen ==> = hex '9'
  LinxSerialConnection.AttachCustomCommand(0x9, readout_HX11); 
  //
  //  If use presses push button, then return to main menu
  //
  if (digitalRead(buttonPin) == LOW)
  {
    //
    //  Process screen input based on screen displayed
    //
    if(SCREEN_STATE == SC_MAIN)
    {
      UpdateButton(2, 14, 150, 30, "FUNCTION GENERATOR"); 
      UpdateButton(2, 50, 150, 30, " DC POWER SUPPLY "); 
      UpdateButton(2, 86, 150, 30, "   PWM CONTROL  "); 
    }
    else if (SCREEN_STATE == SC_FNGEN)
    {
      //
      //  Update sliders
      //
      UpdateSlider('F', 70, 31, 25, 94, objSettings.getMinFrequency(), objSettings.getMaxFrequency(), &gOldPosition1, &gCurrentPosition1);
      UpdateSlider('A', 100, 31, 25, 94, objSettings.getMinGain(), objSettings.getMaxGain(), &gOldPosition2, &gCurrentPosition2);
      UpdateSlider('O', 130, 31, 25, 94, objSettings.getMinOffset(), objSettings.getMaxOffset(), &gOldPosition3, &gCurrentPosition3);
     
      UpdateButton(5, 14, 14, 14, "-");
      UpdateButton(80, 14, 14, 14, "+");
      if (objSettings.getMode() == 'S')
      {
        UpdateButton(5, 30, 60, 14, "SINE");
      }
      else if (objSettings.getMode() == 'Q')
      {
        UpdateButton(5, 30, 60, 14, "SQUARE");
      }
      else if (objSettings.getMode() == 'T')
      {
        UpdateButton(5, 30, 60, 14, "SAW");
      }
      UpdateButton(5, 46, 60, 14, "10-100");
      UpdateButton(5, 62, 60, 14, "100-1k");
      UpdateButton(5, 78, 60, 14, "1k-10k");
      UpdateButton(5, 94, 60, 14, "10-100k");
      UpdateButton(5, 110, 60, 14, "100-1M");
    }
    else if (SCREEN_STATE == SC_DCPOW)
    {
      //
      //  Update sliders
      //
      UpdateSliderHorz('O', 5, 31, 150, 25, objSettings.getMinOffset(), objSettings.getMaxOffset(), &gOldPosition4, &gCurrentPosition4);
      UpdateButton(31, 14, 14, 14, "-");
      UpdateButton(104, 14, 14, 14, "+");
      UpdateButton(45, 86, 70, 30, "GO BACK");
    }
    else if (SCREEN_STATE == SC_PWM)
    {
      //
      //  Update sliders
      //
      UpdateSliderHorz('P', 5, 31, 150, 25, DutyCycleMin, DutyCycleMax, &gOldPosition5, &gCurrentPosition5);
      UpdateButton(31, 14, 14, 14, "-");
      UpdateButton(104, 14, 14, 14, "+");

      UpdateButton(31, 80, 14, 14, "-pin");
      UpdateButton(105, 80, 14, 14, "+pin");

      UpdateButton(45, 100, 70, 20, "GO BACK");
    }
  }
  else
  {
    /* Lets do some diagnostics */
   if (digitalRead(buttonPin2) == HIGH) { 
     TCCR1B = TCCR1B & B11111000 | B00000001; // set timer 1 divisor to 1 for PWM frequency of 31372.55 Hz

     analogWrite(PWMGain,64);
     analogWrite(PWMOffset,192);
     for (int p = 22; p < 54; p++) { 
      pinMode(p,INPUT_PULLUP);
      }
      myGLCD.setFont(SmallFont);
      myGLCD.setBackColor(VGA_PURPLE);
      myGLCD.fillScr(VGA_PURPLE);
      while (digitalRead(19)==LOW) {
        for (int p = 22; p < 54; p+=2) {
          myGLCD.printNumI(digitalRead(p),(p-20)*4,10);
          delay(10);
          }
        for (int p = 23; p < 54; p+=2) {
          myGLCD.printNumI(digitalRead(p),(p-21)*4,20);
          delay(10);
          }
        myGLCD.printNumF(4.85*analogRead(A0)/1024,2,4,30);
        myGLCD.printNumF(4.85*analogRead(A1)/1024,2,4,40);
        }
     resetFunc();
     }
   else {
    ShowDisplay(SC_MAIN,"");
    }
  }
}
//
//  Blink LED on labVIEW call start and end.
//
void blinkled(bool start){
  if (start == true){
    digitalWrite(ledPin,HIGH);
    delay(50);
    digitalWrite(ledPin,LOW);
    delay(50);
    digitalWrite(ledPin,HIGH);
    delay(50);
    digitalWrite(ledPin,LOW);
    delay(50);
  }
}
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_HX11(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  //
  // Functionality:
  // Converts a float into an int, then into 2 bytes and sends this to Labview, 
  // You can apply a scaling factor before, to choose the required resolution.
  // 
  // Inputs (from Labview): None
  // 
  // Outputs: 
  //      2 bytes containing data of a float.  The first byte is stored in response[0] and the second
  //      byte is stored in response[1].  
  //
  //  Read signal from load cell.
  //
  //  Call should return # between 0 and 167,772,15 in unsigned form (8388608 to -8388607 in signed form) or
  //  800000 to 7FFFFF is 2s complement.  You will notice in .read() the statement data[2] ^= 0x80.  This replaces
  //  the 1st bit with 1 if and only if the 1st bit in data[2] is zero.  
  //
  //  Make sense?  Probably not.  The load cell returns the data in 2s complement.  Thus consider what is
  //  returned for -1 and 1.
  //
  //  1.  At -1 the 24 binary is returned as 1111 1111 1111 1111 1111 1111
  //      From 0 the 24 binary is returned as 0000 0000 0000 0000 0000 0000
  //      For +1 the 24 binary is returned as 0000 0000 0000 0000 0000 0001
  //  2.  The command data[2] ^= 0x80  changes the value of 
  //                  -1 to 0111 1111 1111 1111 1111 1111  
  //                   0 to 1000 0000 0000 0000 0000 0000
  //                  +1 to 1000 0000 0000 0000 0000 0001
  //  4.  Thus the conversion are +1 = 0111 1111 1111 1111 1111 1110 = 8388607
  //                               0 = 1000 0000 0000 0000 0000 0000 = 8388608
  //                          and -1 = 1000 0000 0000 0000 0000 0001 = 8388609
  //  5.  Now we want to shift the output about zero.
  //      Therefore, if the vlaue is greater than 8388608 we subtract 8388608 to obtain 1.  Thus,
  //      -1 becomes 8388609 - 8388608 = 1  -> We pass a variable to tell labVIEW it is negative.
  //      +1 becomes 8388608 - 8388607 = 1
  //
  // Make scale call.
  unsigned long value = hx711.read();
  //
  //  Set a scaling factor for the load cell
  //
  unsigned int scaler = input[0]; 
  //
  //  Clear screen the first time labview call is processed.
  //
  if (LabViewCommand9Active == false){
    //  Blink LED
    blinkled(true);
    myGLCD.fillScr(VGA_PURPLE);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setBackColor(VGA_PURPLE);
    myGLCD.print("WORKING",50,50);
    LabViewCommand9Active = true;
  }
  // Deal with negative scale readings
  if (value > 8388608)  // 8388608 is the 2s complement of 800000
  {
    value = scaler*(value - 8388608);
    response[3] = (1 & 0x0000FF);  //bitwise AND.  Bit transferred as 1 if both bit positions are 1.    
  }
  else
  {
    value = scaler*(8388608-value);
    response[3] = (0 & 0x0000FF);
  }
  //
  //  Echo scale output to screen
  //
   myGLCD.printNumI(value,27,70,10);
  //
  // Set number of bytes to send back to LabVIEW
  //
  *numResponseBytes = 4;
  //
  //  Setup data to send back to LabVIEW
  //
  // Conversion of 0xFF00 ==> F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 1111 1111 0000 0000 0000 0000  -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0xFF0000)  >> 16 masks out the lower two bytes of the number stored in value and then
  //  bit shits all of the bits to right by 16.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[0] = (value & 0xFF0000) >> 16; 
  // Conversion of 0xFF00 ==> F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 0000 0000 1111 1111 0000 0000  -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0xFF00)  >> 8 masks out the lower byte of the number stored in value and then
  //  bit shits all of the bits to right by 8.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[1] = (value & 0x00FF00) >> 8; //mask top bits and bottom bits, shift 8
  //
  // Conversion of 0xFF00 ==> 0 - 0000
  //                          0 - 0000
  //                          F - 1111
  //                          F - 1111
  //
  //  Results in Binary of 0000 0000 0000 0000 1111 1111 -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0x00FF) masks out the upper bytes of the number stored in value.  This returns 
  //  the least significant byte of the the number stored in value and stoes it in response[0]
  //
  response[2] = (value & 0x0000FF); //mask top bits, no need to shift.

  return 0;
};

int read_differential() {
  uint8_t low, high;
  //
  //  Setup Positive differential iinput on ADC0 and Negative Differential Input on ADC1 with Gain 1x.
  //
  //  Set MUX5:0 to 010000.
  //    MUX4 = 4.  Thus, 1 << 4 takes 0000 0001 and shifts it to obtain 0001 0000.
  //    ADMUX|0001 0000 returns 1 in every bit position in which both either bit is 1.
  //    Refer to atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf
  //    for explaination of why this MUX5 is turned on (See Table 26.4)
  //
  ADMUX |= (1<<MUX4);             
  //
  //  ADSC = 6, Thus 1<<6 shifts to 0100 0000.  This turns on bit 6 of ADCSRA.  See page 285 of data
  //  sheet to see this starts the conversion.
  //         
  ADCSRA |= (1<<ADSC);            //start conversion
  //
  //  The 6th bit of ADCSRA is 1 until conversion completes.  The while loop below
  //  will run until the 6th bit is zero.
  //
  while (ADCSRA & (1<<ADSC));     //wait until coversion be completed(ADSC=0);
  //
  // When the conversion is complete the data is pushed into 
  // the registers ADCL and ADCH.
  //
  low = ADCL;
  high = ADCH;
  //
  // In differential mode our value is between -512 to 511 (not 0 to 1023). This means 
  // we have 9 bits and 10th bit is the sign bit. but because the number of ADCH and ADCL bits 
  // are 10, for signed number we dont have repetition of 1 in "ADCH" byte.  Thus we repeat 1 and
  // arrange (ADCH ADCL) to a 16 bit and return it's value.
  //
  if(high & (1<<1)){              
    high |= 0b11111110;           
  }                               
  return (high << 8) | low;       
}
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_Diff(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  //
  // Functionality:
  // Converts a float into an int, then into 2 bytes and sends this to Labview, 
  // You can apply a scaling factor before, to choose the required resolution.
  // 
  // Inputs (from Labview): None
  // 
  // Outputs: 
  //      2 bytes containing data of a float.  The first byte is stored in response[0] and the second
  //      byte is stored in response[1].  
  //
  //  Clear screen the first time labview call is processed.  Also, set
  //  manual ADC conversion
  //
  if (LabViewCommand8Active == false){
    //  Blink LED
    blinkled(true);
    //
    //  Set AVCC as the reference voltage for ADC, and enable ADC.
    //  REFS0 = 6.  Thus, 1 << 6 takes 0000 0001 and shifts it to obtain 0100 0000.
    //  ADMUX|0100 0000 returns 1 in every bit position in which both either bit is 1.
    //
    ADMUX = 1<<REFS0;                
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);    //enable ADC, ADC frequency=16MB/128=125kHz (set of prescaler)
    ADCSRB = 0x00;

    myGLCD.fillScr(VGA_PURPLE);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setBackColor(VGA_PURPLE);
    myGLCD.print("WORKING",50,50);
    LabViewCommand8Active = true;
  }
  //
  //  Get the differential value between -512 and 511.  
  //
  signed long value = (500.0*read_differential()/512);
  //
  //  Display voltage on the TFT.
  //
  myGLCD.printNumF((float)value/100.0,2,27,70,'.',6);
  // Deal with negative scale readings
  if (value < 0)  
  {
    value = -value;  //Reverse sign
    response[3] = 1; //Pass 1 to indicate negative value.
  }
  else
  {
     response[3] = 0;
  }
  //
  //  Base on a ref voltage of 5V.  Multiply by 100 to preserve to decimals of precision when passing 
  //  integer back to labVIEW.  Will divide by 100 in
  //  labVIEW.
  unsigned long value2 = value;
  //
  // Set number of bytes to send back to LabVIEW
  //
  *numResponseBytes = 4;
  //
  //  Setup data to send back to LabVIEW
  //
  // Conversion of 0xFF00 ==> F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 1111 1111 0000 0000 0000 0000  -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0xFF0000)  >> 16 masks out the lower two bytes of the number stored in value and then
  //  bit shits all of the bits to right by 16.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[0] = (value2 & 0xFF0000) >> 16; 
  // Conversion of 0xFF00 ==> F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 0000 0000 1111 1111 0000 0000  -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0xFF00)  >> 8 masks out the lower byte of the number stored in value and then
  //  bit shits all of the bits to right by 8.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[1] = (value2 & 0x00FF00) >> 8; //mask top bits and bottom bits, shift 8
  //
  // Conversion of 0xFF00 ==> 0 - 0000
  //                          0 - 0000
  //                          F - 1111
  //                          F - 1111
  //
  //  Results in Binary of 0000 0000 0000 0000 1111 1111 -> This is a 3 byte number and so is the variable value.
  //  The expression (value & 0x00FF) masks out the upper bytes of the number stored in value.  This returns 
  //  the least significant byte of the the number stored in value and stoes it in response[0]
  //
  response[2] = (value2 & 0x0000FF); //mask top bits, no need to shift.

  return 0;
};
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_PortSet(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  //
  // Functionality:
  // Converts a float into an int, then into 2 bytes and sends this to Labview, 
  // You can apply a scaling factor before, to choose the required resolution.
  // 
  // Inputs (from Labview): None
  // 
  // Outputs: 
  //      2 bytes containing data of a float.  The first byte is stored in response[0] and the second
  //      byte is stored in response[1].  
  //
  //  Blink LED
  blinkled(true);
  //
  //  Clear screen the first time labview call is processed.  Also, set
  //  manual ADC conversion
  //
  if (LabViewCommand7Active == false){
    myGLCD.fillScr(VGA_PURPLE);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setBackColor(VGA_PURPLE);
    myGLCD.print("SETTING PORT",20,50);
    LabViewCommand7Active = true;
  }
  unsigned char portactive = input[0]; 
  //unsigned char portactive = 11; 
  //
  for (byte i = 22; i< 30; i++)
  {
    pinMode(i,OUTPUT);
  }
  // Port A data direction register
  //DDRA &= 0b11111111;  // Sets which pins are for output.  We set all to output
  // Set which pins are enabled.
  // PORTA = B11111000; // sets digital pins 29,28,27,26,25 HIGH and 24,23,22 LOW
  //PORTA = portactive; // sets digital pins HIGH OR LOW
  for (int i=22; i < 30; i++){
    int currentBit = bitRead(portactive,i-22);
    if (currentBit == 1){
      digitalWrite(i,HIGH);
    }
    else
    {
      digitalWrite(i,LOW);
    }
  }
  //
  // Set number of bytes to send back to LabVIEW
  //
  *numResponseBytes = 1;
  
  response[0] = 1;

  return 0;
};
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_AnalogReadBurst(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  unsigned int analogValue = 0;
  unsigned char channel = input[0];
  unsigned char numSamp = input[1];
  unsigned long t0, t, delayval,rate;
  unsigned int delay_u;
  unsigned int delay_m;

  rate = ((u32)input[2] << 24) | ((u32)input[3] << 16) | ((u32)input[4] << 8) | (u32)input[5];
  delayval = 10000000UL/rate;
  if (delayval > 1000UL) { 
    delay_m = (delayval/1000UL)-1; 
    delay_u = delayval - (delay_m*1000UL) - 31;
    }
  else { 
    delay_m = 0; 
    if (delayval < 19) { delayval = 19; }
    delay_u = delayval - 19;
    }
   
  //  Blink LED
  blinkled(true);
  //
  //  Clear screen the first time labview call is processed.  Also, change clock rate.
  //
  if (LabViewCommand4Active == false){
    //
    // Set ADC to ~ 55kHz sampling rate - default is slooooow
    //
    sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); 
    //
    //  Do not update the screen state if the function gen screen is displayed.
    //
    if (SCREEN_STATE != SC_FNGEN)
    {
      myGLCD.fillScr(VGA_PURPLE);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.setBackColor(VGA_PURPLE);
      myGLCD.print("AnalogReadBurst",10,30);
      myGLCD.print("Channel:",10,50);
      myGLCD.printNumI(channel,90,50);
      myGLCD.print("Rate:",10,60);
      myGLCD.printNumI(rate/10,90,60);
      myGLCD.print("Samples:",10,70);
      myGLCD.printNumI(numSamp,90,70);

  
    }
    LabViewCommand4Active = true;
  }
  //
  //  Setup sampling
  //
  /* 
  if      (prescale == 32)  { sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); sbi(ADCSRA, ADPS0); } // ADC clk /32
  else if (prescale == 64)  { sbi(ADCSRA, ADPS2); sbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); } // ADC clk /64
  else if (prescale == 128) { sbi(ADCSRA, ADPS2); sbi(ADCSRA, ADPS1); sbi(ADCSRA, ADPS0); } // ADC clk /128 (default)
  else { sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); } // ADC clk /16 (apparently fastest stable)
  */

	//Loop Over All AI channels In Command Packet
  t0 = micros();
	for(int i=0; i<numSamp*2; i++)
	{
		analogValue = analogRead(channel);	
    if (delay_u > 0) { delayMicroseconds(delay_u); }
    if (delay_m > 0) { delay(delay_m); }
    
    response[i] = (analogValue & 0xff00) >> 8;
    i++;
    response[i] = (analogValue & 0x00ff);
	}
  t = (micros()-t0)/numSamp;

	*numResponseBytes = numSamp*2+4;
  
  response[numSamp*2] =   (t & 0xff000000) >> 24;
  response[numSamp*2+1] = (t & 0x00ff0000) >> 16;
  response[numSamp*2+2] = (t & 0x0000ff00) >> 8;
  response[numSamp*2+3] = (t & 0x000000ff);
	return 0;
};
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_AnalogReadBurstDiff(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  signed int analogValue = 0;
  uint8_t low, high;
  unsigned char channel = input[0];
  unsigned char numSamp = input[1];
  unsigned long t0, t, delayval,rate;
  unsigned int delay_u;
  unsigned int delay_m;

  rate = ((u32)input[2] << 24) | ((u32)input[3] << 16) | ((u32)input[4] << 8) | (u32)input[5];
  delayval = 10000000UL/rate;
  if (delayval > 1000UL) { 
    delay_m = (delayval/1000UL)-1; 
    delay_u = delayval - (delay_m*1000UL) - 29;
    }
  else { 
    delay_m = 0; 
    if (delayval < 17) { delayval = 17; }
    delay_u = delayval - 17;
    }
  //  Blink LED
  blinkled(true);
  //
  //  Clear screen the first time labview call is processed.  Also, change clock rate.
  //
  if (LabViewCommand3Active == false){
    //
    // Set ADC to ~ 55kHz sampling rate - default is slooooow
    //
    //sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); 
    //
    //  Set AVCC as the reference voltage for ADC, and enable ADC.
    //  REFS0 = 6.  Thus, 1 << 6 takes 0000 0001 and shifts it to obtain 0100 0000.
    //  ADMUX|0100 0000 returns 1 in every bit position in which both either bit is 1.
    //
    ADMUX = 1<<REFS0;                
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(0<<ADPS1)|(0<<ADPS0);    //enable ADC, ADC frequency=16MB/16=125kHz (set of prescaler)
    ADCSRB = 0x00;
    //
    //  Do not update the screen state if the function gen screen is displayed.
    //
    if (SCREEN_STATE != SC_FNGEN)
    {
      myGLCD.fillScr(VGA_PURPLE);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.setBackColor(VGA_PURPLE);
      myGLCD.print("AnalogReadBurstDiff",10,30);
      myGLCD.print("Channel:",10,50);
      myGLCD.printNumI(channel,90,50);
      myGLCD.print("Rate:",10,60);
      myGLCD.printNumI(rate/10,90,60);
      myGLCD.print("Samples:",10,70);
      myGLCD.printNumI(numSamp,90,70);
    }
    LabViewCommand3Active = true;
  }
  //
  //  Setup sampling
  //
  /* 
  if      (prescale == 32)  { sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); sbi(ADCSRA, ADPS0); } // ADC clk /32
  else if (prescale == 64)  { sbi(ADCSRA, ADPS2); sbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); } // ADC clk /64
  else if (prescale == 128) { sbi(ADCSRA, ADPS2); sbi(ADCSRA, ADPS1); sbi(ADCSRA, ADPS0); } // ADC clk /128 (default)
  else { sbi(ADCSRA, ADPS2); cbi(ADCSRA, ADPS1); cbi(ADCSRA, ADPS0); } // ADC clk /16 (apparently fastest stable)
  signed long value;
	//Loop Over All AI channels In Command Packet
  */
  t0 = micros();
	for(int i=0; i<numSamp*2; i++)
	{
  if (delay_u > 0) { delayMicroseconds(delay_u); }
  if (delay_m > 0) { delay(delay_m); }
  //
  //  Setup Positive differential iinput on ADC0 and Negative Differential Input on ADC1 with Gain 1x.
  //
  //  Set MUX5:0 to 010000.
  //    MUX4 = 4.  Thus, 1 << 4 takes 0000 0001 and shifts it to obtain 0001 0000.
  //    ADMUX|0001 0000 returns 1 in every bit position in which both either bit is 1.
  //    Refer to atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf
  //    for explaination of why this MUX5 is turned on (See Table 26.4)
  //
    ADMUX |= (1<<MUX4);             
    ADCSRA |= (1<<ADSC);            //start conversion
    while (ADCSRA & (1<<ADSC));     //wait until coversion be completed(ADSC=0);
    low = ADCL;
    high = ADCH;
    // The display makes this thing something like 2500 times slower
    // analogValue = (high << 8) | low;
    //
    //  Display voltage on the TFT.
    // 
    // myGLCD.printNumF((float)5.0*analogValue/512.0,2,27,70,'.',6);
        
    response[i] = high;
    i++;
    response[i] = low;
	}
  t = (micros()-t0)/numSamp;
  
	*numResponseBytes = numSamp*2+4;
  
  response[numSamp*2] =   (t & 0xff000000) >> 24;
  response[numSamp*2+1] = (t & 0x00ff0000) >> 16;
  response[numSamp*2+2] = (t & 0x0000ff00) >> 8;
  response[numSamp*2+3] = (t & 0x000000ff);
	return 0;
};
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_PulseGen(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  //
  // Functionality:
  // Converts a float into an int, then into 2 bytes and sends this to Labview, 
  // You can apply a scaling factor before, to choose the required resolution.
  // 
  // Inputs (from Labview): None
  // 
  // Outputs: 
  //      2 bytes containing data of a float.  The first byte is stored in response[0] and the second
  //      byte is stored in response[1]. 
  //  Blink LED
  blinkled(true);
  //
  //  Clear screen the first time labview call is processed.  Also, set
  //  manual ADC conversion
  //
  if (LabViewCommand5Active == false){
    //
    //  Initialize connection to function generator.  By default, after
    //  a reset the output voltage is 1/2 the max voltage.  Frequency 
    //  output is disabled.
    //
    InitializeConnection();
    myGLCD.fillScr(VGA_PURPLE);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setBackColor(VGA_PURPLE);
    myGLCD.print("PULSES GENERATED",15,50);
    LabViewCommand5Active = true;
  }
  unsigned long frequency = 0; // clear it out

  frequency = ((uint32_t)input[0]<<24) | ((uint32_t)input[1]<<16) | ((uint32_t)input[2]<<8) | ((uint32_t)input[3]);

  objSettings.setMode('Q');
  objSettings.setFrequency(frequency);
  //objSettings.setFrequency(1000);
  objSettings.setOffset(objSettings.getZeroOffset());
  objSettings.setGain(objSettings.get5VGain());
  //
  //  Generate the pulse train.
  //
  ProcessCommand();
  //
  //  Set number of bytes returned.
  //
  *numResponseBytes = 1;
 
  response[0] = 1;

  return 0;
};
//
//  Interrupt for processing counter 5 overflows.
//
ISR (TIMER5_OVF_vect)
{
  ++overflowCount;               // count number of Counter1 overflows  
}  // end of TIMER5_OVF_vect
//
//  Delay that does not require Timer0 to be enabled.
//
void ntDelay(byte t){    // non timer delay in seconds
  for (int i = 0; i < t*1000; i++) {
      delayMicroseconds(1000);
      }
}
// %%%%%%   Custom command definition    %%%%%%%%%
// The function must return an int and must have 4 parameters: unsigned char, unsigned char*, unsigned char*, unsigned char*. 
// The parameters are described below:
//
// numInputBytes:     the number of bytes in the input array (from LabVIEW)
// input:             a U8 array which contains the data bytes sent from LabVIEW using the Custom Command VI.
// numResponseBytes:  set to the number of bytes you want to send back to LabVIEW.
// response:          is a U8 array. Fill this with the data bytes you want to send back to LabVIEW.
//
// The function should return 0 (or use the L_OK constant) if everything went well, or an error number otherwise. 
int readout_PulseCount(unsigned char numInputBytes, unsigned char* input, unsigned char* numResponseBytes, unsigned char* response)
{
  //
  // Functionality:
  // Converts a float into an int, then into 2 bytes and sends this to Labview,
  // You can apply a scaling factor before, to choose the required resolution.
  //
  // Inputs (from Labview): None
  //
  // Outputs:
  //      2 bytes containing data of a float.  The first byte is stored in response[0] and the second
  //      byte is stored in response[1].
  //
  //  Clear screen the first time labview call is processed.  Also, set
  //  manual ADC conversion
  //
  //
  //  Initializer the counter to 0.
  //
  unsigned long lCounter;
  unsigned int timer5CounterValue;
  unsigned long frequency = 0; // clear it out
  //
  //  if mode = 1, then count, no generate.
  //  if mode = 2, then frequency, no generate
  //  if mode = 3, then count with generate
  //  if mode = 4, then frequency with generate
  //
  unsigned char mode = input[0];
  unsigned char period = input[1]; //period in seconds
  //mode = 3;
  //period = 1;  // 5 second period
  //  Blink LED
  blinkled(true);
  //
  //  For modes 3 nd 4 generate the requested pulse train.
  if (mode == 3 || mode == 4)
  {
    //
    //
    // Code to generate pulse train
    //
    InitializeConnection();

    frequency = ((uint32_t)input[2] << 24) | ((uint32_t)input[3] << 16) | ((uint32_t)input[4] << 8) | ((uint32_t)input[5]);
    //frequency = 1000;
    objSettings.setMode('Q');
    objSettings.setFrequency(frequency);
    //
    //  Generate the pulse train.
    //
    ProcessCommand();
  }
  //Serial.println(overflowCount);
  // reset Timer 5
  TCCR5A = 0;
  TCCR5B = 0;
  //
  // The Timer Interrupt Mask Register (TIMSK) controls which interrupts are turned on.
  // TOIE5 = 0 -> bit(0) = 1 = 0000 0001 Turns on Timer 5 overlow interrupt.
  //
  TIMSK5 = bit(TOIE5);
  TCNT5 = 0; // Initialize Timer
  //  Reset the over flow counter
  overflowCount = 0;
  //Serial.println(overflowCount);
  //
  // bit(n) computes the value of the specified bit n (bit 0 is 1, bit 1 is 2, bit 2 is 4, etc.).
  // CS50 = 0 -> bit(0) = 1 -> 0000 0001
  // CS51 = 1 -> bit(1) = 2 -> 0000 0010
  // CS52 = 2 -> bit(2) = 4 -> 0000 0100
  //
  // Thus 0000 00001 | 0000 0010 | 0000 0100 = 00000111
  // From the literature this means increment timer 4 on Pin 47 on rising edge
  //
  TCCR5B = bit(CS50) | bit(CS51) | bit(CS52);
  //Serial.println(overflowCount);
  //
  //  Capture the time counting starts.
  //
  unsigned long start = millis();
  unsigned long end = millis();
  if (mode == 1 || mode == 3)
  {
    while (end - start < period * 1000)
    {
      end = millis();
      if (digitalRead(buttonPin2) == HIGH)
      {
        break;
      }
    }
  }
  else
  {
    while (end - start < period * 1000)
    {
      end = millis();
    }
  }
  //Serial.println(overflowCount);
  //ntDelay(1);
  // Stop counter.
  TCCR5B = 0;
  TCCR5A = 0; // stop timer 5
  TCCR5B = 0;
  TIMSK5 = 0;                 // disable Timer5 Interrupt
  timer5CounterValue = TCNT5; // see datasheet, (accessing 16-bit registers)
      // calculate total count
  if (overflowCount == 0)
  {
    lCounter = timer5CounterValue;
  }
  else
  {
    lCounter = ((overflowCount) << 16) + timer5CounterValue; // each overflow is 65536 more
  }
  //
  //  Display results to screen if appropriate.
  //
  if (LabViewCommand6Active == false)
  {
    //
    //  Do not update the screen state if the function gen screen is displayed.
    //
    if (SCREEN_STATE != SC_FNGEN)
    {
      myGLCD.fillScr(VGA_PURPLE);
      myGLCD.setFont(SmallFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.setBackColor(VGA_PURPLE);
      if (mode == 1 || mode == 3)
      {
        myGLCD.print("  PULSE COUNT  ", 45, 50);
        myGLCD.printNumI(lCounter, 50, 60, 8);
      }
      else if (mode == 2 || mode == 4)
      {
        myGLCD.print("  FREQUENCY  ", 50, 50);
        myGLCD.printNumF((float)1000.0 * lCounter / (end - start), 0, 50, 60, '.', 8);
        // Return frequency as an integer
        if (mode == 4)
        {
          lCounter = (float)1000.0 * lCounter / (end - start);
        }
      }
    }
    LabViewCommand6Active = true;
  }
  else
  {
    //
    //  Do not update the screen state if the function gen screen is displayed.
    //
    if (SCREEN_STATE != SC_FNGEN)
    {
      if (mode == 1 || mode == 3)
      {
        myGLCD.print("PULSE COUNT", 45, 50);
        myGLCD.printNumI(lCounter, 50, 60, 8);
      }
      else if (mode == 2 || mode == 4)
      {
        myGLCD.print("FREQUENCY", 50, 50);
        myGLCD.printNumF((float)1000.0 * lCounter / (end - start), 0, 50, 60, '.', 8);
      }
    }
  }
  // Return frequency as an integer in mode 4
  if (mode == 2 || mode == 4)
  {
    lCounter = (float)1000.0 * lCounter / (end - start);
  }
  //
  // Set number of bytes to send back to LabVIEW
  //
  *numResponseBytes = 4;
  //
  //  Setup data to send back to LabVIEW
  //
  // Convert 0xFF000000 ==>   F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 1111 1111 0000 0000 0000 0000 0000 0000  -> This is a 4 byte number and so is the variable value.
  //  The expression (value & 0xFF000000)  >> 24 masks out the lower 3 bytes of the number stored in value and then
  //  bit shits all of the bits to right by 24.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[0] = (lCounter & 0xFFFF0000) >> 24;
  //
  //  Setup data to send back to LabVIEW
  //
  // Convert 0x00FF0000 ==>   0 - 0000
  //                          0 - 0000
  //                          F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 0000 0000 1111 1111 0000 0000 0000 0000  -> This is a 4 byte number and so is the variable value.
  //  The expression (value & 0xFF0000)  >> 16 masks out the lower two bytes of the number stored in value and then
  //  bit shits all of the bits to right by 16.  This returns the most significant byte of the the number
  //  stored in value and stores it in response[0]
  //
  response[1] = (lCounter & 0xFF0000) >> 16;
  // Convert 0x0000FF00 ==>   0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          F - 1111
  //                          F - 1111
  //                          0 - 0000
  //                          0 - 0000
  //
  //  Results in Binary of 0000 0000 0000 0000 1111 1111 0000 0000  -> This is a 4 byte number and so is the variable value.
  //  The expression (value & 0x0000FF00)  >> 8 masks out the lower byte of the number stored in value and then
  //  bit shits all of the bits to right by 8.
  //
  response[2] = (lCounter & 0x00FF00) >> 8; //mask top bits and bottom bits, shift 8
  //
  // Convert 0x000000FF ==>   0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          0 - 0000
  //                          F - 1111
  //                          F - 1111
  //
  //  Results in Binary of 0000 0000 0000 0000 1111 1111 -> This is a 4 byte number and so is the variable value.
  //  The expression (value & 0x00FF) masks out the upper bytes of the number stored in value.  This returns
  //  the least significant byte of the the number stored in value and stoes it in response[3]
  //
  response[3] = (lCounter & 0x0000FF); //mask top bits, no need to shift.

  return 0;
};
