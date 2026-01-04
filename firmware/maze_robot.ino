#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// địa chỉ I2C LCD thường là 0x27 || 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======= Cấu hình Wi-Fi & MQTT =======
const char* ssid     = "Minh Tuấn";
const char* password = "11111112";
const char* broker   = "test.mosquitto.org";
const int   mqttPort = 1883;
const char* clientID = "esp32_robot";

WiFiClient     espClient;
PubSubClient   mqtt(espClient);

// gọi trong bất cứ đâu nếu mất Wi-Fi
void wifiReconnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// gọi trong loop nếu mất MQTT
void mqttReconnect() {
  while (!mqtt.connected()) {
    mqtt.connect(clientID);
    delay(500);
  }
}
// khai báo chân I2C 
#define SDA_PIN 4
#define SCL_PIN 5

// --- Định nghĩa chân HC-SR04 -
// Cảm biến 0 (phía trước)
#define TRIG0 12
#define ECHO0 13

// Cảm biến 1 (bên trái)
#define TRIG1 14
#define ECHO1 27

// Cảm biến 2 (bên phải)
#define TRIG2 25
#define ECHO2 26

// Định nghĩa chân động cơ và chân encoder cho động cơ 1
#define IN1 16  // Chân điều khiển chiều dương động cơ 1
#define IN2 17  // Chân điều khiển chiều âm động cơ 1
#define A1 22   // Chân tín hiệu kênh A encoder của động cơ 1
#define B1 23   // Chân tín hiệu kênh B encoder của động cơ 1

// Định nghĩa chân động cơ và chân encoder cho động cơ 2
#define IN3 18  // Chân điều khiển chiều dương động cơ 2
#define IN4 19  // Chân điều khiển chiều âm động cơ 2
#define A2 32   // Chân tín hiệu kênh A encoder của động cơ 2
#define B2 33   // Chân tín hiệu kênh B encoder của động cơ 2

// Gia trị encoder ban đầu là 0 cho 2 động cơ 
volatile int vitri1 = 0;   // Vị trí động cơ 1
volatile int vitri2 = 0;   // Vị trí động cơ 2

// --- Các hằng số điều khiển ---
#define NGUONG 22  // Ngưỡng khoảng cách an toàn (cm) của cảm biến phía trước
#define NGUONG_MEM 18 


//Các biến cho hàm PID
double lastErr;
double lastTime;
double errSum;


// --- Hàm đo khoảng cách từ cảm biến ---
float doKhoangCach(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long doDaiXung = pulseIn(echo, HIGH);
  float khoangCach = doDaiXung * 0.034 / 2;  // Tính khoảng cách (cm)
  return khoangCach;
}


// --- Hàm cập nhật vị trí encoder ---
void updateVitri1() {
  if (digitalRead(B1) == HIGH)
    vitri1--;
  else
    vitri1++;
}

void updateVitri2() {
  if (digitalRead(B2) == HIGH)
    vitri2++;
  else
    vitri2--;
}



// --- Hàm quay động cơ theo chiều dương/âm ---
void quay(int speedL, int speedR) {
  if(speedL > 0) {
    analogWrite(IN2, speedL);
    analogWrite(IN1, 0);
  }else if(speedL < 0) {
    analogWrite(IN1, abs(speedL));
    analogWrite(IN2, 0);
  }else{ //Trường hợp còn lại là speedL = 0 đẩy lên cao để phanh bằng motor, hạn chế trôi
    analogWrite(IN2, 255);
    analogWrite(IN1, 255);
  }

  if(speedR > 0) {
    analogWrite(IN3, speedR);
    analogWrite(IN4, 0);
  }else if(speedR < 0) {
    analogWrite(IN4, abs(speedR));
    analogWrite(IN3, 0);
  }else{ //Trường hợp còn lại là speedR = 0 đẩy lên cao để phanh bằng motor, hạn chế trôi
    analogWrite(IN3, 255);
    analogWrite(IN4, 255);
  }
}





//Hàm PID đơn giản để điều khiển motor
int simplePID(float Setpoint, float Input, float kp, float ki, float kd) {
  /*How long since we last calculated*/
  unsigned long now = micros();
  double timeChange = (double)(now - lastTime) * 0.000001;

  /*Compute all the working error variables*/
  double error = Setpoint - Input;
  errSum += (error * timeChange);
  double dErr = (error - lastErr) / timeChange;

  /*Compute PID Output*/
  float Output = kp * error + ki * errSum + kd * dErr;

  lastErr = error;
  lastTime = now;
  return (int)Output;
}

///////////////////////////////////////////
enum huong{ BAC = 0,DONG = 1, NAM = 2, TAY = 3};
huong huonght = BAC; 

const int dr[4] = {-1, 0, 1, 0};
const int dc[4] = { 0, 1, 0, -1};

///////////////////////////////////////////////
#define ROWS 6
#define COLS 5 

int row =3;
int col =2;
const int goalR = 1;
const int goalC = 3;
bool reached = false; 

// taọ mảng 2D đầu tiên để lưu giá trị mỗi lần đi qua của map 
uint8_t maze[ROWS][COLS] = {
  {0 , 0 , 0 , 0 , 0},
  {0 , 0 , 0 , 0 , 0},
  {0 , 0 , 0 , 0 , 0},
  {0 , 0 , 0 , 0 , 0},
  {0 , 0 , 0 , 0 , 0},
  {0 , 0 , 0 , 0 , 0}
};
//Hàm đi thẳng
void dithang()
{

  const float Kp = 1.0;
  const float Ki = 0;
  const float Kd = 0.8;
  const int baseSpeed = 90;
  const int motO = 560;  // so cu là 538 xung
  const int SETPOINT = 12;  // giá trị bám 1 bên tường
  const int NGUONG_DT = 18; // vì khi robot bám 1 bên tường thì có lúc sẽ vượt ra khỏi ngưỡng , do đó nó lọt vào trường hợp cuối nên chạy đều , xử lý vấn đề này tạo thêm cho nó một giới hạn
  const int NGUONG_1 = 16;

  int vitri1_bandau = vitri1;
  int vitri2_bandau = vitri2;

  while (abs(vitri1 - vitri1_bandau) < motO && abs(vitri2 - vitri2_bandau) < motO)
  {
    wifiReconnect();
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
    float kctruoc = doKhoangCach(TRIG0, ECHO0);
    float kctrai = doKhoangCach(TRIG1, ECHO1);
    float kcphai = doKhoangCach(TRIG2, ECHO2);

    bool Trai = kctrai < NGUONG_DT;
    bool Phai = kcphai < NGUONG_DT;
    bool Truoc = kctruoc < NGUONG_1;
    int adjustment = 0;

    if (!Truoc && Trai && Phai)
    {
      adjustment = simplePID(0, kctrai - kcphai, Kp, Ki, Kd);
      quay(baseSpeed + adjustment, baseSpeed - adjustment);
    }
    else if (!Truoc && !Trai && Phai)
    {
      adjustment = simplePID(SETPOINT, kcphai, 4, 0 , 1.3);
      quay(90 - adjustment, 90  + adjustment);
    }
    else if (!Truoc && Trai && !Phai)
    {
      adjustment = simplePID(SETPOINT, kctrai, 4, 0 , 1.3);  // bam traiiiiiiiiiiiiiiii
      quay(90 + adjustment, 90 - adjustment);
    }
    else if (!Truoc && !Trai && !Phai)
    {
      quay(78,78);
    }
    delay(10);
  }

 quay(0, 0);
}

// --- Hàm quay 90 độ ---
void quayphai90() {
  int target = 190;  // 90 độ là 182 xung 
  int start1 = vitri1;
  int start2 = vitri2;

  quay(110, -110);

  while ( abs(vitri1 - start1) < target && abs(vitri2 - start2) < target ) {
    wifiReconnect();
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
    delay(10); // Chờ cho đến khi quay đủ 90°
  }
  huonght = (huong)((huonght + 1)%4); // cập nhật hướng sau khi quay phải 90 độ 
  quay(0,0);
}

void quaytrai90() {
  int target = 189;  // 90 độ là 182 xung 
  int start1 = vitri1;
  int start2 = vitri2;

  quay(-110, 110);

  while ( abs(vitri1 - start1) < target && abs(vitri2 - start2) < target ) {
    wifiReconnect();
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
    delay(10); // Chờ cho đến khi quay đủ 90°
  }
  huonght = (huong)((huonght + 3)%4); // cập nhật hướng sau khi quay trái 90 độ 
  quay(0,0);
}

void quay180(){
  int target = 470;  // 180 độ là 412 xung (theo tính toán)

  int start1 = vitri1;
  int start2 = vitri2;

  quay(78, -80);

  while ( abs(vitri1 - start1) < target && abs(vitri2 - start2) < target ) {
    wifiReconnect();
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
    delay(10); // Chờ cho đến khi quay đủ 180
  }
  huonght = (huong)((huonght + 2)%4); // cập nhật hướng sau khi quay 180 độ 
  quay(0,0);
}

void inmap() {
  // 1) Tiêu đề hàng đầu, có xuống dòng
  Serial.println(F("------ MAZE MAP ------"));
  
  // 2) In từng hàng, mỗi ô cách nhau 1 dấu cách
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      Serial.print(maze[i][j]);
      Serial.print(F(" "));
    }
    Serial.println();       // xuống dòng sau khi in hết 1 hàng
  }

  // 3) In thêm 1 dòng trống để tách biệt với lần in kế tiếp
  Serial.println();
}
void capNhatToaDo() {
  // lưu lại ô cũ trước khi di chuyển 
  int oldRow = row;
  int oldCol = col;
  
  switch (huonght) {
    case BAC:  row--; break;  // Di chuyển lên, giảm hàng
    case DONG: col++; break;  // Di chuyển phải, tăng cột
    case NAM:  row++; break;  // Di chuyển xuống, tăng hàng
    case TAY:  col--; break;  // Di chuyển trái, giảm cột
  }

  row = constrain(row, 0, ROWS-1);
  col = constrain(col, 0, COLS-1);
  
  maze[oldRow][oldCol]++;

  inmap();

}



// --- Hàm setup ---
void setup() {
  Serial.begin(115200);

  // 1) Khởi I2C & LCD
  Wire.begin(SDA_PIN, SCL_PIN);      // SDA=4, SCL=5
  lcd.init();
  lcd.backlight();

  // 2) Hiện “Connecting WiFi…”
  lcd.clear();
  lcd.print("Connecting WiFi");
  lcd.setCursor(0,1);
  // 3) Kết nối WiFi
  wifiReconnect();
  // Wi-Fi OK
  lcd.clear();
  lcd.print("WiFi connected");
  delay(1000);

  // 4) Cấu hình broker + kết nối MQTT
  mqtt.setServer(broker, mqttPort);
  lcd.clear();
  lcd.print("Connecting MQTT");
  lcd.setCursor(0,1);
  if (mqtt.connect(clientID)) {
    lcd.print(" OK");
  } else {
    lcd.print(" FAIL");
  }
  delay(1000);

  // 5) Khởi HC-SR04 / encoder / motor
  pinMode(TRIG0, OUTPUT);
  pinMode(ECHO0,  INPUT);
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1,  INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2,  INPUT);

  pinMode(A1, INPUT);  pinMode(B1, INPUT);
  pinMode(A2, INPUT);  pinMode(B2, INPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(A1), updateVitri1, RISING);
  attachInterrupt(digitalPinToInterrupt(A2), updateVitri2, RISING);

  lastTime = micros();
  lastErr  = 0;
  errSum   = 0;

  delay(5000);
}



// --- Hàm loop ---
void loop() {

  wifiReconnect();
  // 2) Giữ MQTT
  if (!mqtt.connected()) mqttReconnect();
  mqtt.loop();
  // đọc cảm biến trước, trái, phải 
  float kctruoc = doKhoangCach(TRIG0, ECHO0);  // Cảm biến phía trước
  float kctrai = doKhoangCach(TRIG1, ECHO1);   // Cảm biến bên trái
  float kcphai = doKhoangCach(TRIG2, ECHO2);   // Cảm biến bên phải

  // 3) Gói JSON và publish lên Node-RED
  StaticJsonDocument<128> doc;
  doc["front"] = kctruoc;
  doc["left"]  = kctrai;
  doc["right"] = kcphai;
  char buf[128];
  size_t len = serializeJson(doc, buf);
  mqtt.publish("esp32_robot/sensors", buf, len);

    // … sau khi publish MQTT xong …

  // Làm tròn về int
  int d0 = int(kctruoc);
  int d1 = int(kctrai);
  int d2 = int(kcphai);

  char buff[17];

  // Hàng 1: CB0 ở giữa
  lcd.clear();
  // "CB0:xxcm" dài 8 ký tự, đặt ở cột (16–8)/2 = 4
  lcd.setCursor(4, 0);
  sprintf(buff, "cb0:%2dcm", d0);
  lcd.print(buff);

  // Hàng 2: CB1 bên trái, CB2 bên phải
  lcd.setCursor(0, 1);
  sprintf(buff, "cb1:%2dcm ", d1);
  lcd.print(buff);
  // "CB2:xxcm" cũng dài 8, đặt ở cột 16−8=8
  lcd.setCursor(8, 1);
  sprintf(buff, " cb2:%2dcm", d2);
  lcd.print(buff);


  

  int hople = 0;
  int nr[4], nc[4], nd[4], rels[4]; // lưu hàng , cột và hướng của ô mà có thể đi được trong 4 hướng 
  for(int d=0; d<4 ; d++)
  {
     int r2 = row + dr[d];
     int c2 = col + dc[d];
     if(r2 < 0 || r2 >= ROWS || c2 <0 || c2 >= COLS) continue; 

     // xác định hướng cần đi so với hướng hiện tại theo ưu tiên trái - thẳng phải 
     int rel = (d - huonght +4)%4 ;

     // lấy hướng xong thì cần qua một ải kiểm tra nữa là xét nó với ngưỡng 
     bool wall = false ; 
     if (rel == 0 && kctruoc < NGUONG ) wall = true; 
     else if (rel == 3 && kctrai  < NGUONG_MEM ) wall = true; 
     else if (rel == 1 && kcphai  < NGUONG_MEM ) wall = true; 

     if (wall) continue; 

     // lưu ô nào thông để chạy 
     nr[hople] = r2; 
     nc[hople] = c2;
     nd[hople] = d;
     rels[hople] = rel;
     hople++;
  }
     // chọn hướng theo trémaux 
     const int PRI[4] = {0,3,1,2};
     int chosen = -1; 

     // pass 1: tìm ô chưa visit (maze==0)
  for (int p = 0; p < 4 && chosen < 0; p++) {
    for (int i = 0; i < hople; i++) {
      if (rels[i] == PRI[p] && maze[nr[i]][nc[i]] == 0) {
        chosen = i;
        break;
      }
    }
  }
  // pass 2: nếu không có, tìm ô đã visit đúng 1 lần
  if (chosen < 0) {
    for (int p = 0; p < 4 && chosen < 0; p++) {
      for (int i = 0; i < hople; i++) {
        if (rels[i] == PRI[p] && maze[nr[i]][nc[i]] == 1) {
          chosen = i;
          break;
        }
      }
    }
  }
  // dead-end: backtrack
  if (chosen < 0) {
    quay180();
    dithang();
    capNhatToaDo();
    return;
  }

  // 4. Thực thi quay → di chuyển → cập nhật
  huong nextDir = (huong) nd[chosen];
  if      (nextDir == huonght) {
    // thẳng
  }
  else if (nextDir == huong((huonght + 1) % 4)) {
    quayphai90();
  }
  else if (nextDir == huong((huonght + 3) % 4)) {
    quaytrai90();
  }
  else {
    quay180();
  }

  dithang();
  capNhatToaDo();

  
   if (!reached && row == goalR && col == goalC) {
    reached = true;
    quay(0, 0);
    Serial.println(F(">>> Reached goal at (0,3)!"));
    while (true) delay(100);
  }
 delay(60);
}
