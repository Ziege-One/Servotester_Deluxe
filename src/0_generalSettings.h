#include <Arduino.h>
#include <WiFi.h>

/* GENERAL SETTINGS ************************************************************************************************
 *  Have a look at the serial monitor in order to check the current settings!
 *  The required login informations for the browser based configuration via 192.186.4.1 can also be found there.
 */

// Display settings ----------------------------------------------------------------------------------------------
//#define OLED1306 //An SSD1306 0.96" Display is selected, if defined. Otherwise an SH1106 1.3" display
//#define ALTERNATIVE_LOGO // Alternative boot logo

// WiFi settings -------------------------------------------------------------------------------------------------
const char* ssid = "Servotester_Deluxe";  // SSID
const char* password = "123456789";       // Password

/* Wifi transmission power: less power = longer battery life. Valid options are:
WIFI_POWER_19_5dBm = 78     // full power
WIFI_POWER_19dBm = 76
WIFI_POWER_18_5dBm = 74
WIFI_POWER_17dBm = 68
WIFI_POWER_15dBm = 60
WIFI_POWER_13dBm = 52       // recommended setting
WIFI_POWER_11dBm = 44
WIFI_POWER_8_5dBm = 34
WIFI_POWER_7dBm = 28         
WIFI_POWER_5dBm = 20
WIFI_POWER_2dBm = 8          // lowest setting, WiFi may become weak
*/ 
wifi_power_t cpType = WIFI_POWER_13dBm; // Only use values from above!

// ADC settings -------------------------------------------------------------------------------------------------
// See: https://github.com/e-tinkers/esp32-adc-calibrate
#define ADC_LINEARITY_COMPENSATION // Use the lookup table in adcLookup.h in order to improve the oscilloscope linearity
#define FAST_ADC // ADC is about twice as fast, if defined, but scope will crash, if WiFi is disabled! Further investigation needed.
