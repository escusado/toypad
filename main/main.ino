// Setup (good unrelated guide https://www.youtube.com/watch?v=5yjdPbCac9g&t=224s)
// Install Arduino IDE https://www.arduino.cc/en/guide
// Install Teensyduino https://www.pjrc.com/teensy/teensyduino.html
// Key codes are there too https://www.pjrc.com/teensy/td_keyboard.html
// Install Adafruit seesaw lib https://learn.adafruit.com/neokey-1x4-qt-i2c/arduino#library-installation-3098587-6

// Initial Imports
#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"

// Adafruit seesaw addressing config
#define SS_SWITCH 24
#define SS_NEOPIX 6
#define SEESAW_ADDR 0x36

// Get access to the devices
Adafruit_NeoKey_1x4 neokey;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);

int32_t encoderPosition;          // keep track of the rotation direction
bool pressed = false;             // keep keypad presses as single input
bool rotaryPressed = false;       // keep rotary encoder presses as single input
bool currentRotaryToggle = false; // keep track of 'e' and 'b' switching
uint8_t currentLedColor = 0;      // tracks the colors of the LEDs cycle

// Device startup
void setup()
{
  // begin serial communication (to print in Arduino IDE Serial Monitor window)
  Serial.begin(9600);
  while (!Serial)
    delay(10);

  Serial.println("Looking for seesaw...");
  if (!ss.begin(SEESAW_ADDR) || !sspixel.begin(SEESAW_ADDR))
  {
    Serial.println("Couldn't find seesaw on default address");
    while (1)
      delay(10);
  }
  Serial.println("seesaw started");

  Serial.println("Starting for NeoKey...");
  if (!neokey.begin(0x30))
  {
    Serial.println("Could not start NeoKey, check wiring?");
    while (1)
      delay(10);
  }
  Serial.println("NeoKey started!");

  Serial.println("Turning on interrupts for rotary encoder...");
  delay(10);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);       
  encoderPosition = ss.getEncoderPosition(); // Get starting position for rotary encoder
  Serial.println("Rotary encoder started");

  // Leds statup sequence on boot (scroll animation)
  for (uint16_t i = 0; i < neokey.pixels.numPixels(); i++)
  {
    neokey.pixels.setPixelColor(i, Wheel(map(i, 0, neokey.pixels.numPixels(), 0, 255)));
    neokey.pixels.show();
    delay(50);
  }
  for (uint16_t i = 0; i < neokey.pixels.numPixels(); i++)
  {
    neokey.pixels.setPixelColor(i, 0x000000);
    neokey.pixels.show();
    delay(50);
  }
  Serial.println("Toypad ON! Lets Fucking GOOOO!");
}

// The magic happens here every 10 milliseconds
void loop()
{

  // Rotary encoder button
  if (!ss.digitalRead(SS_SWITCH) && rotaryPressed == false)
  {
    if (currentRotaryToggle)
    {
      Keyboard.set_key1(KEY_E);
      Keyboard.send_now();
      currentRotaryToggle = false;
    }
    else
    {
      Keyboard.set_key1(KEY_B);
      Keyboard.send_now();
      currentRotaryToggle = true;
    }
    rotaryPressed = true;
    Serial.println("Rotary encoder button");
  }
  if (ss.digitalRead(SS_SWITCH))
  {
    rotaryPressed = false;
  }

  // Rotary encoder Rotation
  int32_t newPosition = ss.getEncoderPosition();
  if (encoderPosition != newPosition)
  {
    Serial.println(newPosition);
    sspixel.setPixelColor(0, Wheel(newPosition & 0xFF));
    sspixel.show();
    if (encoderPosition - newPosition < 0) // check for left/right
    {
      Keyboard.set_key1(KEY_RIGHT_BRACE);
    }
    else
    {
      Keyboard.set_key1(KEY_LEFT_BRACE);
    }
    Keyboard.send_now();
    encoderPosition = newPosition; // and save for next loop
  }

  // 4 Keys Keypad
  uint8_t buttons = neokey.read();
  for (int i = 0; i < neokey.pixels.numPixels(); i++)
  {
    neokey.pixels.setPixelColor(i, Wheel(((i * 256 / neokey.pixels.numPixels()) + currentLedColor) & 255));
  }

  if (buttons == 0x0000)
  {
    pressed = false;
  }

  if (pressed == false)
  {

    // Button 1 Undo
    if (buttons & (1 << 0))
    {
      Keyboard.set_modifier(MODIFIERKEY_GUI);
      Keyboard.set_key1(KEY_Z);
      Keyboard.send_now();

      pressed = true;
      Serial.println("Button A");
    }
    else
    {
      neokey.pixels.setPixelColor(0, 0);
    }

    // Button 2 Redo
    if (buttons & (1 << 1))
    {
      Keyboard.set_modifier(MODIFIERKEY_GUI | MODIFIERKEY_SHIFT);
      Keyboard.set_key1(KEY_Z);
      Keyboard.send_now();

      pressed = true;
      Serial.println("Button B");
    }
    else
    {
      neokey.pixels.setPixelColor(1, 0);
    }

    // Button 3 Cut/Paste
    if (buttons & (1 << 2))
    {
      Keyboard.set_modifier(MODIFIERKEY_GUI);
      Keyboard.set_key1(KEY_X);
      Keyboard.send_now();
      Keyboard.set_key1(KEY_V);
      Keyboard.send_now();

      pressed = true;
      Serial.println("Button C");
    }
    else
    {
      neokey.pixels.setPixelColor(2, 0);
    }

    // Button 3 Color Switching
    if (buttons & (1 << 3))
    {
      Keyboard.set_key1(KEY_X);
      Keyboard.send_now();

      pressed = true;
      Serial.println("Button D");
    }
    else
    {
      neokey.pixels.setPixelColor(3, 0);
    }
  }

  // Prepare keyboard state for next cycle
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.send_now();

  // Turn on leds
  neokey.pixels.show();
  currentLedColor++; // make colors cycle

  delay(10); // main loop delay
}

/************** UTIL *********************/

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85)
  {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}
