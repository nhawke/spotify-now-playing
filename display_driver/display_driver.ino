#include <Wire.h>
#include <SerLCD.h>

#define LINESIZ 128
#define NUMLINES 4
#define NUMCOLUMNS 20
#define MAX_SCROLL_AMOUNT 10

// #define DEBUG_BUFFER 1

SerLCD lcd;

struct linebuf
{
  char buf[LINESIZ];
  int pos;
};
struct linebuf lines[NUMLINES];

void setup()
{
  Serial.begin(115200);
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
    stopTicker();
    readData();
    printBuffer();
    startTicker();
  }

  int t = tick();
  if (t >= 0)
  {
    displayBuffer(t % MAX_SCROLL_AMOUNT);
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

void readData()
{
  int lineno = 0;
  while (lineno < NUMLINES)
  {
    String in = Serial.readStringUntil('\n');
    struct linebuf *line = &lines[lineno];

    memcpy(&line->buf[0], in.c_str(), in.length());
    line->pos = in.length();
    ++lineno;
  }
}

void displayBuffer(int scroll_offset)
{
  for (int i = 0; i < NUMLINES; ++i)
  {
    // Worst case, we need space to send a full line of escaped pipes + \0.
    char sendBuf[NUMCOLUMNS * 2 + 1];
    int sendPos = 0;
    if (lines[i].pos <= NUMCOLUMNS && scroll_offset != 0)
    {
      // This line doesn't need to be scrolled, so leave it as is.
      continue;
    }

    // for each column in the display, populate sendBuf with the bytes needed to
    // fill that column.
    int j;
    for (j = 0; j < NUMCOLUMNS; ++j)
    {
      int idx = j + scroll_offset;
      if (idx >= lines[i].pos)
      {
        // No string left.
        break;
      }
      char c = lines[i].buf[idx];
      sendBuf[sendPos++] = c;
      if (c == '|')
      {
        // Escape pipe characters, which trigger command mode on OpenLCD firmware.
        sendBuf[sendPos++] = c;
      }
    }
    // Pad the buffer with enough spaces to fill out the line
    int spaces = NUMCOLUMNS - j;
    memset(&sendBuf[sendPos], ' ', spaces);
    sendPos += spaces;
    sendBuf[sendPos] = '\0';

    printSendBuffer(sendBuf, sendPos);
    lcd.setCursor(0, i);
    lcd.write(&sendBuf[0]);
  }
}

void printSendBuffer(char buf[], int n)
{
#ifdef DEBUG_BUFFER
  Serial.print("SEND BUFFER LEN: ");
  Serial.println(n, DEC);
  for (int i = 0; i < n; ++i)
  {
    Serial.write(buf[i]);
    Serial.write("  ");
  }
  Serial.write('\n');
#endif
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

// Ticker related things
#define TICK_DURATION_MS 2000
unsigned long lastTickMs;
bool ticking;
int tickCount = 0;
int lastTickReturned = 0;

void startTicker()
{
  ticking = true;
  tickCount = 0;
  lastTickReturned = -1;
  lastTickMs = millis();
}

void stopTicker()
{
  ticking = false;
}

// tick returns the tick count since the ticker was started with
// startTicker(). If there hasn't been a new tick since the last call
// to tick or if the ticker has not been started, returns -1.
int tick()
{
  if (!ticking)
  {
    return -1;
  }
  if (millis() - lastTickMs >= TICK_DURATION_MS)
  {
    lastTickMs = millis();
    ++tickCount;
  }
  if (tickCount == lastTickReturned)
  {
    return -1;
  }
  lastTickReturned = tickCount;
  return tickCount;
}