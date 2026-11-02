/*
  ===========================================================
   CODE TONG - HE SINH THAI NHAN TAO CHO CAY (UNO)
   Gop: DHT11 + Cam bien dat + Cam bien anh sang + MQ-135
        + Relay: Den (K1) - Quat (K2) - May bom (K3) - Phun suong (K4)
        + Coi canh bao + Gui du lieu sang ESP32 qua Serial

   2 che do:
   - THU CONG (mac dinh khi vua nap): bat/tat tung thiet bi qua Serial Monitor
     hoac qua nut bam tren dashboard web.
   - TU DONG: chay theo nguong nhiet do / do am / anh sang / gas / do am dat.
   Go 'h' + Enter tren Serial Monitor de xem lai huong dan lenh.

   KEYPAD (chi noi cot 1): 1 = Den, 4 = Quat, 7 = Suong (bam de dao bat/tat),
   * = Bom 5s (bam lan nua khi dang bom = dung), giu * 2s = doi che do.
   Bam phim khi dang TU DONG -> tu chuyen sang THU CONG.

   AN TOAN MAY BOM (vi bom, quat, suong dung chung adapter 5V-1A):
   - Moi lan bom chi chay toi da THOI_GIAN_BOM roi TU TAT (ca thu cong lan tu dong).
   - Trong luc bom chay, quat + suong bi TAM TAT, bom xong tu bat lai nhu cu.
   - Che do tu dong: sau moi lan bom phai nghi THOI_GIAN_NGHI_BOM cho nuoc ngam
     xuong dat roi moi duoc bom tiep; va KHONG bom neu cam bien dat dang nam
     ngoai khong khi (tranh bom lien tuc lam ngap hop).
  ===========================================================
*/

#include <DHT.h>
#include <SoftwareSerial.h>

// ============ KHAI BAO CHAN ============
#define DHTPIN      2      // DHT11 - nhiet do + do am khong khi
#define DHTTYPE     DHT11
#define SOILPIN     A0     // Cam bien do am dat (analog)
#define LIGHTPIN    A1     // Cam bien anh sang (analog)
#define GASPIN      A2     // MQ-135 (analog)

#define RELAY_DEN   3      // Kenh 1 - Grow Light
#define RELAY_QUAT  5      // Kenh 2 - Quat
#define RELAY_BOM   6      // Kenh 3 - May bom MB370
#define RELAY_SUONG 9      // Kenh 4 - Phun suong

#define COI_PIN     11     // Coi bao dong

#define ESP_TX      12     // UNO D12 -> qua cau chia ap -> ESP32 RX2 (D16)
#define ESP_RX      13     // UNO D13 <- ESP32 TX2 (D17), noi thang

// ============ KEYPAD 4x4 (chi noi COT 1) ============
// Hang R1..R4 -> D4, D7, D8, D10 ; Cot C1 -> A3 ; C2, C3, C4 bo trong
// Nen chi dung 4 phim cua cot 1:  1 = Den, 4 = Quat, 7 = Suong, * = Bom
// Giu phim * 2 giay = doi che do TU DONG <-> THU CONG
const byte KP_HANG[4] = {4, 7, 8, 10};
const char KP_PHIM[4] = {'1', '4', '7', '*'};
#define KP_COT1          A3
#define GIU_DOI_CHE_DO   2000UL   // ms giu phim * de doi che do

// Da test thuc te: module relay nay kich o muc HIGH = BAT, LOW = TAT
#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

// ============ NGUONG DIEU KHIEN TU DONG (CAN HIEU CHINH THEO THUC TE) ============
#define NHIET_DO_BAT_QUAT     28.0   // *C - cao hon muc nay thi bat quat
#define DO_AM_KK_BAT_SUONG    60.0   // %  - thap hon muc nay thi bat phun suong
#define NGUONG_SANG_BAT_DEN   400    // cam bien anh sang: NHO hon muc nay (toi) thi bat den
#define NGUONG_GAS_CANH_BAO   400    // MQ-135 raw vuot muc nay -> coi la bat thuong, keu coi
#define NGUONG_DAT_TUOI       40     // %  - do am dat thap hon muc nay thi tu dong bom

// ============ THONG SO MAY BOM ============
#define THOI_GIAN_BOM        5000UL   // ms - moi lan bom chay 5 giay roi tu tat
#define THOI_GIAN_NGHI_BOM   300000UL // ms - che do tu dong: nghi 5 phut giua 2 lan bom

// ============ HIEU CHINH CAM BIEN DAT (do thuc te, dieu chinh khi do lai) ============
#define DAT_KHO   1012   // gia tri raw khi cam bien de kho trong khong khi (0% am)
#define DAT_UOT   300    // gia tri raw khi cam vao dat/nuoc am (100% am)

// ============ BIEN TOAN CUC ============
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(ESP_RX, ESP_TX); // (RX, TX) = (13, 12)

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000; // DHT11 can toi thieu 2s giua 2 lan doc

float nhietDo = 0, doAmKK = 0;
int giaTriDat = 0, giaTriSang = 0, giaTriGas = 0;
int phanTramDat = 0; // do am dat da quy doi ra % (0-100)

bool trangThaiDen = false, trangThaiQuat = false, trangThaiSuong = false;
bool cheDoTuDong = false; // false = THU CONG (mac dinh), true = TU DONG

bool dangBom = false;
unsigned long lucBatDauBom = 0;
unsigned long lucBomXong = 0;
bool daTungBom = false;

// Bien cho keypad
char kpPhimDoc = 0;           // phim doc duoc o lan quet gan nhat
char kpPhimOnDinh = 0;        // phim da on dinh (da qua chong doi phim), 0 = khong bam
unsigned long kpLucDoiDoc = 0;
unsigned long kpLucBamXuong = 0;
unsigned long kpLanQuet = 0;
bool kpDaXuLyGiu = false;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  dht.begin();

  pinMode(RELAY_DEN, OUTPUT);
  pinMode(RELAY_QUAT, OUTPUT);
  pinMode(RELAY_BOM, OUTPUT);
  pinMode(RELAY_SUONG, OUTPUT);
  pinMode(COI_PIN, OUTPUT);

  // Keypad: hang de tha noi (INPUT) khi khong quet, cot dung dien tro keo len san co
  for (byte i = 0; i < 4; i++) pinMode(KP_HANG[i], INPUT);
  pinMode(KP_COT1, INPUT_PULLUP);

  // Tat het relay truoc khi lam nong MQ-135, tranh bat lon xon luc khoi dong
  apDungRelay();

  Serial.println("========================================");
  Serial.println(" HE SINH THAI CAY - CODE TONG (UNO)");
  Serial.println(" Dang lam nong MQ-135, cho 60 giay...");
  Serial.println("========================================");
  delay(60000); // thoi gian lam nong MQ-135

  inHuongDan();
}

void loop() {
  unsigned long now = millis();

  if (Serial.available())    xuLyLenh(Serial.read());    // lenh tu Serial Monitor
  if (espSerial.available()) xuLyLenh(espSerial.read()); // lenh tu dashboard web (qua ESP32)
  xuLyKeypad();                                           // lenh tu keypad

  // Bom tu tat khi het thoi gian (kiem tra lien tuc, khong doi chu ky 2s)
  if (dangBom && now - lucBatDauBom >= THOI_GIAN_BOM) {
    dungBom();
  }

  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;
    docCamBien();
    if (cheDoTuDong) dieuKhienTuDong();
    inRaSerial();
    guiSangESP32();
  }
}

// ---------- Ghi trang thai ra relay (co khoa an toan cho may bom) ----------
void apDungRelay() {
  digitalWrite(RELAY_DEN, trangThaiDen ? RELAY_ON : RELAY_OFF);
  // Bom, quat, suong chung adapter 5V-1A -> khi bom chay thi tam tat quat + suong
  digitalWrite(RELAY_QUAT,  (trangThaiQuat  && !dangBom) ? RELAY_ON : RELAY_OFF);
  digitalWrite(RELAY_SUONG, (trangThaiSuong && !dangBom) ? RELAY_ON : RELAY_OFF);
  digitalWrite(RELAY_BOM, dangBom ? RELAY_ON : RELAY_OFF);
}

void batDauBom() {
  dangBom = true;
  lucBatDauBom = millis();
  apDungRelay();
  Serial.print(">> BOM: BAT (tu tat sau "); Serial.print(THOI_GIAN_BOM / 1000); Serial.println("s)");
}

void dungBom() {
  if (!dangBom) return;
  dangBom = false;
  lucBomXong = millis();
  daTungBom = true;
  apDungRelay(); // bat lai quat/suong neu truoc do dang bat
  Serial.println(">> BOM: TAT");
}

// ---------- Xu ly lenh dieu khien thu cong ----------
// Lenh "DAT DUNG TRANG THAI": chu HOA = BAT, chu thuong = TAT
// (gui trung hoac roi 1 lan tren duong truyen cung khong lam sai ket qua).
void xuLyLenh(char c) {
  bool laLenhThietBi = (c == 'D' || c == 'd' || c == 'Q' || c == 'q' ||
                        c == 'S' || c == 's' || c == 'B' || c == 'b');
  if (laLenhThietBi && cheDoTuDong) {
    Serial.println(">> Dang o che do TU DONG, go 'm' de chuyen THU CONG truoc.");
    return;
  }

  switch (c) {
    case 'D': case 'd':
      trangThaiDen = (c == 'D');
      apDungRelay();
      Serial.print(">> DEN: "); Serial.println(trangThaiDen ? "BAT" : "TAT");
      break;

    case 'Q': case 'q':
      trangThaiQuat = (c == 'Q');
      apDungRelay();
      Serial.print(">> QUAT: "); Serial.println(trangThaiQuat ? "BAT" : "TAT");
      break;

    case 'S': case 's':
      trangThaiSuong = (c == 'S');
      apDungRelay();
      Serial.print(">> SUONG: "); Serial.println(trangThaiSuong ? "BAT" : "TAT");
      break;

    case 'B': // Bat bom (tu tat sau THOI_GIAN_BOM)
      batDauBom();
      break;

    case 'b': // Tat bom ngay
      dungBom();
      break;

    case 'm':
      cheDoTuDong = false;
      Serial.println(">> Chuyen sang che do THU CONG");
      break;

    case 'a':
      cheDoTuDong = true;
      Serial.println(">> Chuyen sang che do TU DONG (chay theo nguong)");
      break;

    case 'h':
      inHuongDan();
      break;

    default:
      break; // bo qua Enter / xuong dong / ky tu la
  }
}

// ---------- KEYPAD ----------
// Quet 4 hang: moi lan chi keo 1 hang xuong LOW, cac hang khac tha noi (INPUT)
// -> bam 2 phim cung luc cung khong bi chap 2 chan Arduino vao nhau.
char quetPhim() {
  char k = 0;
  for (byte i = 0; i < 4 && k == 0; i++) {
    pinMode(KP_HANG[i], OUTPUT);
    digitalWrite(KP_HANG[i], LOW);
    delayMicroseconds(10);
    if (digitalRead(KP_COT1) == LOW) k = KP_PHIM[i];
    pinMode(KP_HANG[i], INPUT);
  }
  return k;
}

void xuLyKeypad() {
  unsigned long now = millis();
  if (now - kpLanQuet < 10) return; // quet 100 lan/giay la du
  kpLanQuet = now;

  char k = quetPhim();
  if (k != kpPhimDoc) {             // vua thay doi -> cho on dinh (chong doi phim)
    kpPhimDoc = k;
    kpLucDoiDoc = now;
    return;
  }
  if (now - kpLucDoiDoc < 30) return;

  if (k == kpPhimOnDinh) {          // dang giu nguyen phim
    if (k == '*' && !kpDaXuLyGiu && now - kpLucBamXuong >= GIU_DOI_CHE_DO) {
      kpDaXuLyGiu = true;
      doiCheDoBangPhim();
    }
    return;
  }

  char phimCu = kpPhimOnDinh;
  kpPhimOnDinh = k;
  if (k != 0) {                     // vua bam xuong
    kpLucBamXuong = now;
    kpDaXuLyGiu = false;
    if (k != '*') phimBam(k);       // 1, 4, 7: xu ly ngay khi bam
  } else if (phimCu == '*' && !kpDaXuLyGiu) {
    phimBam('*');                   // *: xu ly khi nha tay (de con phan biet bam ngan / giu lau)
  }
}

void phimBam(char k) {
  Serial.print(">> [KEYPAD] Phim "); Serial.println(k);
  if (cheDoTuDong) {                // dang tu dong -> bam phim nghia la muon dieu khien tay
    cheDoTuDong = false;
    Serial.println(">> [KEYPAD] Tu chuyen sang THU CONG");
  }
  tone(COI_PIN, 2000, 60);          // tieng bip ngan xac nhan da nhan phim

  switch (k) {
    case '1': xuLyLenh(trangThaiDen   ? 'd' : 'D'); break; // dao den
    case '4': xuLyLenh(trangThaiQuat  ? 'q' : 'Q'); break; // dao quat
    case '7': xuLyLenh(trangThaiSuong ? 's' : 'S'); break; // dao suong
    case '*': xuLyLenh(dangBom        ? 'b' : 'B'); break; // bom 1 lan / dung bom
  }
  guiSangESP32(); // cap nhat ngay len dashboard + Telegram
}

void doiCheDoBangPhim() {
  xuLyLenh(cheDoTuDong ? 'm' : 'a');
  // Tu dong = 1 tieng bip cao dai, Thu cong = 1 tieng bip tram dai
  if (cheDoTuDong) tone(COI_PIN, 2500, 500);
  else             tone(COI_PIN, 800, 500);
  guiSangESP32();
}

void inHuongDan() {
  Serial.println("========================================");
  Serial.println(" DIEU KHIEN QUA SERIAL MONITOR");
  Serial.println(" Go ky tu roi bam Enter/Send:");
  Serial.println("   D / d = Bat / Tat DEN   (Kenh 1)");
  Serial.println("   Q / q = Bat / Tat QUAT  (Kenh 2)");
  Serial.println("   B / b = Bat / Tat BOM   (Kenh 3) - tu tat sau 5s");
  Serial.println("   S / s = Bat / Tat SUONG (Kenh 4)");
  Serial.println("   m = Che do THU CONG (mac dinh)");
  Serial.println("   a = Che do TU DONG (theo nguong)");
  Serial.println("   h = Hien lai huong dan nay");
  Serial.println(" KEYPAD: 1 = Den | 4 = Quat | 7 = Suong | * = Bom 5s");
  Serial.println("         Giu * 2 giay = doi TU DONG <-> THU CONG");
  Serial.println("========================================");
}

// ---------- Doc tat ca cam bien ----------
void docCamBien() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) doAmKK = h;
  if (!isnan(t)) nhietDo = t;

  giaTriDat  = analogRead(SOILPIN);
  giaTriSang = analogRead(LIGHTPIN);
  giaTriGas  = analogRead(GASPIN);

  phanTramDat = map(giaTriDat, DAT_KHO, DAT_UOT, 0, 100);
  phanTramDat = constrain(phanTramDat, 0, 100);
}

// ---------- Logic dieu khien tu dong (chi chay khi cheDoTuDong = true) ----------
void dieuKhienTuDong() {
  trangThaiDen   = (giaTriSang < NGUONG_SANG_BAT_DEN);
  trangThaiQuat  = (nhietDo > NHIET_DO_BAT_QUAT) || (giaTriGas > NGUONG_GAS_CANH_BAO);
  trangThaiSuong = (doAmKK < DO_AM_KK_BAT_SUONG);

  // Tu dong tuoi khi dat kho
  bool camBienNgoaiKhongKhi = (giaTriDat >= DAT_KHO - 15); // gan bang luc de ngoai khong khi
  bool daNghiDu = !daTungBom || (millis() - lucBomXong >= THOI_GIAN_NGHI_BOM);

  if (!dangBom && phanTramDat < NGUONG_DAT_TUOI && daNghiDu) {
    if (camBienNgoaiKhongKhi) {
      Serial.println("!! Cam bien dat co ve dang nam ngoai dat -> KHONG bom de tranh ngap.");
    } else {
      batDauBom();
    }
  }

  apDungRelay();

  if (giaTriGas > NGUONG_GAS_CANH_BAO) {
    tone(COI_PIN, 1500, 300);
  }
}

// ---------- In ra Serial Monitor de theo doi khi test ----------
void inRaSerial() {
  Serial.print("["); Serial.print(cheDoTuDong ? "TU DONG" : "THU CONG"); Serial.print("] ");
  Serial.print("Nhiet do: "); Serial.print(nhietDo, 1); Serial.print(" C | ");
  Serial.print("Do am KK: "); Serial.print(doAmKK, 0); Serial.print(" % | ");
  Serial.print("Dat: "); Serial.print(giaTriDat); Serial.print(" (");
  Serial.print(phanTramDat); Serial.print("%) | ");
  Serial.print("Sang: "); Serial.print(giaTriSang); Serial.print(" | ");
  Serial.print("Gas: "); Serial.print(giaTriGas); Serial.print(" || ");
  Serial.print("Den:");   Serial.print(trangThaiDen ? "ON" : "off");   Serial.print(" ");
  Serial.print("Quat:");  Serial.print(trangThaiQuat ? "ON" : "off");  Serial.print(" ");
  Serial.print("Bom:");   Serial.print(dangBom ? "ON" : "off");        Serial.print(" ");
  Serial.print("Suong:"); Serial.println(trangThaiSuong ? "ON" : "off");
}

// ---------- Gui du lieu sang ESP32 ----------
void guiSangESP32() {
  espSerial.print("T:");        espSerial.print(nhietDo, 1);
  espSerial.print(",H:");       espSerial.print(doAmKK, 0);
  espSerial.print(",SOIL:");    espSerial.print(giaTriDat);
  espSerial.print(",SOILPCT:"); espSerial.print(phanTramDat);
  espSerial.print(",LUX:");     espSerial.print(giaTriSang);
  espSerial.print(",GAS:");     espSerial.print(giaTriGas);
  espSerial.print(",DEN:");     espSerial.print(trangThaiDen ? 1 : 0);
  espSerial.print(",QUAT:");    espSerial.print(trangThaiQuat ? 1 : 0);
  espSerial.print(",SUONG:");   espSerial.print(trangThaiSuong ? 1 : 0);
  espSerial.print(",BOM:");     espSerial.print(dangBom ? 1 : 0);
  espSerial.print(",MODE:");    espSerial.print(cheDoTuDong ? 1 : 0);
  // Nguong dang dat, de dashboard hien thi dung voi code dang chay
  espSerial.print(",NGT:");     espSerial.print(NHIET_DO_BAT_QUAT, 1);
  espSerial.print(",NGA:");     espSerial.print(DO_AM_KK_BAT_SUONG, 1);
  espSerial.print(",NGS:");     espSerial.print(NGUONG_SANG_BAT_DEN);
  espSerial.print(",NGG:");     espSerial.print(NGUONG_GAS_CANH_BAO);
  espSerial.print(",NGD:");     espSerial.println(NGUONG_DAT_TUOI);
}
