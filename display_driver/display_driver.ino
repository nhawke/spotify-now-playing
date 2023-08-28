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
  while (Serial.available() > 0)
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
      displayBuffer();
      printBuffer();
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
    char sendBuf[LINESIZ];
    int pos = 0;
    for (int j = 0, len = lines[i].pos; j < len; ++j)
    {
      if (j == NUMCOLUMNS || pos == LINESIZ)
      {
        break;
      }
      char c = lines[i].buf[j];
      sendBuf[pos++] = c;
      if (c == '|')
      {
        // Escape pipe characters, which trigger command mode on OpenLCD firmware.
        if (pos < LINESIZ)
        {
          // Double the pipe character if there's room.
          sendBuf[pos++] = c;
        }
        else
        {
          // Ignore the previous pipe if there's no room for a second one.
          --pos;
          break;
        }
      }
    }
    lcd.write((uint8_t *)&sendBuf[0], pos);
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