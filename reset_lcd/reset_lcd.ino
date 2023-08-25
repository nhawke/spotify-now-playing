// Upload with SerLCD attached to Arduino via serial with GND disconnected.
// Once running, start Serial Monitor. Follow instructions in Serial Monitor.

void setup()
{
  Serial.begin(9600); // Begin serial communication.
  while (!Serial)
  {
    ;
  }
}

void lcdReset()
{                     // New Function
  Serial.write(0xFE); // Command code
  Serial.write(0x72); // Reset code
}

void cls()
{                     // New Function
  Serial.write(0xFE); // Command Code
  Serial.write(0x01); // Clear Screen Code
}

void loop()
{
  Serial.print("Attach GND now"); // Note: You can attach the GND while Reset is being sent
  delay(1000);

  for (int i = 0; i < 15; i++)
  {
    lcdReset();
    delay(500);
  }

  cls();
  Serial.print("LCD Reset! Remove GND"); // You have 21 seconds to remove GND before Reset is sent again.
  delay(20000);
}