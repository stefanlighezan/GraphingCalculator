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

char expression[50];
int cursorPos = 0;
bool shiftPressed = false;

float xScale = 1.0;
float yScale = 1.0;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  displayHome();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    handleKey(key);
  }
}

void displayHome() {
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
  drawAxises();
}

float evaluateExpression(char* expr, float x) {
  int len = strlen(expr);
  float result = 0;
  float term = 0;
  char operation = '+';
  bool termStart = true;

  for (int i = 0; i < len; i++) {
    char c = expr[i];
    if (isdigit(c) || c == '.') {
      if (termStart) {
        term = 0;
        termStart = false;
      }
      term = term * 10 + (c - '0');
    } else if (c == 'x') {
      term = x;
      termStart = false;
    } else if (c == '(') {
      int bracketCount = 1;
      int j = i + 1;
      for (; j < len && bracketCount > 0; j++) {
        if (expr[j] == '(') bracketCount++;
        if (expr[j] == ')') bracketCount--;
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
      termStart = true;
    } else if (isalpha(c)) {
      int funcStart = i;
      while (isalpha(expr[i])) {
        i++;
      }
      char func[5];
      strncpy(func, expr + funcStart, i - funcStart);
      func[i - funcStart] = '\0';
      if (strcmp(func, "sin") == 0 || strcmp(func, "cos") == 0 || strcmp(func, "tan") == 0) {
        i++;
        int j = i;
        int bracketCount = 1;
        for (; j < len && bracketCount > 0; j++) {
          if (expr[j] == '(') bracketCount++;
          if (expr[j] == ')') bracketCount--;
        }
        char innerExpr[50];
        strncpy(innerExpr, expr + i, j - i - 1);
        innerExpr[j - i - 1] = '\0';
        float innerVal = evaluateExpression(innerExpr, x);
        if (strcmp(func, "sin") == 0) term = sin(innerVal);
        if (strcmp(func, "cos") == 0) term = cos(innerVal);
        if (strcmp(func, "tan") == 0) term = tan(innerVal);
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

void handleKey(char key) {
  if (key == 'x' && appIndex == -1) {
    openGrapher();
  } else if (appIndex != -1) {
    if (key == '=') {
      if (appIndex == 0) {
        drawGraph(expression);
        expression[0] = '\0';
        cursorPos = 0;
        displayExpr();
      }
    } else if (key == 'C') {
      shiftPressed = true;
    } else if (key == 'A' && shiftPressed) {
      if (strlen(expression) > 0 && cursorPos > 0) {
        if (expression[cursorPos - 1] == '(') {
          int start = cursorPos - 1;
          while (start > 0 && isalpha(expression[start - 1])) {
            start--;
          }
          memmove(expression + start, expression + cursorPos, strlen(expression) - cursorPos + 1);
          cursorPos = start;
        } else {
          memmove(expression + cursorPos - 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
          cursorPos--;
        }
        displayExpr();
      }
      shiftPressed = false;
    } else if (key == '#') {
      openGrapher();
    } else {
      if (shiftPressed) {
        handleShiftedKey(key);
        shiftPressed = false;
      } else {
        memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
        expression[cursorPos] = key;
        cursorPos++;
        displayExpr();
      }
    }
  }
}

void handleShiftedKey(char key) {
  if (key == 'x') {
    expression[0] = '\0';
    cursorPos = 0;
    displayExpr();
  } else if (key == '0' && appIndex == 0) {
    drawGraph(expression);
    expression[0] = '\0';
    cursorPos = 0;
    displayExpr();
  } else if (key == '+') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = '(';
    cursorPos++;
    displayExpr();
  } else if (key == '-') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = ')';
    cursorPos++;
    displayExpr();
  } else if (key == '1') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "sin(", 4);
    cursorPos += 4;
    displayExpr();
  } else if (key == '2') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "cos(", 4);
    cursorPos += 4;
    displayExpr();
  } else if (key == '3') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "tan(", 4);
    cursorPos += 4;
    displayExpr();
  } else if (key == '4') {
    memmove(expression + cursorPos + 4, expression + cursorPos, strlen(expression) - cursorPos + 1);
    strncpy(expression + cursorPos, "exp(", 4);
    cursorPos += 4;
    displayExpr();
  } else if (key == '5') {
    memmove(expression + cursorPos + 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
    expression[cursorPos] = ')';
    cursorPos++;
    displayExpr();
  } else if (key == '6') {
    if (cursorPos > 0) {
      memmove(expression + cursorPos - 1, expression + cursorPos, strlen(expression) - cursorPos + 1);
      cursorPos--;
      displayExpr();
    }
  } else if (key == '7') {
    if (cursorPos < strlen(expression)) {
      memmove(expression + cursorPos, expression + cursorPos + 1, strlen(expression) - cursorPos);
      displayExpr();
    }
  } else if (key == '*') {
    float zoomFactor = 2.0;
    xScale *= zoomFactor;
    yScale *= zoomFactor;
    drawAxises();
    drawGraph(expression);
  } else if (key == '/') {
    float zoomFactor = 0.5;
    xScale *= zoomFactor;
    yScale *= zoomFactor;
    drawAxises();
    drawGraph(expression);
  }
}

void drawAxises() {
  tft.fillScreen(ILI9341_BLACK);

  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
 
  tft.drawLine(0, centerY, tft.width(), centerY, ILI9341_WHITE);
  for (int x = -10; x <= 10; x++) {
    int tickX = centerX + x * (tft.width() / 20) * xScale;
    tft.drawLine(tickX, centerY - 3, tickX, centerY + 3, ILI9341_WHITE);
  }
  tft.drawLine(centerX, 0, centerX, tft.height(), ILI9341_WHITE);
  for (int y = -10; y <= 10; y++) {
    int tickY = centerY - y * (tft.height() / 20) * yScale;
    tft.drawLine(centerX - 3, tickY, centerX + 3, tickY, ILI9341_WHITE);
  }
}

void drawGraph(char* expr) {
  if (appIndex != 0) {
    return;
  }
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  float prevX = -10;
  float prevY = evaluateExpression(expr, prevX);

  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;

  for (float i = -10.0; i <= 10.0; i += 0.05) { 
    float x = i;
    float y = evaluateExpression(expr, x);

    int x1 = prevX * (tft.width() / 20) * xScale + centerX;
    int y1 = centerY - prevY * (tft.height() / 20) * yScale;
    int x2 = x * (tft.width() / 20) * xScale + centerX;
    int y2 = centerY - y * (tft.height() / 20) * yScale;

    tft.drawLine(x1, y1, x2, y2, ILI9341_GREEN);

    prevX = x;
    prevY = y;
  }
}

void openGrapher() {
  appIndex = 0;
  displayGrapher();
}

void displayExpr() {
  tft.fillRect(10, 10, tft.width() - 20, 30, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print(expression);
  tft.setCursor(10 + cursorPos * 12, 10);
  tft.print("_");
}
