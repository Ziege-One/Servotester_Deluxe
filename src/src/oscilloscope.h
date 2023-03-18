/*
  A handy oscilloscope for 0 - 3.3V RC signals
*/

#include "Arduino.h"
#include "adcLookup.h"

//
// ========================================
// GLOBAL VARIABLES
// ========================================
//

const int displayWidth = 128;

const int oneSampleCalibration = 71; // Allows to fine adjust the pulsewith & frequency readings (About 71 µs)

int timeBase = 0;      // define a variable for the x delay (90 for 50 Hz, 10 for 333, 0 for 1520)
int samplingDelay = 0; // define a variable for the X scale / delay
int sampleNo = 0;      // define a variable for the sample number used to collect x position into array
int sampleHi = 0;      // Samples, which are above trigger level
int posX = 0;          // define a variable for the sample number used to write x position on the LCD

// array parameters
#define arraySize displayWidth // how much samples are in the array
int myArray[arraySize];        // define an array to hold the data coming in
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
float signalFrequency1 = 0;
float signalFrequency = 0;
uint32_t pulseWidth1 = 0;
uint32_t pulseWidth = 0;
uint32_t averagingPasses = 10;

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
    samplingDelay -= 4;
    popupMillis = millis();
  }
  if (encoderState == 2)
  {
    samplingDelay += 4;
    popupMillis = millis();
  }

  if (buttonState == 2)
    popupMillis = millis(); // Show popup, if button clicked

  samplingDelay = constrain(samplingDelay, 0, 300); // 160 for 50Hz
}

//
// ========================================
// READ PROBE
// ========================================
//

void readProbe()
{
  portDISABLE_INTERRUPTS();
  // wait until threshold hits trigger level -------------------------------------------------
  sampleNo = 0;
  while ((local_adc1_read(ADC1_CHANNEL_4) < triggerLevel) && (sampleNo < 2500)) // We have to wait long enough for 50Hz or PPM, so longer than arraySize
    sampleNo++;                                                                 // proceed after reaching array end, if no trigger level was detected!
  sampleNo = 0;
  while ((local_adc1_read(ADC1_CHANNEL_4) > triggerLevel) && (sampleNo < 2500)) // We have to wait long enough for 50Hz or PPM, so longer than arraySize
    sampleNo++;

  // fill the array as fast as possible with samples (therefore we have as less code as possible in this for - loop)

  // read samples and store them in array ---------------------------------------------------
  startSampleMicros = esp_timer_get_time();
  sampleNo = 0;
  while (sampleNo < arraySize)
  {
    myArray[sampleNo] = local_adc1_read(ADC1_CHANNEL_4); // Super fast custom analogRead() alterantive
    sampleNo++;
    int64_t m = esp_timer_get_time(); // slim replacement vor delayMicroseconds()
    while (esp_timer_get_time() < m + samplingDelay)
    {
      NOP();
    }
  }
  endSampleMicros = esp_timer_get_time();
  portENABLE_INTERRUPTS();

  // Calculate the required time for taking one sample -------------------------------------
  oneSampleDuration = ((endSampleMicros + oneSampleCalibration - startSampleMicros) / arraySize);

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
    signalFrequency1 = (1000000L / (sampleNo * oneSampleDuration)); // Hz = full waves per one million microseconds
    signalFrequency = (signalFrequency * (averagingPasses - 1) + signalFrequency1) / averagingPasses;
  }
  else
  {
    signalFrequency = 0;
  }
  pulseWidth1 = sampleHi * oneSampleDuration; // Pulsewidth calculation
  pulseWidth = (pulseWidth * (averagingPasses - 1) + pulseWidth1) / averagingPasses;

  // Calibrate & invert the whole array -----------------------------------------------------
  for (sampleNo = 0; sampleNo < arraySize; sampleNo++)
  {
#if defined ADC_LINEARITY_COMPENSATION
    myArray[sampleNo] = (int)ADC_LUT[myArray[sampleNo]]; // get the calibrated value from Lookup table
#endif
    myArray[sampleNo] = (adcMax - myArray[sampleNo]);
  }

  // compute array min, average, max values ------------------------------------------------
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

  if (millis() - previousUpdate >= 200)
  { // every 200 ms
    previousUpdate = millis();

    // clear old display contentmt
    display.clear();

    // draw reading line
    for (posX = 1; posX < displayWidth; posX += 1)  // looks better, if posX = 0 is not displayed!
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
      display.drawString(displayWidth / 2, 0, String(signalFrequency, 0) + "Hz"); // Show signal frequency
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

void oscilloscopeLoop(bool init)
{
  if (init)
  {
    samplingDelay = 160;    // Set ideal delay for standard RC Signals (132 for analogRead, 160 for adc1_get_raw)
    popupMillis = millis(); // Show popup
  }

  adjustADC();

  readProbe();

  drawDisplay();
}
