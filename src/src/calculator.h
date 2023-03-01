/*
  A handy calculator
  This code is based on: https://www.youtube.com/watch?v=LhFTZ7ekLEE&list=LL&index=1&t=96s
  Modified by TheDIYGuy999

  Limitations:
  - Only one operator per calculation
  - No negative numbers
  - Bigger numbers may not be accurate
  - Use it at your own risk!
*/

int matrixSize = 11;
int offsetX = -5; // Keyboard position offset
int offsetY = 7;
char buttons[] = {'0', '.', '=', '/', '1', '2', '3', '+', '4', '5', '6', '-', '7', '8', '9', '*', 'C'};
int mx[] = {11, 22, 33, 44, 11, 22, 33, 44, 11, 22, 33, 44, 11, 22, 33, 44, 55}; // x coordinates
int my[] = {45, 45, 45, 45, 34, 34, 34, 34, 23, 23, 23, 23, 12, 12, 12, 12, 12}; // y coordinates

int ind = 0;
int pres = 0;
String line = "";
String num1 = "";
String num2 = "";
int operation = 0;        // 1 = addition, 2 = subtraction, 3 = multiplication, 4 = division
bool calculated = false;  // Calculation done, clear previous result, if any button pushed
bool operatorSet = false; // Operator was already set

int changeNumber = 0;
float n1 = 0.0000;
float n2 = 0.0000;
float result = 0.0000;

//
// =======================================================================================================
// CALCULATION IS DONE, IF "=" SELECTED & ENCODER PUSHED
// =======================================================================================================
//

void calculate()
{
  int lineLength = line.length();
  for (int i = 0; i < lineLength; i++)

  {
    if (line.charAt(i) == '+')
    {
      changeNumber = 1;
      operation = 1;
    }

    if (line.charAt(i) == '-')
    {
      changeNumber = 1;
      operation = 2;
    }

    if (line.charAt(i) == '*')
    {
      changeNumber = 1;
      operation = 3;
    }

    if (line.charAt(i) == '/')
    {
      changeNumber = 1;
      operation = 4;
    }

    if (changeNumber == 0)
    {
      char letter = line.charAt(i);
      num1 = num1 + letter;
    }

    if (changeNumber == 2)
    {
      char letter = line.charAt(i);
      num2 = num2 + letter;
    }

    if (changeNumber == 1)
      changeNumber = 2;
  }
  n1 = num1.toFloat(); // 7-8 counts precision only!
  n2 = num2.toFloat();

  // std::size_t *pos = 0; // TODO
  // n1 = std::stof(num1, pos);

  if (operation == 1)
  {
    result = n1 + n2;
  }
  if (operation == 2)
  {
    result = n1 - n2;
  }
  if (operation == 3)
  {
    result = n1 * n2;
  }
  if (operation == 4)
  {
    result = n1 / n2;
  }

  Serial.println(n1);
  Serial.println(n2);
  Serial.println(n1 / n2);
  line = "";
  int tenths = result / 10;
  float remain = result - (tenths * 10);
  line = (String)result;
  changeNumber = 0;
  num1 = "";
  num2 = "";
  operation = 0;
  calculated = true;
  operatorSet = false;
}

//
// =======================================================================================================
// CALCULATOR
// =======================================================================================================
//

void calculator(bool left, bool right, bool select)
{
  // Read encoder ----------------------------------------------------------------------------------
  if (right) // Go to the right
    ind++;

  if (left) // Go to the left
    ind--;

  if (ind > 16) // Range limits
    ind = 0;

  if (ind < 0)
    ind = 16;

  if (select) // Send selected button, if encoder pushed -------------------------------------------
  {
    Serial.println(buttons[ind]);
    if (buttons[ind] == 'C') // Clear previous result ***************************
    {
      line = "";
      num1 = "";
      num2 = "";
      operation = 0;
      calculated = false;
      operatorSet = false;
    }

    else if (buttons[ind] == '=') // Do calculation *****************************
    {
      calculate(); // Do calculation <--------------
    }
    else // Every other button pressed (numbers) ********************************
    {
      if (calculated || line == "syntax error") // Clear previous result, if not done before or error
      {
        line = "";
        num1 = "";
        num2 = "";
        operation = 0;
        calculated = false;
        operatorSet = false;
      }
      line = line + buttons[ind]; // Add number
    }

    // Error, if more than one operand per calculation used *************************
    if (buttons[ind] == '+' || buttons[ind] == '-' || buttons[ind] == '*' || buttons[ind] == '/')
    {
      if (operatorSet)
      {
        line = "syntax error";
      }
      operatorSet = true;
    }

    ind = 0; // Jump selection back to "0"
  }

  // clear screen & display new content ------------------------------------------------------------
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  // Show button matrix ----------------------------------
  for (int i = 0; i < 17; i++)
  {
    display.drawString(mx[i] + offsetX, my[i] + offsetY, String(buttons[i])); // Show keyboard button matrix
  }
  display.drawRect(mx[ind] + offsetX - 3, my[ind] + offsetY + 1, matrixSize, matrixSize); // Show rectangle around selected keyboard button

  display.drawRect(0, 0, 128, 18); // Show big rectangle for result
  display.setFont(ArialMT_Plain_16);
  display.drawString(2, 0, String(line)); // Show input line or result

  display.setFont(ArialMT_Plain_10);
  display.drawString(70, 28, "two positive");
  display.drawString(70, 39, "operands");
  display.drawString(70, 49, "only!");

  display.display();
}
