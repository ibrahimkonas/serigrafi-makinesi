#include <U8g2lib.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <EEPROM.h>

// LCD (ST7920 SPI 128x64)
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 13, 11, 10, U8X8_PIN_NONE);

// Rotary Encoder
#define ENCODER_A   31
#define ENCODER_B   33
#define ENCODER_BTN 35
ClickEncoder encoder(ENCODER_A, ENCODER_B, ENCODER_BTN, 4);
volatile int16_t encLast, encValue = 0;

// Parametreler (EEPROM adresleri)
#define EEPROM_STEP_SPEED_ADDR   0
#define EEPROM_STEP_DIST_ADDR    2
#define EEPROM_RAGLE_TIME_ADDR   4
#define EEPROM_PISTON_TIME_ADDR  6

unsigned int stepSpeed = 700;   // us
unsigned int stepDist  = 1000;  // adım
unsigned int ragleTime = 200;   // ms
unsigned int pistonTime = 200;  // ms

// Menü ve modlar
enum MainMenu { MENU_OTOMATIK, MENU_TEST, MENU_AYAR, MENU_COUNT };
byte mainMenuIndex = 0;
byte ayarMenuIndex = 0;
byte testMenuIndex = 0;

enum State { MAIN_MENU, OTOMATIK, TEST, AYAR, ERROR } state = MAIN_MENU;
byte donusYon = 0; // 0: sağdan sola, 1: soldan sağa
char statusMsg[32] = "";

const char *mainMenuTexts[] = { "Otomatik Mod", "Test Modu", "Ayarlar" };
const char *ayarMenuNames[] = {
  "Step Hiz(us):", "Step Adim:", "Ragle Bekle(ms):", "Piston Bekle(ms):"
};
const byte AYAR_MENU_COUNT = 4;

// Makine Pinleri (örnek, değiştirilebilir)
const int anaPistonYukariSensor = 2;
const int anaPistonAsagiSensor  = 3;
const int raglePistonYukariSensor = 4;
const int raglePistonAsagiSensor  = 5;
const int stepSagSensor = 6;
const int stepSolSensor = 7;
const int pedalButon = 8; // LOW: Basili

const int anaPistonA = 22;
const int anaPistonB = 23;
const int raglePistonA = 24;
const int raglePistonB = 25;
const int stepPin = 26;
const int dirPin  = 27;
const int enablePin = 28;

void timerIsr() {
  encoder.service();
}

void setup() {
  u8g2.begin();
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  pinMode(ENCODER_BTN, INPUT_PULLUP);

  pinMode(anaPistonYukariSensor, INPUT);
  pinMode(anaPistonAsagiSensor, INPUT);
  pinMode(raglePistonYukariSensor, INPUT);
  pinMode(raglePistonAsagiSensor, INPUT);
  pinMode(stepSagSensor, INPUT);
  pinMode(stepSolSensor, INPUT);
  pinMode(pedalButon, INPUT_PULLUP);

  pinMode(anaPistonA, OUTPUT);
  pinMode(anaPistonB, OUTPUT);
  pinMode(raglePistonA, OUTPUT);
  pinMode(raglePistonB, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  // EEPROM'dan oku
  stepSpeed = EEPROMReadInt(EEPROM_STEP_SPEED_ADDR, 700);
  stepDist = EEPROMReadInt(EEPROM_STEP_DIST_ADDR, 1000);
  ragleTime = EEPROMReadInt(EEPROM_RAGLE_TIME_ADDR, 200);
  pistonTime = EEPROMReadInt(EEPROM_PISTON_TIME_ADDR, 200);

  digitalWrite(enablePin, LOW); // Step aktif
  tumPistonlarYukari();
  strcpy(statusMsg, "Hazir");

  encLast = encoder.getValue();
}

void loop() {
  switch (state) {
    case MAIN_MENU:   handleMainMenu(); break;
    case OTOMATIK:    otomatikModu(); break;
    case TEST:        testModu(); break;
    case AYAR:        ayarMenusu(); break;
    case ERROR:       hataEkrani(); break;
  }
}

/******************* ANA MENÜ *********************/
void handleMainMenu() {
  int16_t enc = encoder.getValue();
  if (enc != encLast) {
    int diff = enc - encLast;
    encLast = enc;
    mainMenuIndex = (mainMenuIndex + diff + MENU_COUNT) % MENU_COUNT;
  }
  ClickEncoder::Button b = encoder.getButton();
  if (b == ClickEncoder::Clicked) {
    if (mainMenuIndex == 0) { state = OTOMATIK; strcpy(statusMsg, "Bekliyor"); }
    else if (mainMenuIndex == 1) { state = TEST; testMenuIndex = 0; }
    else if (mainMenuIndex == 2) { state = AYAR; ayarMenuIndex = 0; }
    delay(200);
  }
  // LCD
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "Serigrafi Makinesi");
  for (byte i=0; i<MENU_COUNT; i++) {
    if (i == mainMenuIndex) u8g2.drawStr(0, 20+12*i, ">");
    u8g2.drawStr(10, 20+12*i, mainMenuTexts[i]);
  }
  u8g2.drawStr(0, 58, "Menu: Enkoder bas");
  u8g2.sendBuffer();
}

/************* OTOMATİK MOD (Baskı Döngüsü) ***********/
void otomatikModu() {
  static bool ilk = true;
  static byte adim = 0;
  static unsigned long stepStart = 0;
  static bool stepOK = true;

  if (ilk) {
    tumPistonlarYukari();
    strcpy(statusMsg, "Pedal Bekleniyor");
    ilk = false;
    adim = 0;
    donusYon = 0;
  }
  // Pedal bekleniyor
  if (adim == 0) {
    showOtoStatus("Bekliyor", "");
    if (digitalRead(pedalButon) == LOW) {
      strcpy(statusMsg, "Calisiyor");
      adim = 1; delay(100);
    }
  }
  // Ana piston asagi
  else if (adim == 1) {
    anaPistonAsagi();
    showOtoStatus("Ana Piston Iniyor", "");
    if (!waitSensor(anaPistonAsagiSensor, "AnaP Down")) { hataDur("AnaP Down Hata"); ilk = true; adim=0; state=ERROR; return; }
    delay(pistonTime);
    adim = 2;
  }
  // Ragle asagi
  else if (adim == 2) {
    raglePistonAsagi();
    showOtoStatus("Ragle Iniyor", "");
    if (!waitSensor(raglePistonAsagiSensor, "Ragle Down")) { hataDur("Ragle Down Hata"); ilk = true; adim=0; state=ERROR; return; }
    delay(ragleTime);
    adim = 3;
  }
  // Step motor hareketi
  else if (adim == 3) {
    if (donusYon == 0) {
      stepMotorHareket('L');
      stepOK = moveStepUntil(stepSolSensor, stepDist, "Step Sola Gidiyor");
    } else {
      stepMotorHareket('R');
      stepOK = moveStepUntil(stepSagSensor, stepDist, "Step Saga Gidiyor");
    }
    if (!stepOK) { ilk = true; adim=0; state=ERROR; return; }
    adim = 4;
  }
  // Pistonlar yukari
  else if (adim == 4) {
    tumPistonlarYukari();
    showOtoStatus("Pistonlar Yukari", "");
    if (!waitSensor(anaPistonYukariSensor, "AnaP Up") || !waitSensor(raglePistonYukariSensor, "Ragle Up")) {
      hataDur("Piston Yukari Hata"); ilk = true; adim=0; state=ERROR; return;
    }
    donusYon = !donusYon;
    strcpy(statusMsg, "Pedal Bekleniyor");
    adim = 0;
  }
  // Ana menüye dönmek için enkoder tuşu
  ClickEncoder::Button b = encoder.getButton();
  if (b == ClickEncoder::Clicked) { ilk = true; adim=0; state = MAIN_MENU; delay(200);} 
}

/*************** TEST MODU **********************/
const char* testMenuItems[] = {
  "AnaP DOWN", "AnaP UP", "Ragle DOWN", "Ragle UP", "Step LEFT", "Step RIGHT"
};
#define TEST_MENU_COUNT 6
void testModu() {
  // Menüde hareket seçimi
  int16_t enc = encoder.getValue();
  if (enc != encLast) {
    int diff = enc - encLast;
    encLast = enc;
    testMenuIndex = (testMenuIndex + diff + TEST_MENU_COUNT) % TEST_MENU_COUNT;
  }
  ClickEncoder::Button b = encoder.getButton();
  if (b == ClickEncoder::Clicked) {
    switch(testMenuIndex) {
      case 0: anaPistonAsagi();   break;
      case 1: anaPistonYukari();  break;
      case 2: raglePistonAsagi(); break;
      case 3: raglePistonYukari(); break;
      case 4: stepMotorHareket('L'); for (int i=0; i<50; i++) {stepMotorStep(); delayMicroseconds(stepSpeed);} break;
      case 5: stepMotorHareket('R'); for (int i=0; i<50; i++) {stepMotorStep(); delayMicroseconds(stepSpeed);} break;
    }
    delay(200);
  }
  // Ana menüye dönmek için enkoder tuşu uzun basım (LongPressed)
  if (b == ClickEncoder::Held) { state = MAIN_MENU; delay(300); }

  // LCD: Test menüsü ve sensör izleme
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0,10,"TEST MODU");
  for (byte i=0; i<TEST_MENU_COUNT; i++) {
    if (i == testMenuIndex) u8g2.drawStr(0, 20+12*i, ">");
    u8g2.drawStr(10, 20+12*i, testMenuItems[i]);
  }
  // Sensör durumları
  u8g2.setCursor(90, 12); u8g2.print("APUp:");
    u8g2.print(digitalRead(anaPistonYukariSensor) ? "1":"0");
  u8g2.setCursor(90, 22); u8g2.print("APDn:");
    u8g2.print(digitalRead(anaPistonAsagiSensor) ? "1":"0");
  u8g2.setCursor(90, 32); u8g2.print("RGUp:");
    u8g2.print(digitalRead(raglePistonYukariSensor) ? "1":"0");
  u8g2.setCursor(90, 42); u8g2.print("RGDn:");
    u8g2.print(digitalRead(raglePistonAsagiSensor) ? "1":"0");
  u8g2.setCursor(90, 52); u8g2.print("SL:");
    u8g2.print(digitalRead(stepSolSensor) ? "1":"0");
  u8g2.setCursor(90, 62); u8g2.print("SR:");
    u8g2.print(digitalRead(stepSagSensor) ? "1":"0");
  u8g2.sendBuffer();
}

/*************** AYAR MENÜSÜ ********************/
void ayarMenusu() {
  int16_t enc = encoder.getValue();
  if (enc != encLast) {
    int diff = enc - encLast;
    encLast = enc;
    switch(ayarMenuIndex) {
      case 0: stepSpeed = constrain(stepSpeed + diff*10, 100, 2000); break;
      case 1: stepDist  = constrain(stepDist + diff*10, 10, 10000); break;
      case 2: ragleTime = constrain(ragleTime + diff*10, 10, 1000); break;
      case 3: pistonTime = constrain(pistonTime + diff*10, 10, 1000); break;
    }
  }
  ClickEncoder::Button b = encoder.getButton();
  if (b == ClickEncoder::Clicked) {
    ayarMenuIndex = (ayarMenuIndex + 1) % AYAR_MENU_COUNT;
    // Ayarı kaydet
    switch(ayarMenuIndex) {
      case 0: EEPROMWriteInt(EEPROM_STEP_SPEED_ADDR, stepSpeed); break;
      case 1: EEPROMWriteInt(EEPROM_STEP_DIST_ADDR, stepDist); break;
      case 2: EEPROMWriteInt(EEPROM_RAGLE_TIME_ADDR, ragleTime); break;
      case 3: EEPROMWriteInt(EEPROM_PISTON_TIME_ADDR, pistonTime); break;
    }
    delay(200);
  }
  // Ana menüye dönmek için enkoder tuşu uzun basım (LongPressed)
  if (b == ClickEncoder::Held) { state = MAIN_MENU; delay(300); }
  // LCD
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  char buf[32];
  sprintf(buf, "%s %d", ayarMenuNames[ayarMenuIndex], getMenuValue(ayarMenuIndex));
  u8g2.drawStr(0, 12, buf);
  u8g2.drawStr(0, 28, "Menu: Enkoder bas");
  u8g2.drawStr(0, 44, "Deger: Cevir");
  u8g2.drawStr(0, 60, "Geri: Uzun bas");
  u8g2.sendBuffer();
}

/*************** HATA EKRANI ********************/
void hataEkrani() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 20, "HATA OLUSTU!");
  u8g2.drawStr(0, 36, statusMsg);
  u8g2.drawStr(0, 54, "Pedala bas: Devam");
  u8g2.sendBuffer();
  // Pedal ile resetle
  if (digitalRead(pedalButon) == LOW) {
    state = MAIN_MENU;
    strcpy(statusMsg, "Hazir");
    delay(200);
  }
}

/******************* Yardımcı Fonksiyonlar ********************/

// Otomatik modda adım adım durum
void showOtoStatus(const char* adim, const char* ek) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "Otomatik Mod");
  u8g2.drawStr(0, 26, adim);
  u8g2.drawStr(0, 42, statusMsg);
  u8g2.sendBuffer();
}

// Sensörden bekleme fonksiyonu
bool waitSensor(int pin, const char* msg) {
  unsigned long start = millis();
  while (digitalRead(pin) == LOW) {
    showOtoStatus(msg, "");
    if (millis() - start > 4000) return false; // 4sn timeout
  }
  return true;
}

// Step motor hareket fonksiyonu (güvenlikli)
bool moveStepUntil(int sensorPin, unsigned int maxStep, const char* msg) {
  unsigned int i = 0;
  while (digitalRead(sensorPin) == LOW && i < maxStep) {
    stepMotorStep();
    delayMicroseconds(stepSpeed);
    i++;
    showOtoStatus(msg, "");
  }
  if (digitalRead(sensorPin) == HIGH) {
    return true;
  } else {
    hataDur("Step/Sensor HATA");
    return false;
  }
}

void hataDur(const char* msg) {
  strcpy(statusMsg, msg);
  state = ERROR;
}

// Ayar menüsünde değeri döndür
unsigned int getMenuValue(byte idx) {
  switch(idx) {
    case 0: return stepSpeed;
    case 1: return stepDist;
    case 2: return ragleTime;
    case 3: return pistonTime;
    default: return 0;
  }
}

// EEPROM okuma/yazma
void EEPROMWriteInt(int address, int value) {
  byte lowByte = value & 0xFF;
  byte highByte = (value >> 8) & 0xFF;
  EEPROM.write(address, lowByte);
  EEPROM.write(address + 1, highByte);
}
int EEPROMReadInt(int address, int defaultValue) {
  byte lowByte = EEPROM.read(address);
  byte highByte = EEPROM.read(address + 1);
  int val = (highByte << 8) | lowByte;
  if(val == 0xFFFF) return defaultValue;
  return val;
}

// Piston ve step motor kontrolleri
void anaPistonAsagi() {
  digitalWrite(anaPistonA, LOW);
  digitalWrite(anaPistonB, HIGH);
}
void anaPistonYukari() {
  digitalWrite(anaPistonA, HIGH);
  digitalWrite(anaPistonB, LOW);
}
void raglePistonAsagi() {
  digitalWrite(raglePistonA, LOW);
  digitalWrite(raglePistonB, HIGH);
}
void raglePistonYukari() {
  digitalWrite(raglePistonA, HIGH);
  digitalWrite(raglePistonB, LOW);
}
void tumPistonlarYukari() {
  anaPistonYukari();
  raglePistonYukari();
}
void stepMotorHareket(char yon) {
  digitalWrite(dirPin, yon == 'L' ? LOW : HIGH);
}
void stepMotorStep() {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(2);
}