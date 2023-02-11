#include <Arduino.h>

/* GENERAL SETTINGS ************************************************************************************************
 *  Have a look at the serial monitor in order to check the current settings!
 *  The required login informations for the browser based configuration via 192.186.4.1 can also be found there.
 */

// Display settings ----------------------------------------------------------------------------------------------
//#define OLED1306 //A 0.96" Display is selected, if defined. Otherwise a 1.3" display

// WiFi settings -------------------------------------------------------------------------------------------------
const char* ssid = "Servotester_Deluxe";  // SSID
const char* password = "123456789";       // Password

// Power settings ------------------------------------------------------------------------------------------------
// Don't use it, servos will not work!!
//#define LOW_POWER_MODE // Uncommenting this will lower the clock to 80MHz and will increase battery life
