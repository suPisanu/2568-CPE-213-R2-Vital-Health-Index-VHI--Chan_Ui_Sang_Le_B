#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ezButton.h>
#include <MAX30100_PulseOximeter.h>
#include "imgs.h"

//------------------------------//
//   Rotary Encoder Parameter   //
//------------------------------//
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
//-------------------------------//
//END OF Rotary Encoder Parameter//
//-------------------------------//


//-------------------------------------//
//         SSD1306 OLED 128x64         //
//------------------------------------//
  #define I2C_addr 0x3C
  #define Width 128
  #define Height 64
  #define OLED_rst -1
Adafruit_SSD1306 display(Width, Height, &Wire, OLED_rst);
//------------------------------------//
//END OF SSD1306 OLED 128x64 Parameter//
//------------------------------------//


//-------------------------//
//       SPo2 Sensor       //
//-------------------------//
 PulseOximeter pox;
//-------------------------//
//END OF MAX30100 Parameter//
//-------------------------//


//-------------------------//
//      menu parameter     //
//-------------------------//
  int menu = 0;
  int inUse = 0;
//-------------------------//
//  END OF menu Parameter  //
//-------------------------//


//--------------------------------------//
//       Heart Rate Mode parameter     //
//-------------------------------------//
  int counter = 0;
  int measure_done = 0;
  int HeartRateMode = 0;

  bool onFinger = false;

  const unsigned long REPORTING_PERIOD_MS = 1000;
  unsigned long tsLastReport = 0;

  float hr_sum = 0.0f;
  uint16_t spo2_sum = 0;
//------------------------------------//
//  END OF Heart Rate Mode Parameter  //
//------------------------------------//


//-------------------------------------//
//        Body Fat Mode parameter      //
//-------------------------------------//
  int BodyFatMode = 0;
  int height_done = 0;
  int weight_done = 0;
  int height_ = 0;
  int weight_ = 0;
//-------------------------------------//
//  END OF Body Fat Mode Mode Parameter//
//-------------------------------------//

void setup() 
{
  
  Serial.begin(9600);

//Initialize Rotary Encoder//
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(CLK_PIN, INPUT_PULLUP);
  button.setDebounceTime(350);
  prev_CLK_state = digitalRead(CLK_PIN);
//------------------------//

//Initialize SSD1306 OLED 128x64//
  display.begin(SSD1306_SWITCHCAPVCC, I2C_addr);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
  display.display();
//------------------------------//

//Initialize MAX30100 Sensor//
  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_30_6MA); //Set this to adjust IR light current. 
//------------------------------//
}

void loop() 
{
//-------------------------------------------------------------------------------------------------------------//
//-------------------------------BEGIN OF CONSTANT PARAMETER SECTION-------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
  unsigned long now = millis();
  unsigned long curr_t = millis();
  button.loop();
  pox.update();
//-------------------------------------------------------------------------------------------------------------//
//-------------------------------END OF CONSTANT PARAMETER SECTION---------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//



//-------------------------------------------------------------------------------------------------------------//
//-------------------------------BEGIN OF ROTARY ENCODER LOGIC SECTION-----------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
  CLK_state = digitalRead(CLK_PIN);
  if (CLK_state != prev_CLK_state)  //handle encoder direction
  {
    if (now - lastEnc > encDebounce) //debounce
    {
      if (CLK_state == HIGH) 
      {
        if(digitalRead(DT_PIN) != CLK_state)
        {
          direction = CW;
          height_ -= 10; //handle height / weight logic
          
          if(height_done == 1 && weight_done == 0)
          {
            weight_ += 10;
          }
        } 
        else 
        {
          direction = CCW;
          height_ += 10; //handle height / weight logic
          
          if(height_done == 1 && weight_done == 0)
          {
            weight_ += 10;
          }
        }
      }
      lastEnc = now;
    }

    if(inUse == 0) //check if not busy
    {
      if(direction == CW) 
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE); //switch to heart rate menu
        menu = 0;
      }
      else if(direction == CCW)
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[1], 128, 64, SSD1306_BLACK, SSD1306_WHITE); //switch to body fat menu
        menu = 1;
      }
      display.display();
    }
  }
  prev_CLK_state = CLK_state;
//-----------------------------------------------------------------------------------------------------------//
//-------------------------------END OF ROTARY ENCODER LOGIC SECTION-----------------------------------------//
//----------------------------------------------------------------------------------------------------------//



//-------------------------------------------------------------------------------------------------------------//
//-------------------------------BEGIN OF MENU SELECTOR LOGIC SECTION------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
  if(menu == 0 && button.isPressed())
  {
    HeartRateMode = 1;
    inUse = 1;
  }
  if(menu == 1 && button.isReleased()) //isRelease for some reason but it make code work perfectly as its should be.
  {
    BodyFatMode = 1;
    inUse = 1;
  }
//-----------------------------------------------------------------------------------------------------------//
//-------------------------------END OF MENU SELECTOR LOGIC SECTION-----------------------------------------//
//----------------------------------------------------------------------------------------------------------//



//---------------------------------------------------------------------------------------------------------------//
//-------------------------------BEGIN OF HEART RATE MODE LOGIC SECTION-----------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
  if(menu == 0 && HeartRateMode == 1 && inUse == 1)
  {    
    if (pox.getHeartRate() <= 0 && pox.getSpO2() <= 0) //if not detect finger.
    {
      for(int index = 5; index < 8; index++)
      {
        display.clearDisplay();
        display.drawBitmap(0, 0, bitmaps[index], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
        display.display();
      }  
    }
    else //if detect finger.
    {
      if (curr_t - tsLastReport >= REPORTING_PERIOD_MS)  //for sensor to sense every 1 sec.
      {
        uint8_t spo2 = pox.getSpO2();
        float hr = pox.getHeartRate();

        if (spo2 > 0 && hr > 0) //record only valid values.
        {
          spo2_sum += spo2;
          hr_sum += hr;
          counter += 1;
          Serial.println(spo2_sum);
          Serial.println(hr_sum);
        }

        tsLastReport = curr_t;

        for(int index = 2; index < 5; index++) //measure menu...
        {
          display.clearDisplay();
          display.drawBitmap(0, 0, bitmaps[index], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
          display.display();
        }

        if(counter == 10) //measure for 10 time then show result
        {
          HeartRateMode = 0;
          measure_done = 1;
          counter = 0;
          onFinger = false;
        }
      }
    }

    if(measure_done) //result page.
    {
      display.clearDisplay();
      float hr_avg = hr_sum/10.0f;
      uint8_t spo2_avg = spo2_sum/10;
      
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.printf("HeartRate:%.2fbpm\n", hr_avg);
      display.printf("SPo2:%d%%", spo2_avg);
      display.display();

      if(button.isPressed()) //reset to go to main menu.
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
//--------------------------------------------------------------------------------------------------------//
//-------------------------------END OF HEART RATE LOGIC SECTION-----------------------------------------//
//------------------------------------------------------------------------------------------------------//



//-------------------------------------------------------------------------------------------------------------//
//-------------------------------BEGIN OF BODY FAT MODE LOGIC SECTION-----------------------------------------//
//-----------------------------------------------------------------------------------------------------------//
  if(menu == 1 && BodyFatMode == 1 && inUse == 1)
  {
    display.clearDisplay();

    if(height_done == 0 && weight_done == 0) //get height from user.
    {
      if(height_ < 0) //not to go below 0
      {
        height_ = 0;
      }
      display.setCursor(0, 0);
      display.printf("height:%d", height_);
      display.display();
      if(button.isPressed()) //confirmation button then move to next page.
      {
        height_done = 1;
      }
    }
    else if(height_done == 1 && weight_done == 0) //get weight from user.
    {
      if(weight_ < 0) //not to go below 0
      {
        weight_ = 0;
      }
      display.setCursor(0, 0);
      display.printf("weight:%d", weight_);
      display.display();

      if(button.isPressed()) //confirmation button then move to next page.
      {
        weight_done = 1;
      }
    }
    else if(height_done == 1 && weight_done == 1) //display result
    {
      int bmi = weight_ / ((height_/100) * (height_/100));
      display.setCursor(0, 0);
      display.printf("BMI : %d", bmi);
      display.display();

      if (button.isPressed()) //reset to go to main menu.
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
//-----------------------------------------------------------------------------------------------------------//
//-------------------------------END OF BODY FAT MODE LOGIC SECTION-----------------------------------------//
//---------------------------------------------------------------------------------------------------------//



//------------------------------------------------------------------------------------//
//-------------------------------END OF CODE-----------------------------------------//
//------------------------------------------------------------------------------------//
}
