#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

// lcd
#define RS A5
#define E A4
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// MFRC
#define RST_PIN 9
#define SS_PIN 10

// BTN
#define BUTTON_WRITE 2
#define BUTTON_READ 3

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

volatile byte ifWrite = 0;
volatile byte ifClear = 0;

void buttonWrite()
{
  if (ifWrite == 0)
  {
    ifWrite = 1;
  }
}

void buttonRead()
{
  if (ifWrite == 1)
  {
    ifWrite = 0;
  }
}

byte writeInMFRC522(byte *buffer, byte block, byte size)
{

  MFRC522::StatusCode status;
  Serial.setTimeout(5000L);
  Serial.println(F("Type your name, ending with ENTER"));
  byte len = Serial.readBytesUntil('\t', (char *)buffer, 30);

  // while (len > 0 && buffer[len - 1] == '\r')
  //   len--;

  for (byte i = len; i < 30; i++)
  {
    buffer[i] = ' ';
  }
  Serial.println(F("Authenticating again using key B..."));
  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 0;
  }

  Serial.print(F("Writing data into block "));
  Serial.print(block);
  Serial.println(F(" ..."));
  Serial.println();
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(block, buffer, size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 0;
  }
  return 1;
}

byte readFromMFRC522(byte *buffer, byte block, byte size)
{

  MFRC522::StatusCode status;

  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 0;
  }

  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(block, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 1;
  }
  return 1;
}

void lcdInformation(byte succes, byte *buffer)
{
  if (ifWrite == 1)
  {
    if (ifClear == 1)
    {
      ifClear = 0;
      lcd.clear();
    }
    lcd.setCursor(0, 0);
    lcd.write("Write in SerialMonitor");
    if (succes == 1)
    {
      lcd.setCursor(0, 1);
      lcd.write("SUCCES!");
    }
  }
  else
  {
    if (ifClear == 0)
    {
      ifClear = 1;
      lcd.clear();
    }
    lcd.setCursor(0, 0);
    lcd.write("Read From MFRC522: ");
    if (succes == 1)
    {
      for (int i = 0; i < 16; i++)
      {
        lcd.setCursor(i, 1);
        lcd.write((char)(buffer[i]));
      }
    }
  }
}
void infromationFromMFRC522(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.write("HELLO");
  delay(1000);
  lcdInformation(0, {});
  pinMode(BUTTON_WRITE, INPUT_PULLUP);
  pinMode(BUTTON_READ, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_WRITE), buttonWrite, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_READ), buttonRead, FALLING);
}

void loop()
{
  if (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial())
  {
    for (byte i = 0; i < 6; i++)
    {
      key.keyByte[i] = 0xFF;
    }

    infromationFromMFRC522(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print(F(" PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    byte buffer[34];
    byte block = 4;
    byte sizeWrite = 16;
    byte sizeRead = 18;
    if (ifWrite == 1)
    {
      byte succes = writeInMFRC522(buffer, block, sizeWrite);
      lcdInformation(succes, buffer);
    }
    else
    {
      byte succes = readFromMFRC522(buffer, block, sizeRead);
      lcdInformation(succes, buffer);
    }

    Serial.println(" ");
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}
