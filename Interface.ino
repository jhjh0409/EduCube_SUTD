// Importing Libraries
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>           // Widget library
// MPU6050 Libraries
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// Defining CS Pins for Both Screens
#define firstScreenCS 9
#define secondScreenCS 10

// Defining MPU6050 Pins
int sda_pin = 17; // GPIO8 as I2C SDA
int scl_pin = 18; // GPIO9 as I2C SCL

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData2"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// Button Objects
ButtonWidget btn1 = ButtonWidget(&tft);
ButtonWidget btn2 = ButtonWidget(&tft);
ButtonWidget btn3 = ButtonWidget(&tft);
ButtonWidget btn4 = ButtonWidget(&tft);
ButtonWidget btn5 = ButtonWidget(&tft);
ButtonWidget btn6 = ButtonWidget(&tft);
ButtonWidget btn7 = ButtonWidget(&tft);
ButtonWidget btn8 = ButtonWidget(&tft);
#define BUTTON_W 100
#define BUTTON_H 100
#define BUTTON_W2 120
#define BUTTON_H2 120
ButtonWidget* btn[] = {&btn1 , &btn2 , &btn3};;
uint8_t buttonCount = sizeof(btn) / sizeof(btn[0]);
ButtonWidget* lsn[] = {&btn5 , &btn6 , &btn7 , &btn8};;
uint8_t lsnCount = sizeof(lsn) / sizeof(lsn[0]);

const int screenWidth = 240; // Screen width
const int screenHeight = 320; // Screen height

const int countdownMinutes = 1; // Countdown duration in minutes
unsigned long previousMillis = 0;
unsigned long interval = 1000; // Update interval in milliseconds

char* mode = "menu";
int breakRun = 0;
int lessonRun = 0;

void setup() {
  // Use serial port
  Serial.begin(9600);

  pinMode(firstScreenCS, OUTPUT);
  digitalWrite(firstScreenCS, HIGH);
  
  pinMode(secondScreenCS, OUTPUT);
  digitalWrite(secondScreenCS, HIGH);
  
  // Set both cs pins LOW, or 'active' 
  // to initialise both at the same time
  digitalWrite(firstScreenCS, LOW);
  digitalWrite(secondScreenCS, LOW);

  // Initialise the TFT screen
  tft.init();

  // Set both cs pins HIGH, or 'inactive'
  digitalWrite(firstScreenCS, HIGH);
  digitalWrite(secondScreenCS, HIGH);

  // Calibrate touchscreen for second screen
  digitalWrite(secondScreenCS, LOW);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  touch_calibrate();
  initButtons();
  digitalWrite(secondScreenCS, HIGH);

  // Setup first screen
  digitalWrite(firstScreenCS, LOW);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.println("Hello World.");
  digitalWrite(firstScreenCS, HIGH);
}

void loop() {
  digitalWrite(secondScreenCS, LOW);
  static uint32_t scanTime = millis();
  uint16_t t_x = 9999, t_y = 9999; // To store the touch coordinates

  // Scan keys every 50ms at most
  if (millis() - scanTime >= 50) {
    // Pressed will be set true if there is a valid touch on the screen
    bool pressed = tft.getTouch(&t_x, &t_y);
    bool goBack = false;

    // If mode not on menu, set bool to false
    if (mode != "menu") {
      pressed = false;
      goBack = true;
    }

    if (mode == "lesson" and pressed == false)
    {
      for (uint8_t b = 0; b < lsnCount; b++) {
        if (lsn[b]->contains(t_x, t_y)) {
          lsn[b]->press(true);
          lsn[b]->pressAction();
        }
        
        else {
          lsn[b]->press(false);
          lsn[b]->releaseAction();
        }
      }
    }

    if (goBack) {
        if (btn4.contains(t_x, t_y)) {
          btn4.press(true);
          btn4.pressAction();
        }
      }
      
    else {
      btn4.press(false);
      btn4.releaseAction();
    }

    for (uint8_t b = 0; b < buttonCount; b++) {
      if (pressed) {
        if (btn[b]->contains(t_x, t_y)) {
          btn[b]->press(true);
          btn[b]->pressAction();
        }
      }
      
      else {
        btn[b]->press(false);
        btn[b]->releaseAction();
      }
    }
  }

    // Check screen is on Break Mode
    if (mode == "break") {
      digitalWrite(firstScreenCS, LOW);
      // Checks whether breakRun has been ran.
      if (breakRun == 0) {
        breakMode();
        breakRun = 1;
        btn4.drawSmoothButton(false, 2, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing
        tft.setTextSize(5);
      }
      timerCount();
    }

    // Check screen is on Lesson Mode
    if (mode == "lesson") {
      if (lessonRun == 0)
      {
        // Draw Buttons
        lessonScreen();
        lessonRun = 1;
      }
    }
  digitalWrite(firstScreenCS, HIGH);
  digitalWrite(secondScreenCS, HIGH);
}

// Calibrating Touch Function
void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!LittleFS.begin()) {
    Serial.println("Formating file system");
    LittleFS.format();
    LittleFS.begin();
  }

  // check if calibration file exists and size is correct
  if (LittleFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      LittleFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = LittleFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = LittleFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

// Function to start the break timer
void timerCount() {
  unsigned long currentMillis = millis();

      // Check if it's time to update the countdown
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    static int remainingMinutes = countdownMinutes;
    static int remainingSeconds = 0;

    // Update remaining time
    if (remainingSeconds == 0) {
      if (remainingMinutes > 0) {
        remainingMinutes--;
        remainingSeconds = 59;
      } else {
        // Display "Time's up!" when countdown is finished
        tft.fillScreen(TFT_RED); // Clear the screen
        tft.setTextColor(TFT_WHITE); // Set text color to red
        tft.setTextSize(5);
        int xPos = (screenWidth - tft.textWidth("Time's up!")) / 2 + 50;
        int yPos = (screenHeight - tft.fontHeight()) / 2 - 35;
        tft.setCursor(xPos, yPos);
        tft.print("Time's up!");
        delay(2000);
        homeScreen();
        breakRun = 0;
        previousMillis = 0;
        remainingMinutes = countdownMinutes;
      }
    } else {
      remainingSeconds--;
    }
    if (mode != "menu") {
      // Update the display with the new countdown time
      int xPos = (screenWidth - tft.textWidth(formatTime(remainingMinutes, remainingSeconds))) / 2 + 50;
      int yPos = (screenHeight - tft.fontHeight()) / 2 - 35;
      tft.fillRect(xPos, yPos, tft.textWidth("00:00"), tft.fontHeight(), TFT_BLACK); // Clear previous text
      tft.setCursor(xPos, yPos);
      tft.print(formatTime(remainingMinutes, remainingSeconds));
    }
  }
}

void breakMode() {
  tft.fillScreen(TFT_BLACK);
  // Set text color to white
  tft.setTextColor(TFT_WHITE);
  // Set text size
  tft.setTextSize(5);

  // Set the initial position of the text
  int xPos = (screenWidth - tft.textWidth("00:00")) / 2 + 50;
  int yPos = (screenHeight - tft.fontHeight()) / 2 - 35;

  // Display the initial countdown time
  tft.setCursor(xPos, yPos);
  tft.print(formatTime(countdownMinutes, 0));
}

// Function to format time in MM:SS format
String formatTime(int minutes, int seconds) {
  String timeString = "";
  if (minutes < 10) {
    timeString += "0";
  }
  timeString += minutes;
  timeString += ":";
  if (seconds < 10) {
    timeString += "0";
  }
  timeString += seconds;
  return timeString;
}

void btn1_pressAction(void)
{
  if (btn1.justPressed()) {
    Serial.println("First button pressed");
    btn1.drawSmoothButton(true);
  }
}

void btn1_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn1.justReleased()) {
    Serial.println("First button released");
    //btn1.drawSmoothButton(false);
    btn1.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn1.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn1.setReleaseTime(millis());
      //btn1.drawSmoothButton(!btn1.getState());
    }
  }
}

void btn2_pressAction(void)
{
  if (btn2.justPressed()) {
    Serial.println("Second button just pressed");
    btn2.drawSmoothButton(true);
    mode = "lesson";
    tft.fillScreen(TFT_BLACK);
  }
}

void btn2_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn2.justReleased()) {
    Serial.println("Second button just released");
    //btn2.drawSmoothButton(false);
    btn2.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn2.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn2.setReleaseTime(millis());
      //btn2.drawSmoothButton(!btn2.getState());
    }
  }
}

void btn3_pressAction(void)
{
  if (btn3.justPressed()) {
    Serial.println("Third button just pressed");
    btn3.drawSmoothButton(true);
    mode = "break";
    tft.fillScreen(TFT_BLACK);
  }
}

void btn3_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn3.justReleased()) {
    Serial.println("Third button just released");
    //btn3.drawSmoothButton(false);
    btn3.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn3.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn3.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}

void btn4_pressAction(void)
{
  if (btn4.justPressed()) {
    Serial.println("Fourth button just pressed");
    homeScreen();
    breakRun = 0;
    lessonRun = 0;
    previousMillis = 0;
  }
}

void btn4_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn4.justReleased()) {
    Serial.println("Fourth button just released");
    //btn3.drawSmoothButton(false);
    btn4.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn4.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn4.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}

// Call TA
void btn5_pressAction(void)
{
  if (btn5.justPressed()) {
    Serial.println("btn5 button just pressed");
    tft.fillScreen(TFT_RED);
    lessonScreen();
  }
}

void btn5_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn5.justReleased()) {
    Serial.println("btn5 button just released");
    //btn3.drawSmoothButton(false);
    btn5.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn5.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn5.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}

void btn6_pressAction(void)
{
  if (btn6.justPressed()) {
    Serial.println("btn6 button just pressed");
    btn6.drawSmoothButton(true);
  }
}

void btn6_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn6.justReleased()) {
    Serial.println("btn6 button just released");
    //btn3.drawSmoothButton(false);
    btn6.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn6.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn6.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}

void btn7_pressAction(void)
{
  if (btn7.justPressed()) {
    Serial.println("btn7 button just pressed");
    btn7.drawSmoothButton(true);
  }
}

void btn7_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn7.justReleased()) {
    Serial.println("btn7 button just released");
    //btn3.drawSmoothButton(false);
    btn7.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn7.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn7.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}

void btn8_pressAction(void)
{
  if (btn8.justPressed()) {
    Serial.println("btn8 button just pressed");
    btn8.drawSmoothButton(true);
  }
}

void btn8_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btn8.justReleased()) {
    Serial.println("btn8 button just released");
    //btn3.drawSmoothButton(false);
    btn8.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btn8.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btn8.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}
//Template for copying buttons
/*void btnx_pressAction(void)
{
  if (btnx.justPressed()) {
    Serial.println("btnx button just pressed");
    btnx.drawSmoothButton(true);
    mode = "break";
    tft.fillScreen(TFT_BLACK);
  }
}

void btnx_releaseAction(void)
{
  static uint32_t waitTime = 1000;
  if (btnx.justReleased()) {
    Serial.println("btnx button just released");
    //btn3.drawSmoothButton(false);
    btnx.setReleaseTime(millis());
    waitTime = 10000;
  }
  else {
    if (millis() - btnx.getReleaseTime() >= waitTime) {
      waitTime = 1000;
      btnx.setReleaseTime(millis());
      //btn3.drawSmoothButton(!btn3.getState());
    }
  }
}*/

void drawButtons() {
  btn1.drawSmoothButton(false, 2, TFT_BLACK);
  btn2.drawSmoothButton(false, 2, TFT_BLACK);
  btn3.drawSmoothButton(false, 2, TFT_BLACK);
}

// Function to change back to homescreen
void homeScreen() {
  digitalWrite(secondScreenCS, HIGH);
  digitalWrite(firstScreenCS, LOW);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.println("Hello World.");
  digitalWrite(firstScreenCS, HIGH);
  digitalWrite(secondScreenCS, LOW);
  tft.fillScreen(TFT_BLACK);
  drawButtons();
  mode = "menu";
}

void lessonScreen() {
  btn4.drawSmoothButton(false, 2, TFT_BLACK);
  btn5.drawSmoothButton(false, 2, TFT_BLACK);
  btn6.drawSmoothButton(false, 2, TFT_BLACK);
  btn7.drawSmoothButton(false, 2, TFT_BLACK);
  btn8.drawSmoothButton(false, 2, TFT_BLACK);
}

// Initialise Buttons
void initButtons() {
  uint16_t x = (tft.width() - BUTTON_W) / 3 - BUTTON_H;
  uint16_t y = tft.height() / 3 - 10;
  btn1.initButtonUL(0, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_BLUE, "Quiz", 2);
  btn1.setPressAction(btn1_pressAction);
  btn1.setReleaseAction(btn1_releaseAction);
  btn1.drawSmoothButton(false, 2, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing

  x = tft.width() / 3 + 5;
  btn2.initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_GREEN, "Lesson", 2);
  btn2.setPressAction(btn2_pressAction);
  btn2.setReleaseAction(btn2_releaseAction);
  btn2.drawSmoothButton(false, 2, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing

  btn3.initButtonUL(220, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_BLACK, TFT_RED, "Break", 2);
  btn3.setPressAction(btn3_pressAction);
  btn3.setReleaseAction(btn3_releaseAction);
  btn3.drawSmoothButton(false, 2, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing

  btn4.initButtonUL(0, 0, 60, 35, TFT_BLACK, TFT_BLACK, TFT_WHITE, "Back", 2);
  btn4.setPressAction(btn4_pressAction);
  btn4.setReleaseAction(btn4_releaseAction);

  // Buttons 5-8 are Lesson Mode Buttons
  btn5.initButtonUL(80, 0, BUTTON_W2, BUTTON_H2, TFT_WHITE, TFT_BLACK, TFT_RED, "Call TA", 2);
  btn5.setPressAction(btn5_pressAction);
  btn5.setReleaseAction(btn5_releaseAction);

  btn6.initButtonUL(200, 0, BUTTON_W2, BUTTON_H2, TFT_WHITE, TFT_BLACK, TFT_YELLOW, "Slow Down", 2);
  btn6.setPressAction(btn6_pressAction);
  btn6.setReleaseAction(btn6_releaseAction);

  btn7.initButtonUL(80, 120, BUTTON_W2, BUTTON_H2, TFT_WHITE, TFT_BLACK, TFT_GREENYELLOW, "Repeat", 2);
  btn7.setPressAction(btn7_pressAction);
  btn7.setReleaseAction(btn7_releaseAction);

  btn8.initButtonUL(200, 120, BUTTON_W2, BUTTON_H2, TFT_WHITE, TFT_BLACK, TFT_ORANGE, "Question", 2);
  btn8.setPressAction(btn8_pressAction);
  btn8.setReleaseAction(btn8_releaseAction);
}