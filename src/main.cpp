/*
Arduino Monopoly Electronic Banking - macaquedev, Apr 8. 2022

To use, connect like this:

RFID Reader RC522:
- 3.3V and GND to Arduino.
- SDA - pin 10
- SCK - pin 13
- MOSI - pin 11
- MISO - pin 12
- RST - pin 9

Joystick:
- 5V and Ground to Arduino
- S-X - pin A0
- S-Y - pin A1
- S-K - pin 2

LCD I2C: GND, 5V, SCL - A5, SDA - A4.

RFID Cards are simple MiFare 1K cards, but 4K should also work.

*/

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG 1
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x);
#else
#define debug(x)
#define debugln(x)
#endif

#define MAX_NUM_PLAYERS 6 // NOT MORE THAN 10!
#define MIN_NUM_PLAYERS 2

#define MAX_TRANSACTION 5000
#define MIN_TRANSACTION 0

#define JOYSTICK_X_PIN A0
#define JOYSTICK_Y_PIN A1
#define JOYSTICK_SW_PIN 2

#define LCD_I2C_ADDR 0x3F
#define LCD_COLS 16
#define LCD_ROWS 2

#define MAX_START_MONEY 5000
#define MIN_START_MONEY 1000

#define SERIAL_BAUD_RATE 9600

#define DELAY_TIME_BETWEEN_CARD_SCANS 1500 // milliseconds

#define joystickUp (analogRead(JOYSTICK_Y_PIN) > 900)
#define joystickDown (analogRead(JOYSTICK_Y_PIN) < 200)
#define joystickLeft (analogRead(JOYSTICK_X_PIN) > 900)
#define joystickRight (analogRead(JOYSTICK_X_PIN) < 200)
#define joystickPressed (!digitalRead(JOYSTICK_SW_PIN))

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
MFRC522 mfrc522(10, 9);

typedef struct
{
  String address;
  long money;
} Card;

uint8_t displaySelectNumPlayers()
{
  lcd.setCursor(0, 0);
  lcd.print("Players: ");
  lcd.setCursor(0, 1);
  lcd.print("Joystick up/down");
  uint8_t numPlayers = 4;
  for (;;)
  {
    lcd.setCursor(9, 0);
    lcd.print(numPlayers);
    if (joystickPressed)
    {
      lcd.clear();
      return numPlayers;
    }
    else if (joystickUp)
    {
      while (joystickUp)
        ;
      if (numPlayers < MAX_NUM_PLAYERS)
      {
        numPlayers += 1;
      }
    }
    else if (joystickDown)
    {
      while (joystickDown)
        ;
      if (numPlayers > MIN_NUM_PLAYERS)
      {
        numPlayers -= 1;
      }
    }
  }
}

uint8_t waitForAddress(uint8_t numPlayers, Card *cards)
{
  while (!mfrc522.PICC_IsNewCardPresent())
    ;
  while (!mfrc522.PICC_ReadCardSerial())
    ;
  String address = "";
  for (uint8_t i = 0; i < mfrc522.uid.size; i++)
  {
    address.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    address.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  address.toUpperCase();
  uint8_t i;
  for (i = 0; i < numPlayers; i++)
  {
    if (cards[i].address == address)
    {
      break;
    }
    else if (cards[i].address == "")
    {
      cards[i].address = address;
      break;
    }
  }
  return i == numPlayers ? 255 : i;
}

void scanCards(uint8_t numPlayers, Card *cards)
{
  uint8_t cardNum = 0;
  while (cardNum < numPlayers)
  {
    lcd.setCursor(0, 0);
    lcd.print("Scan card ");
    lcd.print(cardNum + 1);
    uint8_t response = waitForAddress(numPlayers, cards);
    if (response == cardNum)
    {
      lcd.setCursor(0, 1);
      lcd.print("Success!");
      cardNum++;
    }
    else
    {
      lcd.setCursor(0, 1);
      lcd.print("Card already used!");
    }
    delay(DELAY_TIME_BETWEEN_CARD_SCANS);
    lcd.clear();
  }
  lcd.setCursor(0, 0);
}

long displayStartingMoneyMenu()
{
  lcd.print("Start $: ");
  lcd.setCursor(0, 1);
  lcd.print("Joystick up/down");
  long startMoney = 1500;
  for (;;)
  {
    lcd.setCursor(9, 0);
    lcd.print(startMoney);
    if (joystickPressed)
    {
      lcd.setCursor(0, 0);
      lcd.clear();
      return startMoney;
    }
    else if (joystickUp)
    {
      if (startMoney < MAX_START_MONEY)
        startMoney += 100;
    }
    else if (joystickDown)
    {
      if (startMoney > MIN_START_MONEY)
        startMoney -= 100;
    }
    delay(200);
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  uint8_t numPlayers = displaySelectNumPlayers();
  Card cards[numPlayers];
  scanCards(numPlayers, cards);
  long startMoney = displayStartingMoneyMenu();
  for (uint8_t i = 0; i < numPlayers; i++)
  {
    cards[i].money = startMoney;
  }
  while (joystickPressed)
    ;
  for (;;)
  {
    uint8_t currentLine = 0;
    uint8_t currentMenuEntry = 0;
    bool pressed = false;
    char menuEntries[][16] = {"View Balance", "Pass Go", "Collect money", "Pay to bank", "Transaction", "Income Tax 10%", "House/HotelTax"};
    for (;;)
    {
      lcd.clear();
      for (uint8_t i = 0; i < LCD_ROWS; i++)
      {
        lcd.setCursor(2, i);
        lcd.print(menuEntries[currentMenuEntry + i]);
      }
      lcd.setCursor(0, currentLine);
      lcd.print(">");
      for (;;)
      {
        if (joystickPressed)
        {
          pressed = true;
          break;
        }
        else if (joystickUp)
        {
          while (joystickUp)
            ;
          if (currentLine > 0)
          {
            currentLine--;
            break;
          }
          else if (currentMenuEntry > 0)
          {
            currentMenuEntry--;
            break;
          }
        }
        else if (joystickDown)
        {
          while (joystickDown)
            ;
          if (currentLine < LCD_ROWS - 1)
          {
            currentLine++;
            break;
          }
          else if (currentMenuEntry < (sizeof(menuEntries) / sizeof(menuEntries[0])) - currentLine - 1)
          {
            currentMenuEntry++;
            break;
          }
        }
      }
      if (pressed)
        break;
    }
    pressed = false;

    switch (currentMenuEntry + currentLine)
    {
    case 0:
    {
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        uint8_t addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Balance: ");
          lcd.print(cards[addr].money);
          lcd.print("$");
          break;
        }
      }
      break;
    }
    case 1:
    {
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        uint8_t addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("$200 added!");
          cards[addr].money += 200;
          break;
        }
      }
      break;
    }
    case 2:
    {
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        uint8_t addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.setCursor(0, 0);
          lcd.print("Amount $: ");
          lcd.setCursor(0, 1);
          lcd.print("Joystick U/D L/R");
          long money = 0;
          uint8_t digits = 1;
          uint8_t newDigits = 1;
          for (;;)
          {
            lcd.setCursor(10, 0);
            lcd.print(money);
            if (joystickPressed)
            {
              lcd.setCursor(0, 0);
              lcd.clear();
              break;
            }
            else if (joystickUp and money <= MAX_TRANSACTION - 100)
              money += 100;
            else if (joystickDown and money >= MIN_TRANSACTION + 100)
              money -= 100;
            else if (joystickLeft and money > MIN_TRANSACTION)
              money -= 1;
            else if (joystickRight and money < MAX_TRANSACTION)
              money += 1;
            newDigits = 0;
            for (uint8_t j = 0;; j++)
              if ((int)(money / (pow(10, j))))
                newDigits += 1;
              else
                break;
            if (newDigits != digits)
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Amount $: ");
              lcd.setCursor(0, 1);
              lcd.print("Joystick U/D L/R");
              digits = newDigits;
            }
            delay(200);
          }
          cards[addr].money += money;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("$");
          lcd.print(money);
          lcd.print(" added!");
          break;
        }
      }
      break;
    }
    case 3:
    {
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        uint8_t addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.setCursor(0, 0);
          lcd.print("Amount $: ");
          lcd.setCursor(0, 1);
          lcd.print("Joystick U/D L/R");
          long money = 0;
          uint8_t digits = 1;
          uint8_t newDigits = 1;
          for (;;)
          {
            lcd.setCursor(10, 0);
            lcd.print(money);
            if (joystickPressed)
            {
              lcd.setCursor(0, 0);
              lcd.clear();
              break;
            }
            else if (joystickUp and money <= MAX_TRANSACTION - 100)
              money += 100;
            else if (joystickDown and money >= MIN_TRANSACTION + 100)
              money -= 100;
            else if (joystickLeft and money > MIN_TRANSACTION)
              money -= 1;
            else if (joystickRight and money < MAX_TRANSACTION)
              money += 1;
            newDigits = 0;
            for (uint8_t j = 0;; j++)
              if ((int)(money / (pow(10, j))))
                newDigits += 1;
              else
                break;
            if (newDigits != digits)
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Amount $: ");
              lcd.setCursor(0, 1);
              lcd.print("Joystick U/D L/R");
              digits = newDigits;
            }
            delay(200);
          }
          cards[addr].money -= money;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("$");
          lcd.print(money);
          lcd.print(" paid!");
          break;
        }
      }
      break;
    }
    case 4:
    {
      uint8_t addr1, addr2;
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan sender: ");
        addr1 = waitForAddress(numPlayers, cards);
        if (addr1 == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.setCursor(0, 1);
          lcd.print("Success!");
          delay(1000);
          break;
        }
      }
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan recipient: ");
        addr2 = waitForAddress(numPlayers, cards);
        if (addr2 == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else if (addr2 == addr1)
        {
          lcd.setCursor(0, 1);
          lcd.print("Same as sender!");
          delay(1500);
        }
        else
          break;
      }
      lcd.setCursor(0, 0);
      lcd.print("Amount $: ");
      lcd.setCursor(0, 1);
      lcd.print("Joystick U/D L/R");
      long money = 0;
      uint8_t digits = 1;
      uint8_t newDigits = 1;
      for (;;)
      {
        lcd.setCursor(10, 0);
        lcd.print(money);
        if (joystickPressed)
        {
          lcd.setCursor(0, 0);
          lcd.clear();
          break;
        }
        else if (joystickUp and money <= MAX_TRANSACTION - 100)
          money += 100;
        else if (joystickDown and money >= MIN_TRANSACTION + 100)
          money -= 100;
        else if (joystickLeft and money > MIN_TRANSACTION)
          money -= 1;
        else if (joystickRight and money < MAX_TRANSACTION)
          money += 1;
        newDigits = 0;
        for (uint8_t j = 0;; j++)
          if ((int)(money / (pow(10, j))))
            newDigits += 1;
          else
            break;
        if (newDigits != digits)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Amount $: ");
          lcd.setCursor(0, 1);
          lcd.print("Joystick U/D L/R");
          digits = newDigits;
        }
        delay(200);
      }
      cards[addr1].money -= money;
      cards[addr2].money += money;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("$");
      lcd.print(money);
      lcd.print(" paid!");
      break;
    }
    case 5:
    {
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        uint8_t addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          long taxAmount = cards[addr].money / 10;
          lcd.print("$");
          lcd.print(taxAmount);
          lcd.print(" paid!");
          cards[addr].money -= taxAmount;
          break;
        }
      }
      break;
    }
    case 6:
    {
      uint8_t addr;
      for (;;)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan card: ");
        addr = waitForAddress(numPlayers, cards);
        if (addr == 255)
        {
          lcd.setCursor(0, 1);
          lcd.print("Invalid card");
          delay(1500);
        }
        else
        {
          lcd.setCursor(0, 1);
          lcd.print("Success!");
          delay(1000);
          break;
        }
      }
      lcd.clear();
      lcd.print("Houses: ");
      lcd.setCursor(0, 1);
      lcd.print("Joystick up/down");
      uint8_t houses = 0;
      uint8_t hotels = 0;
      uint8_t digits = 0;
      uint8_t newDigits = 0;
      for (;;)
      {
        lcd.setCursor(8, 0);
        lcd.print(houses);
        if (joystickPressed)
        {
          lcd.setCursor(0, 0);
          lcd.clear();
          break;
        }
        else if (joystickUp)
          if (houses < 50)
            houses += 1;
          else if (joystickDown)
            if (houses > 0)
              houses -= 1;
        newDigits = 0;
        for (uint8_t j = 0;; j++)
          if ((int)(houses / (pow(10, j))))
            newDigits += 1;
          else
            break;
        if (newDigits != digits)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Houses: ");
          lcd.setCursor(0, 1);
          lcd.print("Joystick up/down");
          digits = newDigits;
        }
        delay(200);
      }
      lcd.clear();
      lcd.print("Hotels: ");
      lcd.setCursor(0, 1);
      lcd.print("Joystick up/down");
      digits = 1;
      newDigits = 1;
      while (joystickPressed)
        ;
      for (;;)
      {
        lcd.setCursor(8, 0);
        lcd.print(hotels);
        if (joystickPressed)
        {
          lcd.setCursor(0, 0);
          lcd.clear();
          break;
        }
        else if (joystickUp)
          if (hotels < 50)
            hotels += 1;
          else if (joystickDown)
            if (hotels > 0)
              hotels -= 1;
        newDigits = 0;
        for (uint8_t j = 0;; j++)
          if ((int)(hotels / (pow(10, j))))
            newDigits += 1;
          else
            break;
        if (newDigits != digits)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Hotels: ");
          lcd.setCursor(0, 1);
          lcd.print("Joystick up/down");
          digits = newDigits;
        }
        delay(200);
      }
      long taxAmount = 40 * houses + 115 * hotels;
      lcd.print("$");
      lcd.print(taxAmount);
      lcd.print(" paid!");
      cards[addr].money -= taxAmount;
    }
    }
    lcd.setCursor(0, 1);
    lcd.print("Press to exit");
    while (joystickPressed)
      ;
    while (not joystickPressed)
      ;
    while (joystickPressed)
      ;
    delay(500);
  }
}
void loop() {}