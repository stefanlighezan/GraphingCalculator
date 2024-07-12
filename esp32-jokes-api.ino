#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>
#include <math.h>

#define TFT_DC 2
#define TFT_CS 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Keypad setup
const byte ROWS = 4;  // four rows
const byte COLS = 4;  // four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};    // connect to the row pinouts of the keypad
byte colPins[COLS] = {26, 25, 33, 32};    // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String function = "y=";

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  displayFunction();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    handleKeyPress(key);
  }
}

void displayFunction() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print(function);
}

void drawEquation(String equation) {
  tft.fillScreen(ILI9341_BLACK);
  tft.drawLine(0, tft.height() / 2, tft.width(), tft.height() / 2, ILI9341_WHITE);  // Draw the x-axis
  tft.drawLine(tft.width() / 2, 0, tft.width() / 2, tft.height(), ILI9341_WHITE);   // Draw the y-axis

  int prevXPos = 0;
  int prevYPos = 0;
  bool firstPoint = true;

  for (float x = -10; x <= 10; x += 0.1) {
    float yVal = evaluateEquation(equation, x);
    int xPos = map(x, -10, 10, 0, tft.width());
    int yPos = map(yVal, -10, 10, tft.height(), 0); // Inverted to match screen coordinates

    if (firstPoint) {
      firstPoint = false;
    } else {
      tft.drawLine(prevXPos, prevYPos, xPos, yPos, ILI9341_GREEN);
    }

    prevXPos = xPos;
    prevYPos = yPos;
  }
}

float evaluateEquation(String equation, float x) {
  // For simplicity, assuming equation is like "y=sin(x)" or "y=2*x" etc.
  if (equation.indexOf("sin(x)") != -1) {
    return sin(x);
  } else if (equation.indexOf("cos(x)") != -1) {
    return cos(x);
  } else if (equation.indexOf("tan(x)") != -1) {
    return tan(x);
  } else if (equation.indexOf("x") != -1) {
    String coefficientStr = equation.substring(2, equation.indexOf("x"));
    float coefficient = coefficientStr.toFloat();
    return coefficient * x;
  }
  return 0;
}

void handleKeyPress(char key) {
  if (key == '#') {
    drawEquation(function);
  } else if (key == 'D') {
    function += "x";
  } else {
    function += key;
  }
  displayFunction();
}
