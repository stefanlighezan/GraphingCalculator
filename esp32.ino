#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>
#include <math.h>

#define TFT_DC 2
#define TFT_CS 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'C', '0', 'x', '/'}
};

int appIndex = -1;

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String expression = "";
bool shiftPressed = false;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  displayHomeScreen();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    handleKeyPress(key);
  }
}

void displayHomeScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Home Screen");
  tft.println("Press # for Grapher");
}

void displayGrapher() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Grapher App");
}

float evaluateExpression(String expr, float x) {
  int len = expr.length();
  float result = 0;
  float term = 0;
  char operation = '+';
  bool term_start = true;

  for (int i = 0; i < len; i++) {
    char c = expr.charAt(i);
    if (isdigit(c) || c == '.') {
      if (term_start) {
        term = 0;
        term_start = false;
      }
      term = term * 10 + (c - '0');
    } else if (c == 'x') {
      term = x;
      term_start = false;
    } else if (c == '(') {
      int bracket_count = 1;
      int j = i + 1;
      for (; j < len && bracket_count > 0; j++) {
        if (expr.charAt(j) == '(') bracket_count++;
        if (expr.charAt(j) == ')') bracket_count--;
      }
      term = evaluateExpression(expr.substring(i + 1, j - 1), x);
      i = j - 1;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
      switch (operation) {
        case '+': result += term; break;
        case '-': result -= term; break;
        case '*': result *= term; break;
        case '/': result /= term; break;
      }
      operation = c;
      term_start = true;
    }
  }

  switch (operation) {
    case '+': result += term; break;
    case '-': result -= term; break;
    case '*': result *= term; break;
    case '/': result /= term; break;
  }

  return result;
}

void handleKeyPress(char key) {
  if (key == 'x' && appIndex == -1) {
    openGrapherApp();
  } else if (appIndex != -1) {
    if (key == '=') {
      if (appIndex == 0) {
        drawGraph(expression);
        expression = "";
        displayExpression();
      }
    } else if (key == 'C') {
      shiftPressed = true;
    } else if (key == 'A' && shiftPressed) {
      if (expression.length() > 0) {
        expression.remove(expression.length() - 1);
        displayExpression();
      }
      shiftPressed = false;
    } else {
      if (shiftPressed) {
        handleShiftedKeyPress(key);
        shiftPressed = false;
      } else {
        expression += key;
        displayExpression();
      }
    }
  }
}

void handleShiftedKeyPress(char key) {
  if (key == 'x') {
    expression = "";
    displayExpression();
  } else if (key == '0' && appIndex == 0) {
    drawGraph(expression);
    expression = "";
    displayExpression();
  } else if (key == '+') {
    expression += "(";
    displayExpression();
  } else if (key == '-') {
    expression += ")";
    displayExpression();
  }
}

void drawGraph(String expr) {
  if (appIndex != 0) {
    return;
  }
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.drawLine(0, tft.height() / 2, tft.width(), tft.height() / 2, ILI9341_WHITE);
  tft.drawLine(tft.width() / 2, 0, tft.width() / 2, tft.height(), ILI9341_WHITE);

  float xScale = tft.width() / 20.0;
  float yScale = tft.height() / 20.0; 

  float prevX = -10;
  float prevY = evaluateExpression(expr, prevX);

  for (float i = -10.0; i <= 10.0; i += 0.1) { 
    float x = i;
    float y = evaluateExpression(expr, x);

    int x1 = prevX * xScale + tft.width() / 2;
    int y1 = tft.height() / 2 - prevY * yScale;
    int x2 = x * xScale + tft.width() / 2;
    int y2 = tft.height() / 2 - y * yScale;

    tft.drawLine(x1, y1, x2, y2, ILI9341_GREEN);

    prevX = x;
    prevY = y;
  }
}

void openGrapherApp() {
  appIndex = 0;
  displayGrapher();
}

void displayExpression() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print(expression);
}
