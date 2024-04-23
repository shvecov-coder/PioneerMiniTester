#include <string>
#include <vector>
#include <WiFi.h>
#include <stdio.h>
#include "Arduino.h"
#include <Adafruit_ST7789.h>

#define outputA 15
#define outputB 17
#define TFT_MOSI 23  // SDA Pin on ESP32
#define TFT_SCLK 18  // SCL Pin on ESP32
#define TFT_CS   -1  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
#define DRONE_SSID "PioneerMini246f28c542b4"
#define DRONE_PASS "12345678"

bool is_connected = false;
int counter = 0;
int aState;
int aLastState;

class Submenu
{
  public:
    Submenu(){}
    Submenu(String name, int idx)
    {
      this->name = name;
      this->y = idx * 20;
      if (idx == 0)
      {
        this->active = true;
      }
    }
    ~Submenu(){}

    String get_name() { return this->name; }
    int get_y() { return this->y; }
    void set_on() { this->active = true; }
    void set_off() { this->active = false; }
    bool is_active() { return this->active; }
    void show(Adafruit_ST7789 * tft)
    {
      Serial.print(this->name.c_str());
    }
  private:
    String name = "";
    bool active = false;
    int y = 0;
};

class Menu
{
  public:
    Menu(){}
    ~Menu(){}

    bool menu_up()
    {
      if (this->current_item_menu != 0)
      {
        this->current_item_menu--;
        for (int i = 0; i < this->get_max_item_menu(); i++)
        {
          if (this->current_item_menu == i)
          {
            this->items_menu[i]->set_on();
          }
          else
          {
            this->items_menu[i]->set_off();
          }
        }
        return true;
      }
      else
      {
        return false;
      }
    }

    bool menu_down()
    {
      if (this->current_item_menu != this->get_max_item_menu() - 1)
      {
        this->current_item_menu++;
        for (int i = 0; i < this->get_max_item_menu(); i++)
        {
          if (this->current_item_menu == i)
          {
            this->items_menu[i]->set_on();
          }
          else
          {
            this->items_menu[i]->set_off();
          }
        }
        return true;
      }
      else
      {
        return false;
      }
    }

    void add_submenu(Submenu * sub)
    {
      this->items_menu.push_back(sub);
    }

    void show(Adafruit_ST7789 * tft)
    {
      //tft->fillScreen(ST77XX_BLACK);
      for (int i = 0; i < this->get_max_item_menu(); i++)
      {
        tft->setCursor(0, this->items_menu[i]->get_y());
        if (this->items_menu[i]->is_active())
        {
          tft->setTextColor(ST77XX_GREEN);
          tft->print("-> ");
          tft->setTextColor(ST77XX_WHITE);
          tft->print(this->items_menu[i]->get_name());
        }
        else
        {  
          tft->print("-> " + this->items_menu[i]->get_name());
        }
      }
    }

    void submenu_show(Adafruit_ST7789 * tft)
    {
      this->items_menu[this->current_item_menu]->show(tft);
    }
  private:
    int current_item_menu = 0;
    std::vector<Submenu *> items_menu;

    int get_max_item_menu() { return this->items_menu.size(); }
};

void connect_check(void * pvParameters)
{
  Adafruit_ST7789 tft = *((Adafruit_ST7789 *)pvParameters);
  tft.setCursor(0, 225);
  tft.setTextSize(1);
  tft.print("Connecting drone ");

  while (true)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      tft.fillCircle(120, 230, 3, ST77XX_GREEN);
    }
    else
    {
      tft.fillCircle(120, 230, 3, ST77XX_RED);
    }
    delay(500);
  }
}

extern "C" void app_main()
{
  initArduino();
  pinMode(outputA, INPUT);
  pinMode(outputB, INPUT);
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

  Serial.begin(115200);
  while(!Serial){}
  aLastState = digitalRead(outputA);

  Menu menu;
  Submenu arm("ARM TEST", 0);
  Submenu disarm("DISARM TEST", 1);
  Submenu telemetry("TELEMETRY TEST", 2);
  Submenu led("LED TEST", 3);
  Submenu takeoff("TAKEOFF TEST", 4);
  Submenu land("LAND TEST", 5);
  menu.add_submenu(&arm);
  menu.add_submenu(&disarm);
  menu.add_submenu(&telemetry);
  menu.add_submenu(&led);
  menu.add_submenu(&takeoff);
  menu.add_submenu(&land);

  tft.init(240, 240, SPI_MODE2); 
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextWrap(true);
  menu.show(&tft);

  xTaskCreate(connect_check, "connected_check", 2048, (void *)&tft, 2, NULL);

  WiFi.begin(DRONE_SSID, DRONE_PASS);

  while(true)
  {
    aState = digitalRead(outputA);
    if (aState != aLastState)
    {
      if (digitalRead(outputB) != aState)
      {
        counter++;
        if (counter > 1)
        {
          if (menu.menu_up())
          {
            menu.show(&tft);
          }
          counter = 0;
        }
      }
      else
      {
        counter--;
        if (counter < -1)
        {
          if (menu.menu_down())
          {
            menu.show(&tft);
          }
          counter = 0;
        }
      }
    }

    aLastState = aState;

    if (Serial.available() > 0)
    {
      int command = Serial.parseInt();

      if (command == 1)
      {
        if (menu.menu_up())
        {
          menu.show(&tft);
        }
      }

      if (command == 2)
      {
        if (menu.menu_down())
        {
          menu.show(&tft);
        }
      }

      if (command == 3)
      {
        menu.submenu_show(&tft);
      }
    }
  }
}
