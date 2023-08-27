#include <Wire.h>
#include <SerLCD.h>

#define LINESIZ 64
#define NUMLINES 4
#define NUMCOLUMNS 20

// #define DEBUG_BUFFER 1

SerLCD lcd;

struct linebuf
{
  char buf[LINESIZ];
  int pos;
};
struct linebuf lines[NUMLINES];
int lineno = 0;

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

  Serial.println("READY");
}

void loop()
{
  if (Serial.available() > 0)
  {
    char in = Serial.read();
    struct linebuf *line = &lines[lineno];

    line->buf[line->pos++] = in;
    if (in == '\n')
    {
      // Replace newline with terminator, and ensure it doesn't count towards the string length.
      line->buf[--line->pos] = '\0';
      ++lineno;
    }
    if (lineno == NUMLINES)
    {
      printBuffer();
      displayBuffer();
      resetBuffer();
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

  for (int i = 0; i < NUMLINES; ++i)
  {
    lcd.setCursor(0, i);
    struct linebuf *line = &lines[i];
    for (int j = 0; j < line->pos; ++j)
    {
      if (j == NUMCOLUMNS)
      {
        break;
      }
      char c = line->buf[j];
      lcd.print(c);
      if (c == '|')
      {
        // Pipe characters trigger command mode, so must be escaped.
        lcd.print(c);
      }
    }
  }
}

void printBuffer()
{
#ifdef DEBUG_BUFFER
  for (int i = 0; i < NUMLINES; ++i)
  {
    for (int j = 0; j < LINESIZ; ++j)
    {
      char c = lines[i].buf[j];
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
    }
    Serial.write('\n');
  }
  Serial.write('\n');
#endif
}

void resetBuffer()
{
  for (int i = 0; i < NUMLINES; ++i)
  {
    lines[i].pos = 0;
  }
  lineno = 0;
}