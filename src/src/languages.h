#include <Arduino.h>
// Language ressources strings for OLED menus

int noOfLanguages = 2; // 0 = English, 1 = Deutsch, 2 = Francais

// Einstellung
String languagesString[] {"English", "Deutsch", "Français"};
String settingsString[]{"Settings", "Einstellung", "Paramètres"};
String onString[]{"On", "Ein", "activé"};
String offString[]{"Off", "Aus", "désactivé"};
String servoStepsString[] {"Servo Steps µs", "Servo Schritte µs", "étape servo µs"};
String servoMaxString[] {"Servo Max. µs", "Servo Max. µs", "Servo Max. µs"};
String servoMinString[] {"Servo Min. µs", "Servo Min. µs", "Servo Min. µs"};
String servoCenterString[] {"Servo Center µs", "Servo Mitte µs", "Servo centre µs"};
String servoHzString[] {"Servo Hz", "Servo Hz", "Servo Hz"};
String PowerScaleString[] {"POWER Scale", "POWER Skala", "échelle POWER"};
String inversedString[] {"Inversed", "Invertiert", "Inversé"};
String standardString[] {"Standard", "Standart", "Défaut"};
String encoderDirectionString[] {"Encoder direction", "Encoder Richtung", "Encodeur direct."};
String languageString[] {"Language", "Sprache", "Langue"};
String pongBallRateString[] {"Pong ball speed", "Pong Ball Gesch.", "Pong V. de balle"};

// Impuls lesen
String impulseString[] {"Impulse", "Impuls", "Impulsion"};

// Automatik
String delayString[] {"Delay", "Verz.", "Ret."};

// Auswahl
String servotesterString[] {"Servotester", "Servotester", "Testeur de servos"};
String readIbusString[] {"Read IBUS", "IBUS lesen", "Lire IBUS"};
String readSbusString[] {"Read SBUS", "SBUS lesen", "Lire SBUS"};
String readCh1String[] {"read CH1", "lesen CH1", "lire CH1"};
String readCh1Ch5String[] {"read CH1 - 5", "lesen CH1 - 5", "lire CH1 - 5"};
String PwmImpulseString[] {"PWM Impulse", "PWM Impuls", "PWM Impulsion"};
String automaticModeString[] {"Automatic Mode", "Automatik Modus","Mode automat."};
String oscillateServoString[] {"(Oscillate Servo)", "(Servo pendeln)", "(Osciller Servo)"};

// Setup
String passwordString[] {"Password:", "Passwort:", "Mot de passe:"};
String ipAddressString[] {"IP address:", "IP Adresse:", "IP Adresse:"};
String WiFiOnString[] {"WiFi On", "WiFi Ein", "WiFi activé"};
String WiFiOffString[] {"WiFi Off", "WiFi Aus", "WiFi desactivé"};
String apIpAddressString[] {"AP-IP-Adress: ", "AP-IP-Adresse: ", "AP-IP-Adresse: "};
String connectingAccessPointString[] {"AP (Accesspoint) connecting…", "AP (Zugangspunkt) einstellen…", "connecter le point d'accès…"};

// Instructions
String operationString[] {"******** Operation: ********", "******** Bedienung: ********", "****** Mode d'emploi : ******"};
String shortPressString[] {"Short Press = Select", "Kurz Drücken = Auswahl", "Appui brièvem. = Sélection"};
String longPressString[] {"Long Press = Back", "Lang Drücken = Zurück", "Appui long   = Retour"};
String doubleclickString[] {"Doubleclick = CH Change", "Doppelklick = CH Wechsel", "Double-clique = CH Changem."};
String RotateKnobString[] {"Rotate = Scroll / Adjust", "Drehen = Blättern / Einstell.", "Tourner     = Défiler"};

// EEPROM
String eepromReadString[] {"EEPROM read.", "EEPROM gelesen.", "EEPROM lire."};
String eepromWrittenString[] {"EEPROM written.", "EEPROM geschrieben.", "EEPROM écrit."};
String eepromInitString[] {"EEPROM initialized.", "EEPROM initialisiert.", "EEPROM initialisé."};

// Battery
String eepromVoltageString[] {"Battery voltage: ", "Akkuspannung: ", "Voltage de batterie: "};
String noBatteryString[] {"No Battery connected", "Kein Akku angeschlossen", "Pas de batterie connectée"};
