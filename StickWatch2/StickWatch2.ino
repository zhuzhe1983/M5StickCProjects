#include "src/Adafruit_GFX/Adafruit_GFX.h"
#include "src/Adafruit_GFX/ext_canvas.h"
#include "src/Adafruit_GFX/config.h"
#include "src/image_data.h"

// #include <stdio.h>
// #include "esp_sleep.h"
// #include "soc/rtc_cntl_reg.h"
// #include "soc/rtc_io_reg.h"
// #include "soc/sens_reg.h"
// #include "soc/soc.h"
// #include "driver/gpio.h"
// #include "driver/rtc_io.h"
// #include "esp32/ulp.h"

#include "src/RTCLib/RTClib.h"

#include "res.h"
#include "imu.h"

#define calibration_x 15
#define calibration_y 0

#define sensitivity_x 1.9
#define sensitivity_y 1.2

PCF8563 rtc;

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT  80

#define ST77XX_BLACK      0x000000
#define ST77XX_WHITE      0xFFFFFF
#define ST77XX_RED        0xFF0000
#define ST77XX_GREEN      0x00FF00
#define ST77XX_BLUE       0x0000FF
#define ST77XX_CYAN       0x00FFFF
#define ST77XX_MAGENTA    0xFF00FF
#define ST77XX_YELLOW     0xFFFF00
#define ST77XX_ORANGE     0xFFA500

const uint32_t COLORS_LIGHT[10] = {
  0xff4aad, 0x0e88fe, 0xcc03fc, 0xfe49ad, 0xff0505,
  0xfa660d, 0xfff800, 0x14fa00, 0x0496ff, 0xc900ff
};
const uint32_t COLORS_DARK[10] = {
  0x471f32, 0x002548, 0x3b004b, 0x3e1a2c, 0x4a000c,
  0x4a1e00, 0x4b4500, 0x004700, 0x022242, 0x380249
};

#include <Wire.h>

#include <SPI.h>
#include "src/Lcd_Driver.h"

#define TFT_MOSI      15
#define TFT_CLK       13
#define TFT_CS        5   // Chip select line for TFT display on Shield
#define TFT_DC        23  // Data/command line for TFT on Shield
#define TFT_RST       18  // Reset line for TFT is handled by seesaw!

GFXcanvas24 canvas = GFXcanvas24(LCD_WIDTH, LCD_HEIGHT);

// End of constructor list

int stateA = 0;

// int LED_RI = 9;
int LED_BUILTIN = 10;
int BUTTON_HOME = 37;
int BUTTON_PIN = 39;

//int BUTTON_HOME = 34;
RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

DateTime now;

void show_time() {
  now = rtc.now();
  now.hour();
  char buf[100];
  strncpy(buf, "YYYY.MM.DD hh:mm:ss\0", 100);
  now.format(buf);
  // Serial.printf("time now: %s\n", buf);

  canvas.setCursor(28, 70);
  canvas.setTextColor(0xAAFFFFFF);
  canvas.print(buf);
}

void setup(void) {
  Wire.begin();
  rtc.begin();
  DateTime now = rtc.now();
  if (now.year() < 2019) {
    Serial.println("RTC need factory reset!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }

  // pinMode(LED_RI, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BUTTON_HOME, INPUT | PULLUP);
  pinMode(BUTTON_PIN, INPUT | PULLUP);

  pinMode(TFT_MOSI, OUTPUT);
  pinMode(TFT_CLK, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);

  digitalWrite(TFT_CS, LOW);

  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(10);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)

  SPI.begin (TFT_CLK, -1, TFT_MOSI, -1);
  SPI.beginTransaction(SPISettings(70000000, MSBFIRST, SPI_MODE0));

  Serial.begin(115200);
  while (!Serial);             // Leonardo: wait for serial monitor

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);

  Wire.beginTransmission(0x34);
  Wire.write(0x10);
  Wire.write(0x9f);  //OLED_VPP Enable
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x28);
  Wire.write(0x9f); //Enable LDO2&LDO3, LED&TFT 3.3V
  // Wire.write(0xff); //Enable LDO2&LDO3, LED&TFT 3.3V
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x82);  //Enable all the ADCs
  Wire.write(0xff);
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x33);  //Enable Charging, 100mA, 4.2V, End at 0.9
  Wire.write(0xC0);
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x33);
  Wire.write(0xC3);
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0xB8);  //Enable Colume Counter
  Wire.write(0x80);
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x12);
  Wire.write(0x4d); //Enable DC-DC1, OLED_VDD, 5B V_EXT
  Wire.endTransmission();

  Wire.beginTransmission(0x34);
  Wire.write(0x36);
  Wire.write(0x5c); //PEK
  Wire.endTransmission();

  Lcd_Init();
  // Lcd_Clear(WHITE); // TODO?
  // Lcd_pic(gImage_001);
  attachInterrupt(digitalPinToInterrupt(BUTTON_HOME), home_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
  canvas.setRotation(1);

  sh200i_init();
  // runGraphicTest();
}

long loopTime, startTime, endTime, fps;

#define PAGE_CLOCK 0
#define PAGE_TIMER 1
#define PAGE_KEYBOARD 2
#define PAGE_GAME 3

#define PAGE_COUNT 4

int current_page = PAGE_CLOCK;

int cursorX, cursorY;
int clicked_cursor_x = -1, clicked_cursor_y = -1;

void draw_cursor() {
  int times = 1;
  int average_accX = 0, average_accY = 0;
  for (int i = 0; i < times; i++) {
    read_imu();
    average_accX += accX;
    average_accY += accY;
  }
  average_accX /= times;
  average_accY /= times;
  cursorX = 80 + calibration_x - sensitivity_x * average_accY / 50;
  cursorY = 40 + calibration_y - sensitivity_y * average_accX / 50;
  if (cursorX < 0) {
    cursorX = 0;
  } else  if (cursorX > 159) {
    cursorX = 159;
  }
  if (cursorY < 0) {
    cursorY = 0;
  } else if (cursorY > 79) {
    cursorY = 79;
  }
  // draw cursor
  canvas.fillCircle(cursorX, cursorY, 1, ST77XX_WHITE);
}

void draw_menu() {
  canvas.drawRect(0, 0, 33, 11, ST77XX_WHITE);

  canvas.setCursor(2, 2);
  canvas.setTextColor(0xAAFFFFFF);
  canvas.print("Timer");

  canvas.drawRect(35, 0, 35, 11, ST77XX_WHITE);
  canvas.setCursor(38, 2);
  canvas.setTextColor(0xAAFFFFFF);
  canvas.print("Clock");

  canvas.drawRect(36 + 35, 0, 50, 11, ST77XX_WHITE);
  canvas.setCursor(38 + 35, 2);
  canvas.setTextColor(0xAAFFFFFF);
  canvas.print("Keyboard");
}

void page_1_2() {
  show_time();
  loopTime = millis();
  int count = 0;
  if (current_page == PAGE_CLOCK) {
    count = now.hour() * 100 + now.minute();
  } else {
    // 4 digit 100ms counter
    count = (loopTime / 100) % 10000;
  }
  startTime = loopTime;

  int x_start = 6;
  int x_delta = 7;
  int r = 2;
  for (int n = 0; n < 10; n++) {
    // canvas.fillCircle(x_start + x_delta * n, 4, r, COLORS_LIGHT[n]);
  }

  int y2 = 11;
  for (int n = 0; n < 10; n++) {
    // canvas.fillCircle(x_start + x_delta * n, y2, r, COLORS_DARK[n]);
  }

  int y_start = 19;
  for (int pos = 0; pos < 4; pos++) {
    uint8_t curr_digit = 0;
    if (pos == 0) {
      curr_digit = count / 1000;
    } else if (pos == 1) {
      curr_digit = count / 100 % 10;
    } else if (pos == 2) {
      curr_digit = count / 10 % 10;
    } else if (pos == 3) {
      curr_digit = count % 10;
    }
    for (int row = 0; row < 7; row++) {
      for (int col = 0; col < 5; col++) {
        uint32_t color = DIGITS[curr_digit][row][col] ?
                         COLORS_LIGHT[curr_digit] : COLORS_DARK[curr_digit];
        canvas.fillCircle(x_start + col * 7, y_start + row * 7, r, color);
      }
    }
    x_start += 39;
  }
}

char input_text[128] = {};

int keyboard_start_x = 3;
int keyboard_start_y = 33;
int keyboard_offset_x = 5;

int _1_center_x = keyboard_start_x + 2;
int _1_center_y = keyboard_start_y + 3;

double keyboard_x_space = 12.0;
double keyboard_y_space = 12.0;

char keyboard[4][14] = {
  {'`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', ' '},
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\\'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', ' ', ' '},
  {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', ' ', ' ', ' '}
};

void handle_keyboard_click(int x, int y) {
  // calculate y first so that offset is right
  double y_delta_count = (y - _1_center_y) / keyboard_y_space;
  double x_delta_count = (x - _1_center_x - y_delta_count * keyboard_offset_x /* x has some offset */) / keyboard_x_space;
  int xx = int(x_delta_count + 0.5);
  int yy = int(y_delta_count + 0.5);
  char clicked_char = keyboard[yy][xx];
  Serial.printf("debug: y=%lf, x=%lf, char: %c\r\n", y_delta_count, x_delta_count, clicked_char);
  int len = strlen(input_text);
  input_text[len] = clicked_char;
  input_text[len + 1] = '\0';
}

void handle_other_area_click(int x, int y) {
  if (x >= 111 && x <= 124 && y >= 16 && y <= 27) {
    Serial.println("cl clicked");
    input_text[0] = '\0';
  }
  if (x >= 126 && x <= 157 && y >= 16 && y <= 27) {
    Serial.println("<- clicked");
    int len = strlen(input_text);
    if (len > 0) {
      input_text[len - 1] = '\0';
    }
  }
}

void page_keyboard() {
  int input_start_x = 0;
  canvas.drawRect(input_start_x, 15, 110, 13, 0xAAFFFFFF);
  canvas.setTextColor(0xAAFFFFFF);
  // print cl button
  canvas.drawRect(input_start_x + 111, 15, 14, 13, 0xAAFFFFFF);
  canvas.setCursor(input_start_x + 111 + 2, 15 + 2);
  canvas.print("cl");
  // print backspace button
  canvas.drawRect(input_start_x + 112 + 14, 15, 32, 13, 0xAAFFFFFF);
  canvas.setCursor(input_start_x + 112 + 14 + 2, 15 + 2);
  canvas.print("<-");
  // print input text
  canvas.setCursor(input_start_x + 3, 18);
  canvas.print(input_text);

  canvas.setCursor(keyboard_start_x, keyboard_start_y);
  canvas.print("` 1 2 3 4 5 6 7 8 9 0 - =");
  canvas.setCursor(keyboard_start_x + keyboard_offset_x, keyboard_start_y + 12);
  canvas.print("Q W E R T Y U I O P [ ] \\");
  canvas.setCursor(keyboard_start_x + keyboard_offset_x * 2, keyboard_start_y + 24);
  canvas.print("A S D F G H J K L ; '    ");
  canvas.setCursor(keyboard_start_x + keyboard_offset_x * 3, keyboard_start_y + 36);
  canvas.print("Z X C V B N M , . /      ");
  canvas.setTextSize(1);
  if (clicked_cursor_x >= 0 && clicked_cursor_y >= 0) {
    if (clicked_cursor_y > 33) {
      handle_keyboard_click(clicked_cursor_x, clicked_cursor_y);
    } else {
      handle_other_area_click(clicked_cursor_x, clicked_cursor_y);
    }
    clicked_cursor_x = clicked_cursor_y = -1;
  }
}

#define GAME_STATE_INIT     0
#define GAME_STATE_PLAYING  1
int game_state = GAME_STATE_INIT;

#define STICK_STILL 0
#define STICK_LEFT  1
#define STICK_RIGHT 2

int game_stick_state = STICK_STILL;

#define STICK_LENGTH_SHORT   25
#define STICK_LENGTH_NORMAL  40
#define STICK_LENGTH_LONG    80

#define STICK_START_Y  72
#define STICK_HEIGHT    3
int stick_length = STICK_LENGTH_NORMAL;

#define BALL_R 2

double stick_pos = 80.0;
double ball_x = 80.0;
double ball_y = STICK_START_Y - BALL_R * 2;
double ball_speed_x = 2.0;
double ball_speed_y = -2.0;

void page_game() {
  if (game_state == GAME_STATE_INIT) {
    canvas.setTextSize(2);
    canvas.setTextColor(0xAAFF8888);
    canvas.setCursor(14, 18);
    canvas.print("DxBall Game");
    canvas.setCursor(18, 50);
    canvas.setTextSize(1);
    canvas.setTextColor(0xAAFFFFFF);
    canvas.print("press button to start");
    int arrow_center_x = 77;
    canvas.drawFastVLine(arrow_center_x, 65, 5, 0xAAFFFFFF);
    canvas.fillTriangle(arrow_center_x - 3, 70,
                        arrow_center_x + 3, 70,
                        arrow_center_x, 75, 0xAAFFFFFF);
  } else if (game_state == GAME_STATE_PLAYING) {
    // update stick position
    if (game_stick_state == STICK_LEFT) {
      stick_pos -= 4;
    } else if (game_stick_state == STICK_RIGHT) {
      stick_pos += 4;
    }
    ball_x += ball_speed_x;
    ball_y += ball_speed_y;
    if (ball_x <= BALL_R || ball_x >= SCREEN_WIDTH - BALL_R) {
      ball_speed_x = -ball_speed_x;
      Serial.printf("case 1: %lf\r\n", ball_speed_x);
    } else if (ball_y <= BALL_R || ball_y >= STICK_START_Y - BALL_R) {
      ball_speed_y = -ball_speed_y;
      Serial.printf("case 2: %lf\r\n", ball_speed_y);
    }
    // update ball position
    canvas.fillRect(stick_pos - stick_length / 2 , STICK_START_Y,
                    stick_length, STICK_HEIGHT, 0xAAFF0000);
    canvas.fillCircle(ball_x, ball_y, BALL_R, 0xAA00FF00);
  }
  if (digitalRead(BUTTON_HOME) == 0) {
    // Serial.println("button 1 is down");
    game_button1_pressed();
  } else if (digitalRead(BUTTON_PIN) == 0) {
    // Serial.println("button 2 is down");
    game_button2_pressed();
  } else {
    game_no_button_pressed();
  }
}

void game_button1_pressed() {
  if (game_state == GAME_STATE_PLAYING) {
    game_stick_state = STICK_LEFT;
  }
}

void game_button2_pressed() {
  if (game_state == GAME_STATE_INIT) {
    game_state = GAME_STATE_PLAYING;
  } else if (game_state == GAME_STATE_PLAYING) {
    game_stick_state = STICK_RIGHT;
  }
}
void game_no_button_pressed() {
  if (game_state == GAME_STATE_PLAYING) {
    game_stick_state = STICK_STILL;
  }
}

void loop(void) {
  canvas.fillScreen(0x00000000); // fill screen bg
  if (current_page == PAGE_CLOCK | current_page == PAGE_TIMER) {
    draw_menu();
    page_1_2();
    draw_cursor();
  } else if (current_page == PAGE_KEYBOARD) {
    draw_menu();
    page_keyboard();
    draw_cursor();
  } else if (current_page == PAGE_GAME) {
    page_game();
  }
  // send frame then delay
  sendGRAM();

  loopTime = millis();
  endTime = loopTime;
  unsigned long delta = endTime - startTime;
  fps = 1000 / delta;
  // Serial.printf("fill+draw+send GRAM cost: %ldms, calc fps:%ld, real fps:%ld\r\n", delta, fps, fps > 60 ? 60 : fps);
  delay(25); // fps wrong fix
}

unsigned long last_isr_time;
#define ISR_DITHERING_TIME_MS 10

// 中断函数
void home_isr() {
  if (millis() - last_isr_time < ISR_DITHERING_TIME_MS) {
    return;
  }
  last_isr_time = millis();
  if (current_page != PAGE_GAME) {
    current_page++;
    if (current_page > PAGE_COUNT - 1) {
      current_page = 0;
    }
  }
}

// 中断函数
void button_isr() {
  if (millis() - last_isr_time < ISR_DITHERING_TIME_MS) {
    return;
  }
  last_isr_time = millis();
  if (current_page != PAGE_GAME) {
    Serial.printf("cursorX=%d, cursorY=%d\r\n", cursorX, cursorY);
    if (current_page == PAGE_KEYBOARD) {
      clicked_cursor_x = cursorX;
      clicked_cursor_y = cursorY;
    }
  }
}

void sendGRAM() {
  Lcd_pic(canvas.getBuffer(), GRAM_BUFFER_SIZE);
}
