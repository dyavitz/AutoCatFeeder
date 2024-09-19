////////////////////////////////////////////////////////////////////////////////
//  USER CHANGEABLE VARIABLES
////////////////////////////////////////////////////////////////////////////////

// Used for Hall sensor fail backup function
#define HALL_SENSOR_BACKUP_VALUE 300  // Interval should be larger than one 60-degree turn of the feeder (one portion)

// Delay before stopping the feeder servo after feeding
#define FEEDER_STOP_DELAY 100  // Adjust for peddle wheel stop position

// LCD backlight on time after pressing a button (in ms)
#define LCD_BACKLIGHT_ON_TIME 30000

// Exit menu without saving after the time specified (in ms)
#define MENU_TIMEOUT_VALUE 7000

////////////////////////////////////////////////////////////////////////////////

// Include required libraries
#include <LiquidCrystal_I2C.h>    
#include <Button.h>               
#include <DS3232RTC.h>            
#include <Time.h>                
#include <Wire.h>                 
#include <EEPROM.h>

// CONNECTIONS:
//
// LCD (I2C module):
// SCL - A5
// SDA - A4
// VCC
// GND

// Hall sensor and button pins
#define HALL_SENSOR_PIN  3
#define BUTTON_BACK_PIN  4
#define BUTTON_UP_PIN    5
#define BUTTON_DOWN_PIN  6
#define BUTTON_SELECT_PIN  7
#define BUTTON_CANCEL1_PIN 8
#define BUTTON_CANCEL2_PIN 9
#define BUTTON_MANUAL_PIN  10

// LED Pins for indicators
#define LED_CANCEL1_PIN  A0
#define LED_CANCEL2_PIN  A1
#define LED_SUCCESS1_PIN A2
#define LED_SUCCESS2_PIN A3

// Feeder servo output pin
#define SERVO_OUTPUT_PIN 12

// Hall sensor input pin
#define LED_HALL_SENSOR_PIN 13

// LCD address (16x2 display), adjust address as needed
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// Button configuration
#define DEBOUNCE_MS 20
#define REPEAT_FIRST 1000
#define REPEAT_INCR 200
#define PULLUP true
#define INVERT true

// Button declarations
Button buttonSelect(BUTTON_SELECT_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonUp(BUTTON_UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonDown(BUTTON_DOWN_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonBack(BUTTON_BACK_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonCancel1(BUTTON_CANCEL1_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonCancel2(BUTTON_CANCEL2_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button buttonManual(BUTTON_MANUAL_PIN, PULLUP, INVERT, DEBOUNCE_MS);

// Variables for feeding portions, time settings, and manual overrides
int count, lastCount = -1;
unsigned long rpt = REPEAT_FIRST;
unsigned long timeoutValue = 0;
bool manualCancelFeed1 = false, manualCancelFeed2 = false;
bool manualFeed = false;
int feedAmount1 = 1, feedAmount2 = 1;
int portions = 0, turns = 0;
bool feederSuccess = false;

// System states and user inputs
enum STATES { MAIN, MENU_EDIT_FEEDTIME1, MENU_EDIT_FEEDTIME2, MENU_EDIT_FEEDAMOUNT, MENU_EDIT_TIME, MENU_EDIT_DATE, MENU_EDIT_SETTINGS };
enum INPUTS { btnSELECT, btnUP, btnDOWN, btnBACK, btnCANCEL1, btnCANCEL2, btnMANUAL, trigTIMEOUT };
STATES state;
int8_t userInput, trigger;
int Second, Minute, Hour, Day, Month, Year;
int8_t DoW;
String day_of_week;
unsigned char address, data;

// Time and feeding schedule
int feed_time1_hour, feed_time1_minute;
bool feed_time1_active = false, alarm1Activated = false;
int feed_time2_hour, feed_time2_minute;
bool feed_time2_active = false, alarm2Activated = false;

// Blink and spinning wheel timing
uint32_t blink_interval = 500, blink_previousMillis = 0, blink_currentMillis = 0;
boolean blink_state = false;
uint32_t spinningWheel_interval = 170, spinningWheel_previousMillis = 0, spinningWheel_currentMillis = 0;
int spinningWheelSymbol = 0;
uint32_t hallSensorBackup_interval = HALL_SENSOR_BACKUP_VALUE, hallSensorBackup_currentMillis = 0;
boolean hallSensorFail = false;
uint32_t lcdBacklight_interval = LCD_BACKLIGHT_ON_TIME, lcdBacklight_currentMillis = 0;

// RTC error flag
boolean RTC_error = true, long_press_button = false;

// Custom LCD characters
byte bell_symbol_Char[8] = { B00100, B01110, B01110, B01110, B11111, B00100, B00000, B00000 };
byte arrow_up_Char[8] = { B00100, B01110, B11111, B01110, B01110, B01110, B01110, B00000 };
byte arrow_down_Char[8] = { B00000, B01110, B01110, B01110, B01110, B11111, B01110, B00100 };
byte thickDash_Char[8] = { 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b00000, 0b00000, 0b00000 };

// Hall sensor flag
volatile boolean hallSensorActivated = false;

// Interrupt handler for the Hall sensor
void HallSensorIsr() {
  hallSensorActivated = true;
  digitalWrite(LED_HALL_SENSOR_PIN, HIGH);  // Turn on Hall sensor LED
}

// Function declarations
void setup();
void loop();
void change_states();
void check_inputs();
void transition(int trigger);
void check_alarm();
void check_manual_feed();
void display_menu_option_set_feedtime1();
void display_menu_option_set_feedtime2();
void display_menu_option_set_feed_amount();
void display_menu_option_set_time();
void display_menu_option_set_date();
void midnight_reset();
void display_time();
void displayFeedingAmounts();
void displayFeedingTimes();
void set_feedAmount();
void set_time();
void set_date();
void set_feeding1_time();
void set_feeding2_time();
void get_time();
void get_date();
void write_time();
void write_date();
void write_feeding_time1();
void write_feeding_time2();
void write_feedamount();
void get_feedamount();
void get_feed_time1();
void get_feed_time2();
void check_RTC();
byte decToBcd(byte val);
byte bcdToDec(byte val);
void leading_zero(int digits);
void blinkFunction();
void displaySpinningWheel();
void startFeederServo();
void stopFeederServo();
void activateFeeder(int portions);
void check_LcdBacklight();
void lcd_backlight_ON();
void ledsAndLcdDisplayStartup();
void hallSensorCheck();

void setup() {
  // LCD Initialization
  lcd.begin();
  lcd_backlight_ON();
  Wire.begin();
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_HALL_SENSOR_PIN, OUTPUT);
  pinMode(SERVO_OUTPUT_PIN, OUTPUT);
  pinMode(LED_CANCEL1_PIN, OUTPUT);
  pinMode(LED_CANCEL2_PIN, OUTPUT);
  pinMode(LED_SUCCESS1_PIN, OUTPUT);
  pinMode(LED_SUCCESS2_PIN, OUTPUT);

  // Turn off all LEDs initially
  digitalWrite(LED_CANCEL1_PIN, LOW);
  digitalWrite(LED_CANCEL2_PIN, LOW);
  digitalWrite(LED_SUCCESS1_PIN, LOW);
  digitalWrite(LED_SUCCESS2_PIN, LOW);
  digitalWrite(LED_HALL_SENSOR_PIN, LOW);

  // Load custom characters into the LCD
  lcd.createChar(0, thickDash_Char);
  lcd.createChar(1, bell_symbol_Char);
  lcd.createChar(6, arrow_up_Char);
  lcd.createChar(7, arrow_down_Char);

  // Initialize feeder servo and attach interrupts
  stopFeederServo();
  setSyncProvider(RTC.get);
  setSyncInterval(60);
  RTC.squareWave(SQWAVE_NONE);
  attachInterrupt(INT1, HallSensorIsr, FALLING);

  ledsAndLcdDisplayStartup();
  state = MAIN;
  get_feed_time1();
  get_feed_time2();
}

void loop() {
  change_states();
  check_inputs();
  check_alarm();
  check_manual_feed();
  midnight_reset();
  check_RTC();
  hallSensorCheck();
  check_LcdBacklight();
}

void change_states() {
  switch (state) {
    case MAIN:
      display_time();
      displayFeedingAmounts();
      displayFeedingTimes();
      break;
    case MENU_EDIT_FEEDTIME1:
      display_menu_option_set_feedtime1();
      break;
    case MENU_EDIT_FEEDTIME2:
      display_menu_option_set_feedtime2();
      break;
    case MENU_EDIT_FEEDAMOUNT:
      display_menu_option_set_feed_amount();
      break;
    case MENU_EDIT_TIME:
      display_menu_option_set_time();
      break;
    case MENU_EDIT_DATE:
      display_menu_option_set_date();
      break;
    default:
      break;
  }
}
//*******************************************************************************
// Change states based on inputs and timeouts
void check_inputs() {
  // First, check if the menu timeout has occurred
  if (millis() - timeoutValue > MENU_TIMEOUT_VALUE) {
    userInput = trigTIMEOUT;
    transition(userInput);
  }

  // Read button states
  buttonSelect.read();
  buttonUp.read();
  buttonDown.read();
  buttonBack.read();
  buttonManual.read();
  buttonCancel1.read();
  buttonCancel2.read();

  // Manual cancel Feed1 button
  if (buttonCancel1.wasPressed()) {
    manualCancelFeed1 = !manualCancelFeed1; // Toggle cancel status
    lcd_backlight_ON(); // Turn on the LCD backlight

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(manualCancelFeed1 ? "Feed #1 cancelled" : "Feed #1 restored");
    delay(2000);
    lcd.clear();
  }

  // Manual cancel Feed2 button
  if (buttonCancel2.wasPressed()) {
    manualCancelFeed2 = !manualCancelFeed2;
    lcd_backlight_ON(); // Turn on the LCD backlight

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(manualCancelFeed2 ? "Feed #2 cancelled" : "Feed #2 restored");
    delay(2000);
    lcd.clear();
  }

  // Manual Feed button
  if (buttonManual.wasPressed()) {
    manualFeed = true;
    lcd_backlight_ON(); // Turn on the LCD backlight
  }

  // Select button
  if (buttonSelect.wasPressed()) {
    userInput = btnSELECT;
    lcd_backlight_ON();
    transition(userInput);
  }

  // Up button
  if (buttonUp.wasPressed()) {
    userInput = btnUP;
    transition(userInput);
    lcd_backlight_ON();
  }

  // Long press UP button
  if (buttonUp.pressedFor(rpt)) {
    rpt += REPEAT_INCR;
    long_press_button = true;
    userInput = btnUP;
    transition(userInput);
  }

  // Down button
  if (buttonDown.wasPressed()) {
    userInput = btnDOWN;
    transition(userInput);
    lcd_backlight_ON();
  }

  // Back button
  if (buttonBack.wasPressed()) {
    userInput = btnBACK;
    transition(userInput);
    lcd_backlight_ON();
  }
}

//*******************************************************************************
// Function to handle transitions between states
void transition(int trigger) {
  switch (state) {
    case MAIN:
      timeoutValue = millis();
      if (trigger == btnSELECT) {
        lcd.clear();
        state = MENU_EDIT_FEEDTIME1;
      }
      break;

    case MENU_EDIT_FEEDTIME1:
      timeoutValue = millis();
      if (trigger == trigTIMEOUT) {
        lcd.clear();
        state = MAIN;
      }
      if (trigger == btnUP) {
        // No action, first menu
      } else if (trigger == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_FEEDTIME2;
      } else if (trigger == btnSELECT) {
        lcd.clear();
        state = EDIT_FEED_TIME1_HOUR;
      }
      if (trigger == btnBACK) {
        lcd.clear();
        state = MAIN;
      }
      break;

    case MENU_EDIT_FEEDTIME2:
      timeoutValue = millis();
      if (trigger == trigTIMEOUT) {
        lcd.clear();
        state = MAIN;
      }
      if (trigger == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEEDTIME1;
      } else if (trigger == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_FEEDAMOUNT;
      } else if (trigger == btnSELECT) {
        lcd.clear();
        state = EDIT_FEED_TIME2_HOUR;
      }
      if (trigger == btnBACK) {
        lcd.clear();
        state = MAIN;
      }
      break;

    case MENU_EDIT_FEEDAMOUNT:
      timeoutValue = millis();
      if (trigger == trigTIMEOUT) {
        lcd.clear();
        state = MAIN;
      }
      if (trigger == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEEDTIME2;
      } else if (trigger == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      } else if (trigger == btnSELECT) {
        lcd.clear();
        state = EDIT_FEED_AMOUNT1;
      }
      if (trigger == btnBACK) {
        lcd.clear();
        state = MAIN;
      }
      break;
    default:
      break;
  }
}

//*******************************************************************************
// Handle the alarm activation and feeding process
void check_alarm() {
  // Check every minute for the alarm
  if (Second == 0) {
    // Check for feed time 1
    if (Hour == feed_time1_hour && Minute == feed_time1_minute) {
      alarm1Activated = true;
      if (feed_time1_active && !manualCancelFeed1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("*** Feed #1 ***");
        lcd.setCursor(1, 1);
        lcd.print("/");
        lcd.print(feedAmount1);
        lcd.print(" portions");

        activateFeeder(feedAmount1);
        if (!hallSensorFail) {
          digitalWrite(LED_SUCCESS1_PIN, HIGH);
        } else {
          digitalWrite(LED_SUCCESS1_PIN, LOW);
        }
      }
    }

    // Check for feed time 2
    if (Hour == feed_time2_hour && Minute == feed_time2_minute) {
      alarm2Activated = true;
      if (feed_time2_active && !manualCancelFeed2) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("*** Feed #2 ***");
        lcd.setCursor(1, 1);
        lcd.print("/");
        lcd.print(feedAmount2);
        lcd.print(" portions");

        activateFeeder(feedAmount2);
        if (!hallSensorFail) {
          digitalWrite(LED_SUCCESS2_PIN, HIGH);
        } else {
          digitalWrite(LED_SUCCESS2_PIN, LOW);
        }
      }
    }
  } else {
    if (alarm1Activated) {
      alarm1Activated = false;
      manualCancelFeed1 = false;
    }
    if (alarm2Activated) {
      alarm2Activated = false;
      manualCancelFeed2 = false;
    }
  }
}

//*******************************************************************************
// Handle manual feed requests
void check_manual_feed() {
  if (manualFeed) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Manual feeding...");
    lcd.setCursor(2, 1);
    lcd.print("1 portion");

    activateFeeder(1);
    manualFeed = false;
  }
}

//*******************************************************************************
// Function to activate the feeder for the given number of portions
void activateFeeder(int portions) {
  turns = 1;
  int rotationTime = 0;
  spinningWheelSymbol = 1;
  hallSensorBackup_currentMillis = millis();

  while (turns <= portions) {
    lcd.setCursor(0, 1);
    lcd.print(turns);
    displaySpinningWheel();

    startFeederServo();

    if (hallSensorActivated || millis() - hallSensorBackup_currentMillis > hallSensorBackup_interval) {
      rotationTime = millis() - hallSensorBackup_currentMillis;
      if (!hallSensorActivated) {
        hallSensorFail = true;
      }

      hallSensorActivated = false;
      hallSensorBackup_currentMillis = millis();
      buttonBack.read();
      if (buttonBack.wasPressed()) {
        break;
      }

      turns++;
      digitalWrite(LED_HALL_SENSOR_PIN, LOW);
    }
  }

  lcd.setCursor(15, 1);
  lcd.print(" ");
  delay(FEEDER_STOP_DELAY);
  stopFeederServo();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("* Feeding done *");
  if (manualFeed) {
    lcd.setCursor(0, 1);
    lcd.print("Rotation time: ");
    lcd.print(rotationTime);
    lcd.print("ms");
  }
  delay(2000);
  lcd.clear();
}

//*******************************************************************************
// Function to start the feeder servo
void startFeederServo() {
  digitalWrite(SERVO_OUTPUT_PIN, HIGH);
  delayMicroseconds(950);  // Full speed CCW for HSR-2645CRH Servo
  digitalWrite(SERVO_OUTPUT_PIN, LOW);
  delay(20);
}

//*******************************************************************************
// Function to stop the feeder servo
void stopFeederServo() {
  digitalWrite(SERVO_OUTPUT_PIN, HIGH);
  delayMicroseconds(1490);  // Stop position for HSR-2645CRH Servo
  digitalWrite(SERVO_OUTPUT_PIN, LOW);
  delay(20);
}

//*******************************************************************************
// Handle LCD backlight timeout
void check_LcdBacklight() {
  if (millis() - lcdBacklight_currentMillis > LCD_BACKLIGHT_ON_TIME) {
    lcd.noBacklight();
  }
}

//*******************************************************************************
// Turn on the LCD backlight and reset the timer
void lcd_backlight_ON() {
  lcdBacklight_currentMillis = millis();
  lcd.backlight();
}

//*******************************************************************************
// Show the startup animation for LEDs and LCD
void ledsAndLcdDisplayStartup() {
  digitalWrite(LED_CANCEL1_PIN, HIGH);
  digitalWrite(LED_CANCEL2_PIN, HIGH);
  digitalWrite(LED_SUCCESS1_PIN, HIGH);
  digitalWrite(LED_SUCCESS2_PIN, HIGH);
  digitalWrite(LED_HALL_SENSOR_PIN, HIGH);

  lcd.clear();
  for (int p = 0; p <= 15; p++) {
    lcd.setCursor(p, 0);
    lcd.write(255);
    lcd.setCursor(15 - p, 1);
    lcd.write(255);
    delay(50);
  }
  lcd.clear();
  for (int p = 0; p <= 15; p++) {
    lcd.setCursor(15 - p, 0);
    lcd.write(255);
    lcd.setCursor(p, 1);
    lcd.write(255);
    delay(50);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feed-O-Matic");
  lcd.setCursor(0, 1);
  lcd.print("v1.1_UK 20-02-18");
  delay(5000);
  lcd.clear();

  digitalWrite(LED_CANCEL1_PIN, LOW);
  digitalWrite(LED_CANCEL2_PIN, LOW);
  digitalWrite(LED_SUCCESS1_PIN, LOW);
  digitalWrite(LED_SUCCESS2_PIN, LOW);
  digitalWrite(LED_HALL_SENSOR_PIN, LOW);
}

//*******************************************************************************
// Blink function for various displays
void blinkFunction() {
  blink_currentMillis = millis();
  if (blink_currentMillis - blink_previousMillis > blink_interval) {
    blink_previousMillis = blink_currentMillis;
    blink_state = !blink_state;
  }
}


