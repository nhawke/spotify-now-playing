#include <Wire.h>
#include <SerLCD.h>

#define LINESIZ 128
#define NUMLINES 4
#define NUMCOLUMNS 20

#define DEBUG_BUFFER 1

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
    // New data to read
    stopTicker();
    readData();
    printBuffer();
    startTicker();
  }

  int t = tick();
  if (t >= 0)
  {
    displayBuffer(t % 2);
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
    char sendBuf[LINESIZ + 1];
    int pos = 0;

    if (lines[i].pos <= NUMCOLUMNS && scroll_offset != 0)
    {
      // this line doesn't need to be scrolled, leave it as is
      continue;
    }

    for (int j = scroll_offset; pos < NUMCOLUMNS && j < lines[i].pos; ++j)
    {
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
    sendBuf[pos] = '\0';
    lcd.setCursor(0, i);
    lcd.print(&sendBuf[0]);
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

// Ticker related things
#define TICK_DURATION_MS 2000
unsigned long lastTick;
bool ticking;
int tickCount = 0;

void startTicker()
{
  ticking = true;
  tickCount = 0;
  lastTick = millis();
}

void stopTicker()
{
  ticking = false;
}

// tick returns the tick count since the ticker was started with
// startTicker(). If the ticker has not been started, returns -1.
int tick()
{
  if (ticking && millis() - lastTick >= TICK_DURATION_MS)
  {
    lastTick = millis();
    return ++tickCount;
  }
  return -1;
}