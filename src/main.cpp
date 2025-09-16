#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ezButton.h>
#include <MAX30100_PulseOximeter.h>
#include "imgs.h"

#define SW_PIN 4
#define DT_PIN 2
#define CLK_PIN 15
#define CW 0
#define CCW 1
int direction = CW;
int CLK_state;
int prev_CLK_state;
unsigned long lastEnc = 0;
const int encDebounce = 5; // ms
ezButton button(SW_PIN);

#define I2C_addr 0x3C
#define Width 128
#define Height 64
#define OLED_rst -1
Adafruit_SSD1306 display(Width, Height, &Wire, OLED_rst);

PulseOximeter pox;

int menu = 0;
int counter = 0;
int measure_done = 0;
int HeartRateMode = 0;
int BodyFatMode = 0;
int inUse = 0;
int height_done = 0;
int weight_done = 0;
bool onFinger = false;
const unsigned long REPORTING_PERIOD_MS = 1000;
unsigned long tsLastReport = 0;
uint16_t spo2_sum = 0;
float hr_sum = 0.0f;
int height_ = 0;
int weight_ = 0;

void setup() 
{
  Serial.begin(9600);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(CLK_PIN, INPUT_PULLUP);
  button.setDebounceTime(350);
  prev_CLK_state = digitalRead(CLK_PIN);

  display.begin(SSD1306_SWITCHCAPVCC, I2C_addr);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
  display.display();

  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_30_6MA);
}

void loop() 
{
  unsigned long now = millis();
  unsigned long curr_t = millis();
  button.loop();
  pox.update();
  CLK_state = digitalRead(CLK_PIN);
    
  if (CLK_state != prev_CLK_state) 
  {
    if (now - lastEnc > encDebounce) 
    {
      if (CLK_state == HIGH) 
      {
        if(digitalRead(DT_PIN) != CLK_state)
        {
          direction = CW;
          height_ -= 10;
          
          if(height_done == 1 && weight_done == 0)
          {
            weight_ += 10;
          }
        } 
        else 
        {
          direction = CCW;
          height_ += 10;
          
          if(height_done == 1 && weight_done == 0)
          {
            weight_ += 10;
          }
        }
      }
      lastEnc = now;
    }

    if(inUse == 0)
    {
      if(direction == CW)
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        menu = 0;
      }
      else if(direction == CCW)
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[1], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        menu = 1;
      }
      display.display();
    }
  }
  prev_CLK_state = CLK_state;

  if(menu == 0 && button.isPressed())
  {
    HeartRateMode = 1;
    inUse = 1;
  }
  if(menu == 1 && button.isReleased())
  {
    BodyFatMode = 1;
    inUse = 1;
  }

  if(menu == 0 && HeartRateMode == 1 && inUse == 1)
  {    
    if (pox.getHeartRate() <= 0 && pox.getSpO2() <= 0) 
    {
      for(int index = 5; index < 8; index++)
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[index], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        display.display();
      }  
    }
    else 
    {
      if (curr_t - tsLastReport >= REPORTING_PERIOD_MS) 
      {
        uint8_t spo2 = pox.getSpO2();
        float hr = pox.getHeartRate();

        if (spo2 > 0 && hr > 0)
        {
          spo2_sum += spo2;
          hr_sum += hr;
          counter += 1;
          Serial.println(spo2_sum);
          Serial.println(hr_sum);
        }

        tsLastReport = curr_t;

        for(int index = 2; index < 5; index++)
        {
          display.clearDisplay();
          display.drawBitmap(0, 0, bitmaps[index], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
          display.display();
        }

        if(counter == 10)
        {
          HeartRateMode = 0;
          measure_done = 1;
          counter = 0;
          onFinger = false;
        }
      }
    }

    if(measure_done)
    {
      display.clearDisplay();
      float hr_avg = hr_sum/10.0f;
      uint8_t spo2_avg = spo2_sum/10;
      
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.printf("HeartRate:%.2fbpm\n", hr_avg);
      display.printf("SPo2:%d%%", spo2_avg);
      display.display();

      if(button.isPressed())
      {
        measure_done = 0;
        HeartRateMode = 0;
        inUse = 0;
        menu = -1;
        onFinger = false;
        spo2_sum = 0;
        hr_sum = 0.0f;
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        display.display();
      }
    }
  }

  if(menu == 1 && BodyFatMode == 1 && inUse == 1)
  {
    display.clearDisplay();

    if(height_done == 0 && weight_done == 0)
    {
      if(height_ < 0)
      {
        height_ = 0;
      }
      display.setCursor(0, 0);
      display.printf("height:%d", height_);
      display.display();
      if(button.isPressed())
      {
        height_done = 1;
      }
    }
    else if(height_done == 1 && weight_done == 0)
    {
      if(weight_ < 0)
      {
        weight_ = 0;
      }
      display.setCursor(0, 0);
      display.printf("weight:%d", weight_);
      display.display();

      if(button.isPressed())
      {
        weight_done = 1;
      }
    }
    else if(height_done == 1 && weight_done == 1) 
    {
      int bmi = weight_ / ((height_/100) * (height_/100));
      display.setCursor(0, 0);
      display.printf("BMI : %d", bmi);
      display.display();

      if (button.isPressed())
      {
        BodyFatMode = 0;
        inUse = 0;
        height_done = 0;
        weight_done = 0;
        height_ = 0;
        weight_ = 0;
        menu = -1;
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[1], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        display.display();
      }
    }
  }
}
