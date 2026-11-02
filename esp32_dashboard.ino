/*
  ===========================================================
   ESP32 - DASHBOARD WEB + TELEGRAM DIEU KHIEN HE SINH THAI CAY
   - Nhan du lieu cam bien tu UNO qua Serial2 (D16=RX2, D17=TX2)
   - Dashboard web (mo bang trinh duyet khi o cung WiFi / WiFi rieng cua ESP32)
   - Telegram bot: dieu khien + xem trang thai TU XA qua internet, nhan canh bao
   - Dinh ky day du lieu len ThingSpeak
  ===========================================================
*/
const char* WIFI_SSID     = "";
const char* WIFI_PASS     = "";
const char* TS_WRITE_KEY  = "";

// ----- Telegram -----
const char* TG_BOT_TOKEN  = "";
// Chat ID cua CHU bot. Bot CHI nhan lenh tu dung Chat ID nay.
// Neu de trong "": bot khong nhan lenh nao, chi tra loi Chat ID cua nguoi nhan tin
// de ban dien vao day roi nap lai.
const char* TG_CHAT_ID    = "";

// ESP32 tu phat 1 WiFi rieng (AP) de vao dashboard KHONG can qua WiFi nha.
const char* AP_SSID = "HeSinhThaiCay";
const char* AP_PASS = "12345678";

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "trang_web.h"   // noi dung trang HTML nam trong file nay

WebServer server(80);

// ============ BIEN LUU DU LIEU MOI NHAT NHAN TU UNO ============
float nhietDo = 0, doAmKK = 0;
int giaTriDat = 0, giaTriSang = 0, giaTriGas = 0;
int phanTramDat = 0;
bool trangThaiDen = false, trangThaiQuat = false, trangThaiSuong = false, trangThaiBom = false;
bool cheDoTuDong = false;
unsigned long lanNhanCuoi = 0; // millis() cua lan nhan du lieu gan nhat tu UNO
bool daNhanDuLieu = false;

// Nguong dang duoc UNO ap dung (chi de HIEN THI)
float nguongNhiet = 0, nguongAm = 0;
int nguongSang = 0, nguongGas = 0, nguongDat = 0;

String bufferNhan = ""; // gom ky tu tu Serial2 cho den khi gap '\n'

unsigned long lastThingSpeak = 0;
const unsigned long THINGSPEAK_INTERVAL = 20000; // >=15s theo yeu cau ThingSpeak free

// ============ BIEN CHO TELEGRAM ============
WiFiClientSecure tgClient;
HTTPClient tgHttp;
long tgOffset = 0;
bool tgDaBoQuaTinCu = false;      // lan dau: bo qua cac lenh cu gui luc ESP32 dang tat
bool tgDaChaoKhoiDong = false;
unsigned long lastTgPoll = 0;
const unsigned long TG_POLL_INTERVAL = 4000;   // 4s hoi Telegram 1 lan
unsigned long tgHenGuiTrangThai = 0;           // != 0: den thoi diem nay thi gui trang thai

unsigned long lastKiemTraCanhBao = 0;
unsigned long lanCanhBaoGas = 0, lanCanhBaoDat = 0;
bool daCanhBaoGas = false, daCanhBaoDat = false, daBaoMatUNO = false;
const unsigned long CANH_BAO_NGHI = 30UL * 60UL * 1000UL; // moi loai canh bao cach nhau 30 phut

const char TG_BAN_PHIM[] =
  "{\"keyboard\":[[\"/trangthai\"],"
  "[\"/den_bat\",\"/den_tat\"],"
  "[\"/quat_bat\",\"/quat_tat\"],"
  "[\"/suong_bat\",\"/suong_tat\"],"
  "[\"/bom\",\"/bom_tat\"],"
  "[\"/tudong\",\"/thucong\"]],"
  "\"resize_keyboard\":true}";

// ============ TACH GIA TRI TU CHUOI DU LIEU UNO GUI ============
String layGiaTri(const String &data, const String &key) {
  int idx = data.indexOf(key + ":");
  if (idx == -1) return "";
  idx += key.length() + 1;
  int end = data.indexOf(',', idx);
  if (end == -1) end = data.length();
  return data.substring(idx, end);
}

void xuLyDongDuLieu(const String &line) {
  if (line.indexOf("T:") == -1) return; // dong khong hop le, bo qua
  nhietDo      = layGiaTri(line, "T").toFloat();
  doAmKK       = layGiaTri(line, "H").toFloat();
  giaTriDat    = layGiaTri(line, "SOIL").toInt();
  phanTramDat  = layGiaTri(line, "SOILPCT").toInt();
  giaTriSang   = layGiaTri(line, "LUX").toInt();
  giaTriGas    = layGiaTri(line, "GAS").toInt();
  trangThaiDen = layGiaTri(line, "DEN").toInt() == 1;
  trangThaiQuat= layGiaTri(line, "QUAT").toInt() == 1;
  trangThaiSuong = layGiaTri(line, "SUONG").toInt() == 1;
  trangThaiBom = layGiaTri(line, "BOM").toInt() == 1;
  cheDoTuDong  = layGiaTri(line, "MODE").toInt() == 1;
  nguongNhiet  = layGiaTri(line, "NGT").toFloat();
  nguongAm     = layGiaTri(line, "NGA").toFloat();
  nguongSang   = layGiaTri(line, "NGS").toInt();
  nguongGas    = layGiaTri(line, "NGG").toInt();
  nguongDat    = layGiaTri(line, "NGD").toInt();
  lanNhanCuoi  = millis();
  daNhanDuLieu = true;
}

bool duLieuConMoi() {
  return daNhanDuLieu && (millis() - lanNhanCuoi < 10000);
}

// Gui 1 lenh (1 ky tu) sang UNO. Gui 2 lan cho chac vi SoftwareSerial ben UNO hay roi ky tu;
// an toan vi day la lenh "dat dung trang thai", gui trung khong lam sai ket qua.
void guiLenhUNO(char c) {
  Serial2.print(c);
  delay(30);
  Serial2.print(c);
}

// ============ CAC ROUTE WEB ============
void handleRoot() {
  server.send_P(200, "text/html", TRANG_CHINH);
}

void handleData() {
  String json = "{";
  json += "\"t\":" + String(nhietDo, 1) + ",";
  json += "\"h\":" + String(doAmKK, 0) + ",";
  json += "\"soil\":" + String(giaTriDat) + ",";
  json += "\"soilpct\":" + String(phanTramDat) + ",";
  json += "\"lux\":" + String(giaTriSang) + ",";
  json += "\"gas\":" + String(giaTriGas) + ",";
  json += "\"den\":" + String(trangThaiDen ? 1 : 0) + ",";
  json += "\"quat\":" + String(trangThaiQuat ? 1 : 0) + ",";
  json += "\"suong\":" + String(trangThaiSuong ? 1 : 0) + ",";
  json += "\"bom\":" + String(trangThaiBom ? 1 : 0) + ",";
  json += "\"mode\":" + String(cheDoTuDong ? 1 : 0) + ",";
  json += "\"ngt\":" + String(nguongNhiet, 1) + ",";
  json += "\"nga\":" + String(nguongAm, 1) + ",";
  json += "\"ngs\":" + String(nguongSang) + ",";
  json += "\"ngg\":" + String(nguongGas) + ",";
  json += "\"ngd\":" + String(nguongDat) + ",";
  json += "\"tuoi\":" + String((millis() - lanNhanCuoi) / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCmd() {
  if (!server.hasArg("c")) { server.send(400, "text/plain", "thieu tham so c"); return; }
  String c = server.arg("c");
  if (c.length() == 1) guiLenhUNO(c[0]);
  server.send(200, "text/plain", "ok");
}

// ============ THINGSPEAK ============
void guiThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + String(TS_WRITE_KEY) +
               "&field1=" + String(nhietDo, 1) +
               "&field2=" + String(doAmKK, 0) +
               "&field3=" + String(giaTriDat) +
               "&field4=" + String(giaTriSang) +
               "&field5=" + String(giaTriGas);
  http.begin(url);
  http.GET();
  http.end();
}

// ============ TELEGRAM ============
String jsonEsc(const String &s) {
  String o;
  o.reserve(s.length() + 16);
  for (unsigned int i = 0; i < s.length(); i++) {
    char ch = s[i];
    if (ch == '\\' || ch == '"') { o += '\\'; o += ch; }
    else if (ch == '\n') o += "\\n";
    else o += ch;
  }
  return o;
}

// Goi 1 ham cua Telegram Bot API (POST JSON). Tra ve noi dung phan hoi, "" neu loi.
String tgGoi(const String &method, const String &body) {
  if (WiFi.status() != WL_CONNECTED) return "";
  String url = "https://api.telegram.org/bot" + String(TG_BOT_TOKEN) + "/" + method;
  if (!tgHttp.begin(tgClient, url)) return "";
  tgHttp.addHeader("Content-Type", "application/json");
  int code = tgHttp.POST(body);
  String res = (code > 0) ? tgHttp.getString() : "";
  tgHttp.end();
  return res;
}

void tgGui(const String &chatId, const String &text) {
  String body = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + jsonEsc(text) +
                "\",\"reply_markup\":" + String(TG_BAN_PHIM) + "}";
  tgGoi("sendMessage", body);
}

void tgGuiChu(const String &text) {
  if (strlen(TG_CHAT_ID) == 0) return;
  tgGui(String(TG_CHAT_ID), text);
}

String tgChuoiTrangThai() {
  String s = "📊 TRẠNG THÁI - chế độ: ";
  s += cheDoTuDong ? "TỰ ĐỘNG" : "THỦ CÔNG";
  s += "\n";
  if (!duLieuConMoi()) {
    s += "⚠️ Chưa nhận được dữ liệu mới từ UNO, số liệu dưới đây có thể đã cũ.\n";
  }
  s += "🌡️ Nhiệt độ: " + String(nhietDo, 1) + " °C\n";
  s += "💧 Độ ẩm không khí: " + String(doAmKK, 0) + " %\n";
  s += "🪴 Độ ẩm đất: " + String(phanTramDat) + " % (raw " + String(giaTriDat) + ")\n";
  s += "☀️ Ánh sáng: " + String(giaTriSang) + "\n";
  s += "💨 Khí gas: " + String(giaTriGas) + "\n";
  s += "💡 Đèn: ";    s += trangThaiDen ? "BẬT" : "tắt";
  s += "\n🌀 Quạt: "; s += trangThaiQuat ? "BẬT" : "tắt";
  s += "\n💦 Sương: "; s += trangThaiSuong ? "BẬT" : "tắt";
  s += "\n🚿 Bơm: ";  s += trangThaiBom ? "ĐANG BƠM" : "tắt";
  return s;
}

String tgChuoiHuongDan() {
  return String("🌱 Hệ sinh thái cây - các lệnh:\n") +
         "/trangthai - xem cảm biến + thiết bị\n" +
         "/den_bat  /den_tat - đèn\n" +
         "/quat_bat  /quat_tat - quạt\n" +
         "/suong_bat  /suong_tat - phun sương\n" +
         "/bom - bơm 1 lần (tự tắt)   /bom_tat - dừng bơm\n" +
         "/tudong - chế độ tự động theo ngưỡng\n" +
         "/thucong - chế độ thủ công\n" +
         "Bật/tắt từng thiết bị chỉ dùng được ở chế độ thủ công.";
}

void tgXuLyLenh(const String &chatId, String cmd) {
  // Chua cau hinh chu bot: chi tra ve Chat ID, khong nhan lenh nao
  if (strlen(TG_CHAT_ID) == 0) {
    tgGui(chatId, "Chat ID của bạn là: " + chatId +
                  "\nĐiền số này vào dòng TG_CHAT_ID trong esp32_dashboard.ino rồi nạp lại để bot bắt đầu nhận lệnh.");
    return;
  }
  // Nguoi la nhan tin: bo qua hoan toan
  if (chatId != String(TG_CHAT_ID)) {
    Serial.print("Telegram: bo qua tin nhan tu chat la "); Serial.println(chatId);
    return;
  }

  cmd.trim();
  cmd.toLowerCase();
  int viTriA = cmd.indexOf('@');           // "/bom@ten_bot" -> "/bom"
  if (viTriA > 0) cmd = cmd.substring(0, viTriA);
  int viTriCach = cmd.indexOf(' ');
  if (viTriCach > 0) cmd = cmd.substring(0, viTriCach);

  if (cmd == "/start" || cmd == "/help") { tgGuiChu(tgChuoiHuongDan()); return; }
  if (cmd == "/trangthai")               { tgGuiChu(tgChuoiTrangThai()); return; }

  if (cmd == "/tudong") {
    guiLenhUNO('a');
    tgGuiChu("Đã gửi lệnh chuyển sang chế độ TỰ ĐỘNG. Mình sẽ báo trạng thái sau vài giây.");
    tgHenGuiTrangThai = millis() + 5000;
    return;
  }
  if (cmd == "/thucong") {
    guiLenhUNO('m');
    tgGuiChu("Đã gửi lệnh chuyển sang chế độ THỦ CÔNG. Mình sẽ báo trạng thái sau vài giây.");
    tgHenGuiTrangThai = millis() + 5000;
    return;
  }

  char ma = 0;
  if      (cmd == "/den_bat")   ma = 'D';
  else if (cmd == "/den_tat")   ma = 'd';
  else if (cmd == "/quat_bat")  ma = 'Q';
  else if (cmd == "/quat_tat")  ma = 'q';
  else if (cmd == "/suong_bat") ma = 'S';
  else if (cmd == "/suong_tat") ma = 's';
  else if (cmd == "/bom")       ma = 'B';
  else if (cmd == "/bom_tat")   ma = 'b';

  if (ma == 0) {
    tgGuiChu("Không hiểu lệnh này. Gửi /help để xem danh sách lệnh.");
    return;
  }
  if (cheDoTuDong) {
    tgGuiChu("Đang ở chế độ TỰ ĐỘNG nên không bật/tắt tay được. Gửi /thucong trước.");
    return;
  }
  if (!duLieuConMoi()) {
    tgGuiChu("⚠️ ESP32 đang không nhận được dữ liệu từ UNO, lệnh có thể không tới nơi. Vẫn thử gửi...");
  }
  guiLenhUNO(ma);
  tgGuiChu("Đã gửi lệnh " + cmd + ". Mình sẽ báo trạng thái sau vài giây.");
  tgHenGuiTrangThai = millis() + 5000;
}

// Hoi Telegram xem co tin nhan moi khong (moi lan lay toi da 1 tin)
void tgKiemTraTinMoi() {
  String body;
  if (!tgDaBoQuaTinCu) body = "{\"offset\":-1,\"limit\":1,\"timeout\":0}";
  else body = "{\"offset\":" + String(tgOffset) + ",\"limit\":1,\"timeout\":0,\"allowed_updates\":[\"message\"]}";

  String res = tgGoi("getUpdates", body);
  if (res.indexOf("\"ok\":true") == -1) return; // loi mang / token sai -> thu lai lan sau

  int idx = res.indexOf("\"update_id\":");
  if (idx == -1) {                 // khong co tin nao
    tgDaBoQuaTinCu = true;
    return;
  }
  long updateId = res.substring(idx + 12).toInt();
  tgOffset = updateId + 1;

  if (!tgDaBoQuaTinCu) {           // tin cu tu truoc khi ESP32 bat len -> khong thuc hien
    tgDaBoQuaTinCu = true;
    return;
  }

  int idxChat = res.indexOf("\"chat\":{\"id\":");
  if (idxChat == -1) return;
  int p = idxChat + 13;
  String chatId = "";
  while (p < (int)res.length() && (isDigit(res[p]) || res[p] == '-')) chatId += res[p++];

  int idxText = res.indexOf("\"text\":\"", idxChat);
  if (idxText == -1) return;       // tin khong phai van ban (anh, sticker...)
  p = idxText + 8;
  String text = "";
  while (p < (int)res.length()) {
    char ch = res[p];
    if (ch == '\\' && p + 1 < (int)res.length()) { text += res[p + 1]; p += 2; continue; }
    if (ch == '"') break;
    text += ch;
    p++;
  }

  Serial.print("Telegram nhan: "); Serial.println(text);
  tgXuLyLenh(chatId, text);
}

// Canh bao tu dong gui ve Telegram
void kiemTraCanhBao() {
  if (strlen(TG_CHAT_ID) == 0 || WiFi.status() != WL_CONNECTED) return;
  unsigned long now = millis();

  // Mat lien lac voi UNO qua 60s
  bool matUNO = daNhanDuLieu && (now - lanNhanCuoi > 60000);
  if (matUNO && !daBaoMatUNO) {
    daBaoMatUNO = true;
    tgGuiChu("⚠️ ESP32 không nhận được dữ liệu từ UNO hơn 60 giây. Kiểm tra nguồn UNO hoặc dây nối D12/D13.");
  } else if (!matUNO && daBaoMatUNO) {
    daBaoMatUNO = false;
    tgGuiChu("✅ Đã nhận lại dữ liệu từ UNO.");
  }
  if (!duLieuConMoi()) return;

  // Khi gas vuot nguong
  if (nguongGas > 0 && giaTriGas > nguongGas) {
    if (!daCanhBaoGas || now - lanCanhBaoGas > CANH_BAO_NGHI) {
      daCanhBaoGas = true; lanCanhBaoGas = now;
      tgGuiChu("🚨 Khí gas cao bất thường: " + String(giaTriGas) + " (ngưỡng " + String(nguongGas) + ").");
    }
  }

  // Dat kho trong luc dang o che do thu cong (che do tu dong thi UNO tu tuoi)
  if (!cheDoTuDong && nguongDat > 0 && phanTramDat < nguongDat) {
    if (!daCanhBaoDat || now - lanCanhBaoDat > CANH_BAO_NGHI) {
      daCanhBaoDat = true; lanCanhBaoDat = now;
      tgGuiChu("🪴 Đất đang khô: " + String(phanTramDat) + "% (ngưỡng " + String(nguongDat) +
               "%). Gửi /bom để tưới, hoặc /tudong để hệ thống tự tưới.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.setRxBufferSize(1024); // du cho vai dong du lieu UNO trong luc ESP32 ban goi Telegram
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX2=16 (nhan tu UNO), TX2=17 (gui cho UNO)

  // Che do AP_STA: vua tu phat WiFi rieng, vua ket noi WiFi nha cung luc
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("========================================");
  Serial.print("Da phat WiFi rieng, ten mang: "); Serial.println(AP_SSID);
  Serial.print("Mat khau: "); Serial.println(AP_PASS);
  Serial.print("Vao dashboard qua WiFi nay tai: http://");
  Serial.println(WiFi.softAPIP()); // co dinh: 192.168.4.1
  Serial.println("========================================");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang thu ket noi WiFi nha (toi da 15s)");
  unsigned long batDauCho = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - batDauCho < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Da ket noi WiFi nha! Vao dashboard qua WiFi nha tai: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Chua ket noi duoc WiFi nha - chay tiep voi WiFi rieng cua ESP32, se tu thu ket noi lai.");
    Serial.println("Telegram + ThingSpeak chi hoat dong khi co WiFi nha.");
  }

  // Telegram dung HTTPS; bo qua kiem tra chung chi cho nhe (du dung cho do an)
  tgClient.setInsecure();
  tgHttp.setReuse(true);
  tgHttp.setTimeout(5000);
  if (strlen(TG_CHAT_ID) == 0) {
    Serial.println("Telegram: CHUA co TG_CHAT_ID -> nhan tin bat ky cho bot de bot tra ve Chat ID.");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/cmd", handleCmd);
  server.begin();
}

void loop() {
  server.handleClient();

  // Doc du lieu tu UNO gui sang (gom tung dong ket thuc bang \n)
  while (Serial2.available()) {
    char ch = Serial2.read();
    if (ch == '\n') {
      xuLyDongDuLieu(bufferNhan);
      bufferNhan = "";
    } else if (ch != '\r') {
      if (bufferNhan.length() < 400) bufferNhan += ch;
    }
  }

  unsigned long now = millis();

  if (now - lastThingSpeak >= THINGSPEAK_INTERVAL) {
    lastThingSpeak = now;
    guiThingSpeak();
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!tgDaChaoKhoiDong && strlen(TG_CHAT_ID) > 0) {
      tgDaChaoKhoiDong = true;
      tgGuiChu("✅ Hệ sinh thái cây đã online. Gửi /help để xem lệnh.");
    }

    if (now - lastTgPoll >= TG_POLL_INTERVAL) {
      lastTgPoll = now;
      tgKiemTraTinMoi();
    }

    if (tgHenGuiTrangThai != 0 && (long)(millis() - tgHenGuiTrangThai) >= 0) {
      tgHenGuiTrangThai = 0;
      tgGuiChu(tgChuoiTrangThai());
    }

    if (now - lastKiemTraCanhBao >= 10000) {
      lastKiemTraCanhBao = now;
      kiemTraCanhBao();
    }
  }
}
