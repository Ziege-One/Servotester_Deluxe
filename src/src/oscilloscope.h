/*
  A handy oscilloscope for 0 - 3.3V RC signals
*/

#include "Arduino.h"

/*
 ###################################################################
 Title:       Arduino Oscilloscope
 Purpose:     Made for the Servotester Deluxe
 Created by:  TheDIYGuy999
 Note:        Please reuse, repurpose, and redistribute this code.
 Note:        ESP32 & 1106 OLED
 ###################################################################
 */

//
// ========================================
// GLOBAL VARIABLES
// ========================================
//

int displayWidth = 128;

int timeBase = 0;      // define a variable for the x delay (90 for 50 Hz, 10 for 333, 0 for 1520)
int samplingDelay = 0; // define a variable for the X scale / delay
int sampleNo = 0;      // define a variable for the sample number used to collect x position into array
int sampleHi = 0;      // Samples, which are above trigger level
int posX = 0;          // define a variable for the sample number used to write x position on the LCD

// array parameters
#define arraySize displayWidth // how much samples are in the array
int myArray[128];              // define an array to hold the data coming in
int arrayMin = 0;
int arrayAverage = 0;
int arrayMax = 0;
Array<int> samplingArray = Array<int>(myArray, arraySize);

// OLED scaling and offset
int OledScaleX = 1;
int scaleY = 85;        // define a variable for the y scale (85) adcmax/scaleY = bottom end
int scopeOffsetY = -13; // shifted 13 pixels downwards to make room for text

// Voltage measuring
float voltMax = 3.3;
int adcMax = 4095; // the maximum reading from the ADC 1023, but 4095 on ESP32!
float vPP = 0;     // peak to peak voltage

// Frequency & pulsewidth measuring
unsigned long startSampleMicros = 0;
unsigned long endSampleMicros = 0;
unsigned long oneSampleDuration = 0;
int signalFrequency = 0;
int pulseWidth = 0;

// trigger
int triggerLevel = 2048;
byte triggerMode = 0;

// Display
unsigned long popupMillis;

// menu
byte menu = 1; // the current menu item

//
// ========================================
// ADSJUST ADC TIMEBASE
// ========================================
//
void adjustADC()
{

  if (encoderState == 1)
  {
    samplingDelay += 4;
    popupMillis = millis();
  }
  if (encoderState == 2)
  {
    samplingDelay -= 4;
    popupMillis = millis();
  }

  samplingDelay = constrain(samplingDelay, 0, 332); // 332 for 50Hz
}

//
// ========================================
// READ PROBE
// ========================================
//

void readProbe()
{

  // wait until threshold hits trigger level
  sampleNo = 0;
  while ((analogRead(OSCILLOSCOPE_PIN) < triggerLevel) && (sampleNo < arraySize))
    sampleNo++; // proceed after reaching array end, if no trigger level was detected!
  sampleNo = 0;
  while ((analogRead(OSCILLOSCOPE_PIN) > triggerLevel) && (sampleNo < arraySize))
    sampleNo++;

  // fill the array as fast as possible with samples (therefore we have as less code as possible in this for - loop)

  // read samples and store them in array ----------------------------
  portDISABLE_INTERRUPTS();
  startSampleMicros = micros();
  sampleNo = 0;
  while (sampleNo < arraySize)
  {
    myArray[sampleNo] = analogRead(OSCILLOSCOPE_PIN); // read the value from the sensor:
    sampleNo++;
    delayMicroseconds(samplingDelay);
  }
  endSampleMicros = micros();
  portENABLE_INTERRUPTS();

  // Calculate the required time for taking one sample
  oneSampleDuration = (endSampleMicros - startSampleMicros) / arraySize;

  // calculate signal frequency using trigger level crossing
  sampleNo = 0;
  sampleHi = 0;
  while (myArray[sampleNo] < triggerLevel && (sampleNo < (arraySize - 1)))
    sampleNo++;
  while (myArray[sampleNo] > triggerLevel && sampleNo < (arraySize - 1))
  {
    sampleNo++;
    sampleHi++; // Samples above zero crossing for pulsewidth calculation
  }
  if (sampleHi > 0)
  {
    signalFrequency = (1000000L / (sampleNo * oneSampleDuration)); // Hz = full waves per one million microseconds
  }
  else
  {
    signalFrequency = 0;
  }
  pulseWidth = sampleHi * oneSampleDuration; // Pulsewidth calculation

  // invert the whole array
  for (sampleNo = 0; sampleNo < arraySize; sampleNo++)
  {
    myArray[sampleNo] = (adcMax - myArray[sampleNo]);
  }

  // compute array min, average, max values
  arrayMin = samplingArray.getMin();
  arrayMax = samplingArray.getMax();
  arrayAverage = samplingArray.getAverage();
  vPP = (arrayMax - arrayMin) * voltMax / adcMax;
}

//
// ========================================
// DRAW LCD
// ========================================
//

void drawDisplay()
{

  static unsigned long previousUpdate;

  if (millis() - previousUpdate >= 250)
  { // every 250 ms
    previousUpdate = millis();

    // clear old display content
    display.clear();

    // draw reading line
    for (posX = 0; posX < displayWidth; posX += 1)
    { // for loop draws displayWidth pixels
      display.drawLine((posX * OledScaleX) - (OledScaleX - 1), myArray[posX - 1] / scaleY - scopeOffsetY, posX * OledScaleX, myArray[posX] / scaleY - scopeOffsetY);
    }

    // draw reference lines
    for (sampleNo = 0; sampleNo < displayWidth; sampleNo += 4)
    {
      display.setPixel(sampleNo, (adcMax / scaleY) - scopeOffsetY);     // Draw 5V Line
      display.setPixel(sampleNo, (adcMax / scaleY / 2) - scopeOffsetY); // Draw 2,5V Line
      display.setPixel(sampleNo, 0 - scopeOffsetY);                     // Draw 0V Line
    }

    // draw array min, average, max lines
    for (sampleNo = 0; sampleNo < displayWidth; sampleNo += 10)
    {
      // display.setPixel(sampleNo, (arrayMin / scaleY) - scopeOffsetY);      // Draw min Line
      display.setPixel(sampleNo, (arrayAverage / scaleY) - scopeOffsetY); // Draw average Line
      // display.setPixel(sampleNo, (arrayMax / scaleY) - scopeOffsetY);      // Draw max Line

      // draw trigger level marking
      display.drawLine(2, (adcMax - triggerLevel) / scaleY - scopeOffsetY, 4, (adcMax - triggerLevel) / scaleY - scopeOffsetY);
    }

    // Readings on top of display
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(displayWidth, 0, String(vPP) + "vPP"); // Show peak to peak voltage

    if (triggerLevel > 0)
    { // if trigger is active
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(displayWidth / 2, 0, String(signalFrequency) + "Hz"); // Show signal frequency
    }

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, String(pulseWidth) + "µs"); // Show pulsewidth

    // Popup window
    if (millis() - popupMillis < 1500)
    {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setColor(BLACK);
      display.fillRect(25, 22, 78, 29); // Clear area behind window
      display.setColor(WHITE);
      display.drawRect(25, 22, 78, 29); // Draw window frame
      display.drawString(64, 25, "Sampling delay");
      display.drawString(64, 35, String(samplingDelay) + " µs"); // Show sampling delay
    }

    // refresh display content
    display.display();
  }
}

//
// ========================================
// MAIN OSCILLOSCOPE LOOP
// ========================================
//

void oscilloscopeLoop()
{

  adjustADC();

  readProbe();

  drawDisplay();
}
