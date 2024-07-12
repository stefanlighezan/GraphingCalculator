#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>
#include <math.h>

#define TFT_DC 2
#define TFT_CS 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'C', '0', 'x', '/'}
};

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
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    handleKeyPress(key);
  }
}

void displayExpression() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print(expression);
}

float evaluateExpression(String expr, float x) {
  float result = 0.0;
  String replacedExpr = expr;
  replacedExpr.replace("x", String(x));
  char *mutableExpr = strdup(replacedExpr.c_str()); // Convert to mutable string

  char *token = strtok(mutableExpr, "+-*/");
  float currentValue = atof(token); // Convert initial token to float
  char lastOperator = '\0';

  while (token != NULL) {
    token = strtok(NULL, "+-*/");
    if (token != NULL) {
      char op = token[0];
      float nextValue = atof(token + 1); // Convert the number after the operator

      switch (lastOperator) {
        case '\0':
          result = currentValue;
          break;
        case '+':
          result += currentValue;
          break;
        case '-':
          result -= currentValue;
          break;
        case '*':
          result *= currentValue;
          break;
        case '/':
          result /= currentValue;
          break;
      }

      currentValue = nextValue;
      lastOperator = op;
    }
  }

  switch (lastOperator) {
    case '\0':
      result = currentValue;
      break;
    case '+':
      result += currentValue;
      break;
    case '-':
      result -= currentValue;
      break;
    case '*':
      result *= currentValue;
      break;
    case '/':
      result /= currentValue;
      break;
  }

  free(mutableExpr); // Free allocated memory
  return result;
}

void handleKeyPress(char key) {
  if (key == '=') {
    drawGraph(expression);
    expression = ""; // Clear expression after graphing
    displayExpression();
  } else if (key == 'C') {
    shiftPressed = true; // Set shift/CTX mode
  } else {
    if (shiftPressed) {
      handleShiftedKeyPress(key);
      shiftPressed = false; // Reset shift/CTX mode after using it
    } else {
      expression += key;
      displayExpression();
    }
  }
}

void handleShiftedKeyPress(char key) {
  if (key == 'x') {
    // Not applicable in this mode since we're evaluating without "y = "
    expression = "";
    displayExpression();
  } else if (key == '0') {
    // Graphing functionality
    drawGraph(expression);
    expression = ""; // Clear expression after graphing
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
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.drawLine(0, tft.height() / 2, tft.width(), tft.height() / 2, ILI9341_WHITE);  // Draw x-axis
  tft.drawLine(tft.width() / 2, 0, tft.width() / 2, tft.height(), ILI9341_WHITE);   // Draw y-axis

  float xScale = tft.width() / 20.0; // Scaling factor for x-values
  float yScale = tft.height() / 20.0; // Scaling factor for y-values

  float prevX = -10;
  float prevY = evaluateExpression(expr, prevX);

  for (float i = -10.0; i <= 10.0; i++) { // 200 points for smoothness
    float x = i; // Range from -10 to 10
    float y = evaluateExpression(expr, x);
    println(y)

    int x1 = prevX * xScale + tft.width() / 2;
    int y1 = tft.height() / 2 - prevY * yScale;
    int x2 = x * xScale + tft.width() / 2;
    int y2 = tft.height() / 2 - y * yScale;

    tft.drawLine(x1, y1, x2, y2, ILI9341_GREEN);

    prevX = x;
    prevY = y;
  }
}
