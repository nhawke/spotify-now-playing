#include <Wire.h>
#include <SerLCD.h>

#define BUFSIZ 256
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

      Serial.print((char *)buf);
      printBuffer();
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

void printBuffer()
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
      Serial.print(buf[i], HEX);
      Serial.print(' ');
      ++column;
    }
  }
}
