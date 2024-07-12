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

char expression[50]; // Character array to store expression
int cursorPos = 0;
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
  tft.println("Press x for Grapher");
}

void displayGrapher() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Grapher App");
  drawAxes();
}

float evaluateExpression(char* expr, float x) {
  int len = strlen(expr);
  float result = 0;
  float term = 0;
  char operation = '+';
  bool term_start = true;

  for (int i = 0; i < len; i++) {
    char c = expr[i];
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
        if (expr[j] == '(') bracket_count++;
        if (expr[j] == ')') bracket_count--;
      }
      char subexpr[50];
      strncpy(subexpr, expr + i + 1, j - i - 2);
      subexpr[j - i - 2] = '\0';
      term = evaluateExpression(subexpr, x);
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
    } else if (isalpha(c)) {
      int func_start = i;
      while (isalpha(expr[i])) {
        i++;
      }
      char func[5];
      strncpy(func, expr + func_start, i - func_start);
      func[i - func_start] = '\0';
      if (strcmp(func, "sin") == 0 || strcmp(func, "cos") == 0 || strcmp(func, "tan") == 0) {
        i++; // skip '('
        int j = i;
        int bracket_count = 1;
        for (; j < len && bracket_count > 0; j++) {
          if (expr[j] == '(') bracket_count++;
          if (expr[j] == ')') bracket_count--;
        }
        char inner_expr[50];
        strncpy(inner_expr, expr + i, j - i - 1);
        inner_expr[j - i - 1] = '\0';
        float inner_val = evaluateExpression(inner_expr, x);
        if (strcmp(func, "sin") == 0) term = sin(inner_val);
        if (strcmp(func, "cos") == 0) term = cos(inner_val);
        if (strcmp(func, "tan") == 0) term = tan(inner_val);
        i = j - 1;
      }
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
        expression[0] = '\0'; // Clear expression
        cursorPos = 0;
        displayExpression();
      }
    } else if (key == 'C') {
      shiftPressed = true;
    } else if (key == 'A' && shiftPressed) {
      if (strlen(expression) > 0 && cursorPos > 0) {
        memmove(expression + cursorPos - 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
        cursorPos--;
        displayExpression();
      }
      shiftPressed = false;
    } else if (key == '#') {
      openGrapherApp();  // Reset grapher
    } else {
      if (shiftPressed) {
        handleShiftedKeyPress(key);
        shiftPressed = false;
      } else {
        memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
        expression[cursorPos] = key;
        cursorPos++;
        displayExpression();
      }
    }
  }
}

void handleShiftedKeyPress(char key) {
  if (key == 'x') {
    expression[0] = '\0'; // Clear expression
    cursorPos = 0;
    displayExpression();
  } else if (key == '0' && appIndex == 0) {
    drawGraph(expression);
    expression[0] = '\0'; // Clear expression
    cursorPos = 0;
    displayExpression();
  } else if (key == '+') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = '(';
    cursorPos++;
    displayExpression();
  } else if (key == '-') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = ')';
    cursorPos++;
    displayExpression();
  } else if (key == '1') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "sin(", 4);
    cursorPos += 4;
    displayExpression();
  } else if (key == '2') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "cos(", 4);
    cursorPos += 4;
    displayExpression();
  } else if (key == '3') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "tan(", 4);
    cursorPos += 4;
    displayExpression();
  } else if (key == '4') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "exp(", 4);
    cursorPos += 4;
    displayExpression();
  } else if (key == '5') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = ')';
    cursorPos++;
    displayExpression();
  } else if (key == '6') {
    if (cursorPos > 0) cursorPos--;
    displayExpression();
  } else if (key == '7') {
    if (cursorPos < strlen(expression)) cursorPos++;
    displayExpression();
  }
}

void drawAxes() {
  tft.drawLine(0, tft.height() / 2, tft.width(), tft.height() / 2, ILI9341_WHITE);
  tft.drawLine(tft.width() / 2, 0, tft.width() / 2, tft.height(), ILI9341_WHITE);
}

void drawGraph(char* expr) {
  if (appIndex != 0) {
    return;
  }
  
  tft.fillScreen(ILI9341_BLACK);
  drawAxes();

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  float xScale = tft.width() / 20.0;
  float yScale = tft.height() / 20.0; 

  float prevX = -10;
  float prevY = evaluateExpression(expr, prevX);

  for (float i = -10.0; i <= 10.0; i += 0.05) { 
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
  tft.fillRect(10, 10, tft.width() - 20, 30, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print(expression);
  tft.setCursor(10 + cursorPos * 12, 10); // Adjust position based on text size
  tft.print("_");
}
