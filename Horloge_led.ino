#include "LedControl.h"
#include "RTClib.h"
#include <Wire.h>

#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10

#define MAX_DEVICES 4
#define DELAY 50

#define button 3

#define BUFFER_SIZE 20
#define VERSION     "1.0"

LedControl lc = LedControl(DATA_PIN,CLK_PIN,CS_PIN,MAX_DEVICES);
RTC_DS3231 rtc;

byte data[MAX_DEVICES][8];

// 4 x 7
byte numeric[][8] = {
  {0xF0,0x90,0x90,0x90,0x90,0x90,0xF0,0x00}, // 0
  {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00}, // 1
  {0xF0,0x10,0x10,0xF0,0x80,0x80,0xF0,0x00}, // 2
  {0xF0,0x10,0x10,0x70,0x10,0x10,0xF0,0x00}, // 3
  {0x90,0x90,0x90,0xF0,0x10,0x10,0x10,0x00}, // 4
  {0xF0,0x80,0x80,0xF0,0x10,0x10,0xF0,0x00}, // 5
  {0xF0,0x80,0x80,0xF0,0x90,0x90,0xF0,0x00}, // 6
  {0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x00}, // 7
  {0xF0,0x90,0x90,0xF0,0x90,0x90,0xF0,0x00}, // 8
  {0xF0,0x90,0x90,0xF0,0x10,0x10,0xF0,0x00}, // 9
  {0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x00}, // :
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // empty
};

byte tempc[][8] = {
  {0x00,0xF8,0x20,0x20,0x20,0x20,0x20,0x00}, // T (5)
  {0x00,0x00,0x70,0x80,0x80,0x80,0x70,0x00}, // c (4)
  {0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x00}  // : (1)
};

// 3 x 5
byte numdate[][8] = {
  {0x00,0x00,0xE0,0xA0,0xA0,0xA0,0xE0,0x00}, // 0
  {0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00}, // 1
  {0x00,0x00,0xE0,0x20,0xE0,0x80,0xE0,0x00}, // 2
  {0x00,0x00,0xE0,0x20,0xE0,0x20,0xE0,0x00}, // 3
  {0x00,0x00,0xA0,0xA0,0xE0,0x20,0x20,0x00}, // 4
  {0x00,0x00,0xE0,0x80,0xE0,0x20,0xE0,0x00}, // 5
  {0x00,0x00,0xE0,0x80,0xE0,0xA0,0xE0,0x00}, // 6
  {0x00,0x00,0xE0,0x20,0x20,0x20,0x20,0x00}, // 7
  {0x00,0x00,0xE0,0xA0,0xE0,0xA0,0xE0,0x00}, // 8
  {0x00,0x00,0xE0,0xA0,0xE0,0x20,0xE0,0x00}, // 9
  {0x00,0x00,0x20,0x40,0x40,0x40,0x80,0x00}  // /
};

int buttonVal = 1;

char serial_buffer[BUFFER_SIZE];
int buffer_position;

void setup() {
  pinMode(button, INPUT_PULLUP);
  
  for (int i = 0; i < MAX_DEVICES; i++)
  {
    lc.shutdown(i,false);
    lc.setIntensity(i,5);
    lc.clearDisplay(i);
    for (int j = 0; j < 8; j++)
    {
      data[i][j] = numeric[11][j];
    }
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // If the RTC have lost power it will sets the RTC to the date & time this sketch was compiled in the following line
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.begin(57600);
  Wire.begin();
}

void loop() {
  if (!digitalRead(button)) 
  {
    changeDisplay();
    delay(500);
  }
  updateTime();
  fillData(true);
  display();
  delay(DELAY);
}

void fillData(boolean sep) 
{
  DateTime now = rtc.now();
  if (buttonVal == 1)
  {
    int num1 = now.hour() / 10;
    int num2 = now.hour() % 10;
    int num3 = now.minute() / 10;
    int num4 = now.minute() % 10;
    int num5 = now.second() / 10;
    int num6 = now.second() % 10;
    int separator = 10;
    if (!sep) {
      separator = 11;
    }
  
    for(int i = 0; i < 8; i++)
    {
      data[3][i] = (numeric[num1][i] >> 1) + (numeric[num2][i] >> 6);
      data[2][i] = (numeric[num2][i] << 2) + (numeric[separator][i] >> 2) + (numeric[num3][i] >> 3); //+ (numeric[num4][i] >> 7);
      data[1][i] = (numeric[num4][i]) + (numeric[separator][i] >> 4) + (numeric[num5][i] >> 5);
      data[0][i] = (numeric[num5][i] << 3) + (numeric[num6][i] >> 2);
    }
  }
  else if (buttonVal == 3)
  {
    int num1 = 0;
    int num2 = 0;
    int temperature = rtc.getTemperature();
    if (temperature < 10)
    {
      num2 = temperature;
    }
    else
    {
      num1 = temperature / 10;
      num2 = temperature % 10;
    }
    
    for(int i = 0; i < 8; i++)
    {
      data[3][i] = tempc[0][i] + (numdate[num1][i] >> 6);
      data[2][i] = (numdate[num1][i] << 2) + (numdate[num2][i] >> 2) + (tempc[1][i] >> 6);
      data[1][i] = (tempc[1][i] << 2);
      data[0][i] = numeric[11][i];
    }
  }
  else if (buttonVal == 2)
  {
    int jour1 = now.day() / 10;
    int jour2 = now.day() % 10;
    int mois1 = now.month() / 10;
    int mois2 = now.month() % 10;
    int annee1 = (now.year() % 100) / 10;
    int annee2 = (now.year() % 100) % 10;

    for(int i = 0; i < 8; i++)
    {
      data[3][i] = numdate[jour1][i] + (numdate[jour2][i] >> 4);
      data[2][i] = numdate[10][i] + (numdate[mois1][i] >> 4);
      data[1][i] = numdate[mois2][i] + (numdate[10][i] >> 4);
      data[0][i] = numdate[annee1][i] + (numdate[annee2][i] >> 4);
    }
  }
}

void display()
{
  for (int matrix = 0; matrix < MAX_DEVICES; matrix++)
  {
    for (int row = 0; row < 8; row++)
    {
      lc.setRow(matrix,row,data[matrix][row]);
    }
  }
}

void changeDisplay() {
  buttonVal++;
  if (buttonVal > 3)
  {
    buttonVal = 1;
  }
}

void updateTime()
{
  if (Serial.available() > 0) {
    
    // Read the incoming character
    char incoming_char = Serial.read();
    
    // End of line?
    if(incoming_char == '\n') {
      
      // Parse the command
      
      // ## 
      if(serial_buffer[0] == '#' && serial_buffer[1] == '#')
        Serial.println("!!");
        
      // ?V
      else if(serial_buffer[0] == '?' && serial_buffer[1] == 'V')
        Serial.println(VERSION);

      // ?T 
      else if(serial_buffer[0] == '?' && serial_buffer[1] == 'T') {        
        DateTime now = rtc.now();
        char time_string[20];
        sprintf(time_string, "%02d/%02d/%d %02d:%02d:%02d", 
          now.day(), now.month(), now.year(),
          now.hour(), now.minute(), now.second());
          Serial.println(time_string);
      }

      // !T 
      else if(serial_buffer[0] == '!' && serial_buffer[1] == 'T') {

        String time_string = String(serial_buffer);
        int day = time_string.substring(2, 4).toInt();
        int month = time_string.substring(4, 6).toInt();        
        int year = time_string.substring(6, 10).toInt();
        int hour = time_string.substring(10, 12).toInt();
        int minute = time_string.substring(12, 14).toInt();
        int second = time_string.substring(14, 16).toInt();
        DateTime set_time = DateTime(year, month, day, hour, minute, second);
        rtc.adjust(set_time);
        Serial.println("OK");
      }
      
      // Reset the buffer
      buffer_position = 0;
    }
    
    // Carriage return, do nothing
    else if(incoming_char == '\r');
    
    // Normal character
    else {
      
      // Buffer full, we need to reset it
      if(buffer_position == BUFFER_SIZE - 1) buffer_position = 0;

      // Store the character in the buffer and move the index
      serial_buffer[buffer_position] = incoming_char;
      buffer_position++;      
    }
  }
}
