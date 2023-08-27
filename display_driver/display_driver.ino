#include <Wire.h>
#include <SerLCD.h>

#define BUFSIZ 128
#define EOF 4

SerLCD lcd;
char buf[BUFSIZ];
int pos = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
  Wire.begin();
  lcd.begin(Wire);

  setupLCD();

  lcd.clear();

  requestInfo();
}

void loop()
{
  if (Serial.available() > 0)
  {
    char in = Serial.read();
    buf[pos++] = in;
    if (in == EOF)
    {
      // An EOF shouldn't count towards the length of the data so we decrement pos.
      buf[--pos] = '\0';

      printBuffer();
      displayBuffer();
      pos = 0;
    }
  }
}

void setupLCD()
{
  lcd.disableSplash();
  lcd.disableSystemMessages();
  lcd.setBacklight(255, 255, 255); // white
  lcd.setContrast(5);
  lcd.noCursor();
}

void displayBuffer()
{
  lcd.clear();

  int row = 0;
  int column = 0;
  for (int i = 0; i < pos; i++)
  {
    char c = buf[i];
    if (c == '\n')
    {
      ++row;
      lcd.setCursor(0, row);
      column = 0;
    }
    else if (column < MAX_COLUMNS)
    {
      lcd.print(buf[i]);
      ++column;
    }
  }
}

void printBuffer()
{
  int columns = 64;
  for (int i = 0; i < BUFSIZ; ++i)
  {
    char c = buf[i];
    switch (c)
    {
    case '\n':
      Serial.write("\\n ");
      break;
    case '\0':
      Serial.write("\\0 ");
      break;
    default:
      Serial.write(c);
      Serial.write("  ");
    }
    if (i % columns == columns - 1)
    {
      Serial.write('\n');
    }
  }
  Serial.write('\n');
}

void requestInfo()
{
  Serial.println("READY");
}