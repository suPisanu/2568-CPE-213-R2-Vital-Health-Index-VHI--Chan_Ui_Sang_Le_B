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

volatile int direction = CW;      // ทิศทางหมุน encoder
volatile int height_ = 0;         // ค่าความสูง
volatile int weight_ = 0;         // ค่าน้ำหนัก
volatile bool updateFlag = false; // flag สำหรับอัปเดตหน้าจอ

ezButton button(SW_PIN); // ปุ่มกด encoder

//-------------------------------//
//  LED AND BUZZER  //
//-------------------------------//
#define LED_PIN1 12
#define LED_PIN2 13
#define BUZZER 14
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

//-------------------------//
//       SPo2 Sensor       //
//-------------------------//
PulseOximeter pox;
//-------------------------//

//-------------------------//
//      menu parameter     //
//-------------------------//
int menu = 0;
int inUse = 0;
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

//-------------------------------------//
//        Body Fat Mode parameter      //
//-------------------------------------//
int BodyFatMode = 0;
int height_done = 0;
int weight_done = 0;
//-------------------------------------//

//------------------------------//
// Encoder Interrupt Function   //
//------------------------------//
volatile unsigned long lastEnc = 0;
const int encDebounce = 5; // ms

void IRAM_ATTR handleEncoder()
{
  unsigned long now = millis();
  if (now - lastEnc > encDebounce)
  { // debounce
    int CLK_state = digitalRead(CLK_PIN);
    int DT_state = digitalRead(DT_PIN);

    if (CLK_state == HIGH)
    { // rising edge
      if (DT_state != CLK_state)
      {
        direction = CW; // clockwise
        if (height_done == 0)
          height_ -= 10;
        else if (weight_done == 0)
          weight_ -= 10;
      }
      else
      {
        direction = CCW; // counter-clockwise
        if (height_done == 0)
          height_ += 10;
        else if (weight_done == 0)
          weight_ += 10;
      }
      updateFlag = true; // flag ให้ main loop อัปเดต display
    }
    lastEnc = now;
  }
}

//------------------------------//
// Button press tracking        //
//------------------------------//
bool buttonProcessed = false; // เช็คว่าการกดปัจจุบันได้ถูกประมวลผลแล้วหรือยัง

void setup()
{
  Serial.begin(9600);

  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Initialize Rotary Encoder
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(CLK_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), handleEncoder, CHANGE); // ใช้ interrupt

  button.setDebounceTime(350);

  // Initialize SSD1306 OLED 128x64
  display.begin(SSD1306_SWITCHCAPVCC, I2C_addr);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
  display.display();

  // Initialize MAX30100 Sensor
  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_30_6MA);
}

// Callback เมื่อหัวใจเต้น
void onBeatDetected()
{
  if (menu == 0 && HeartRateMode == 1 && inUse == 1)
  {
    digitalWrite(LED_PIN1, HIGH);
    digitalWrite(LED_PIN2, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(10);
  }
}

void loop()
{
  unsigned long now = millis();
  unsigned long curr_t = millis();
  button.loop();
  pox.update();

  pox.setOnBeatDetectedCallback(onBeatDetected);

  digitalWrite(LED_PIN1, LOW);
  digitalWrite(LED_PIN2, LOW);
  digitalWrite(BUZZER, LOW);

  //-------------------------------//
  // Update display หาก encoder หมุน
  //-------------------------------//
  if (updateFlag && inUse == 0)
  {
    updateFlag = false;
    display.clearDisplay();
    if (direction == CW)
    {
      display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE); // heart rate menu
      menu = 0;
    }
    else if (direction == CCW)
    {
      display.drawBitmap(0, 0, bitmaps[1], 128, 64, SSD1306_BLACK, SSD1306_WHITE); // body fat menu
      menu = 1;
    }
    display.display();
  }

  //-------------------------------//
  // Button press logic "wait release"
  //-------------------------------//
  if (button.isPressed() && !buttonProcessed)
  {
    buttonProcessed = true; // mark ว่ากดแล้ว

    if (menu == 0 && inUse == 0)
    {
      HeartRateMode = 1;
      inUse = 1;
    }
    else if (menu == 1 && inUse == 0)
    {
      BodyFatMode = 1;
      inUse = 1;
    }
    else if (measure_done)
    {
      measure_done = 0;
      HeartRateMode = 0;
      inUse = 0;
      menu = -1;
      spo2_sum = 0;
      hr_sum = 0.0f;
      display.clearDisplay();
      display.drawBitmap(0, 0, bitmaps[0], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
      display.display();
    }
    else if (height_done && weight_done)
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

    else if (height_done == 0 && weight_done == 0)
    {
      height_done = 1; 
    }
    else if (height_done == 1 && weight_done == 0)
    {
      weight_done = 1; 
    }
  }

  if (button.isReleased())
  {
    buttonProcessed = false; // ปล่อยปุ่มแล้ว reset flag
  }

  //-------------------------------//
  // Heart Rate Mode
  //-------------------------------//
  if (menu == 0 && HeartRateMode == 1 && inUse == 1)
  {
    if (pox.getHeartRate() <= 0 && pox.getSpO2() <= 0)
    {
      for (int index = 5; index < 8; index++)
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
        }

        tsLastReport = curr_t;

        for (int index = 2; index < 5; index++)
        {
          display.clearDisplay();
          display.drawBitmap(0, 0, bitmaps[index], 128, 64, SSD1306_BLACK, SSD1306_WHITE);
          display.display();
        }

        if (counter == 10)
        {
          HeartRateMode = 0;
          measure_done = 1;
          counter = 0;
          onFinger = false;
        }
      }
    }

    if (measure_done)
    {
      float hr_avg = hr_sum / 10.0f;
      uint8_t spo2_avg = spo2_sum / 10;

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);

      display.setCursor(0, 0);
      display.print("Health Monitor");
      display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

      display.setCursor(0, 20);
      display.printf("HeartRate: %.1f bpm\n", hr_avg);
      if (hr_avg < 60)
        display.print("           (Low)");
      else if (hr_avg > 100)
        display.print("           (High)");
      else
        display.print("           (Normal)");

      display.setCursor(0, 42);
      display.printf("SpO2     : %d %%\n", spo2_avg);
      if (spo2_avg < 95)
        display.print("           (Low)");
      else
        display.print("           (Normal)");

      display.drawLine(0, 60, 127, 60, SSD1306_WHITE);
      display.display();
    }
  }

  //-------------------------------//
  // Body Fat Mode
  //-------------------------------//
  if (menu == 1 && BodyFatMode == 1 && inUse == 1)
  {
    display.clearDisplay();

    // Step 1: Height input
    if (height_done == 0 && weight_done == 0)
    {
      if (height_ < 0)
        height_ = 0;
      display.drawLine(0, 5, 127, 5, SSD1306_WHITE);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(5, 20);
      display.printf("Height:%d", height_);
      display.drawLine(0, 55, 127, 55, SSD1306_WHITE);
      display.display();
    }
    // Step 2: Weight input
    else if (height_done == 1 && weight_done == 0)
    {
      if (weight_ < 0)
        weight_ = 0;
      display.clearDisplay();
      display.drawLine(0, 5, 127, 5, SSD1306_WHITE);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(5, 20);
      display.printf("Weight:%d", weight_);
      display.drawLine(0, 55, 127, 55, SSD1306_WHITE);
      display.display();
    }
    // Step 3: BMI result
    else if (height_done == 1 && weight_done == 1)
    {
      float h_m = height_ / 100.0f;
      float bmi = weight_ / (h_m * h_m);

      display.clearDisplay();
      display.drawLine(0, 5, 127, 5, SSD1306_WHITE);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(5, 20);
      display.printf("BMI: %.1f", bmi);

      display.setTextSize(1);
      display.setCursor(30, 40);
      if (bmi < 18.5)
        display.print("Underweight");
      else if (bmi < 25)
        display.print("Normal");
      else if (bmi < 30)
        display.print("Overweight");
      else
        display.print("Obese");
      display.drawLine(0, 60, 127, 60, SSD1306_WHITE);

      display.display();
    }
  }
}
