/***
   作者：岳远浩
   邮箱：3117144922@qq.com
   参数设置：
   L298N驱动 左：14，27   右：26，25
   四条扩展线：黑:D4,红：D5，黄：D16，绿：D17
   左手（1）：D4，右手（2）：D5，头（3）:D16





 ***/
//舵机初始
#include <ESP32_Servo.h>
Servo servo_4;
Servo servo_5;
Servo servo_16;


//esp now






//定义电机端口
#define M1_1 14
#define M1_2 27
#define M2_1 25
#define M2_2 26

//超声测距
#define SOUND_SPEED 0.034
const int trigPin = 13;
const int echoPin = 12;
long duration;
float distanceCm;
int autoState = 0;
#include <WiFi.h>
#include <WiFiClient.h>


//巴法云服务器地址默认即可
#define TCP_SERVER_ADDR "bemfa.com"

///****************需要修改的地方*****************///

//服务器端口//TCP创客云端口8344//TCP设备云端口8340
#define TCP_SERVER_PORT "8344"
//WIFI名称，区分大小写，不要写错
#define DEFAULT_STASSID  "Tenda_9F4558"
//WIFI密码
#define DEFAULT_STAPSW   "18263599188"
//用户私钥，可在控制台获取,修改为自己的UID
#define UID  "53828b3b873986c70e1c7ce026484666"
//主题名字，可在控制台新建
#define TOPIC  "walle"
//单片机LED引脚值
const int LED_Pin = 2;

///*********************************************///

//led 控制函数
void turnOnLed();
void turnOffLed();


//最大字节数
#define MAX_PACKETSIZE 512
//设置心跳值300ms
#define KEEPALIVEATIME 30*10




//tcp客户端相关初始化，默认即可
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;//心跳
unsigned long preTCPStartTick = 0;//连接
bool preTCPConnected = false;



//相关函数初始化
//连接WIFI
void doWiFiTick();
void startSTA();

//TCP初始化连接
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);





/*
   发送数据到TCP服务器
*/
void sendtoTCPServer(String p) {

  if (!TCPclient.connected())
  {
    Serial.println("Client is not readly");

    return;
  }
  TCPclient.print(p);
  Serial.println("[Send to TCPServer]:String");
  Serial.println(p);
}


/*
   初始化和服务器建立连接
*/
void startTCPClient() {
  if (TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT))) {
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n", TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT));
    char tcpTemp[128];
    sprintf(tcpTemp, "cmd=1&uid=%s&topic=%s\r\n", UID, TOPIC);

    sendtoTCPServer(tcpTemp);
    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  }
  else {
    Serial.print("Failed connected to server:");
    Serial.println(TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}


/*
   检查数据，发送心跳
*/
//************************************************************************************//
void doTCPClientTick() {
  //检查是否断开，断开后重连
  if (WiFi.status() != WL_CONNECTED) return;

  if (!TCPclient.connected()) {//断开重连

    if (preTCPConnected == true) {

      preTCPConnected = false;
      preTCPStartTick = millis();
      Serial.println();
      Serial.println("TCP Client disconnected.");
      TCPclient.stop();
    }
    else if (millis() - preTCPStartTick > 1 * 1000) //重新连接
      startTCPClient();
  }
  else
  {
    if (TCPclient.available()) {//收数据
      char c = TCPclient.read();
      TcpClient_Buff += c;
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();

      if (TcpClient_BuffIndex >= MAX_PACKETSIZE - 1) {
        TcpClient_BuffIndex = MAX_PACKETSIZE - 2;
        TcpClient_preTick = TcpClient_preTick - 200;
      }
      preHeartTick = millis();
    }
    if (millis() - preHeartTick >= KEEPALIVEATIME) { //保持心跳
      preHeartTick = millis();
      Serial.println("--Keep alive:");
      sendtoTCPServer("cmd=0&msg=keep\r\n");
    }
  }
  if ((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick >= 200))
  { //data ready
    TCPclient.flush();
    Serial.println("Buff");
    Serial.println(TcpClient_Buff);
    if ((TcpClient_Buff.indexOf("&msg=on") > 0)) {
      turnOnLed();
    } else if ((TcpClient_Buff.indexOf("&msg=off") > 0)) {
      turnOffLed();
    } else if ((TcpClient_Buff.indexOf("&msg=go") > 0)) {
      Go();
    } else if ((TcpClient_Buff.indexOf("&msg=back") > 0)) {
      Back();
    } else if ((TcpClient_Buff.indexOf("&msg=left") > 0)) {
      Left();
    } else if ((TcpClient_Buff.indexOf("&msg=right") > 0)) {
      Right();
    } else if ((TcpClient_Buff.indexOf("&msg=stop") > 0)) {
      Stop();
    } else if ((TcpClient_Buff.indexOf("&msg=up1") > 0)) {
      up1();
    } else if ((TcpClient_Buff.indexOf("&msg=down1") > 0)) {
      down1();
    } else if ((TcpClient_Buff.indexOf("&msg=up2") > 0)) {
      up2();
    } else if ((TcpClient_Buff.indexOf("&msg=down2") > 0)) {
      down2();
    } else if ((TcpClient_Buff.indexOf("&msg=up3") > 0)) {
      up3();
    } else if ((TcpClient_Buff.indexOf("&msg=down3") > 0)) {
      down3();
    } else if ((TcpClient_Buff.indexOf("&msg=stopArm") > 0)){
      stopArm();
    } else if ((TcpClient_Buff.indexOf("&msg=auto") > 0)) {
      autoState = 1;
    } else if ((TcpClient_Buff.indexOf("&msg=stopauto") > 0)) {
      autoState = 0;
    }
    TcpClient_Buff = "";
    TcpClient_BuffIndex = 0;
  }
}
//***************************************************************************************//
void startSTA() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_STASSID, DEFAULT_STAPSW);

}



/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
//************************************************************//
void doWiFiTick() {
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
    Serial.printf("Heap size:%d\r\n", ESP.getFreeHeap());
  }

  //未连接1s重连
  if ( WiFi.status() != WL_CONNECTED ) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  }
  //连接成功建立
  else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}
//打开指示灯
void turnOnLed() {
  Serial.println("Turn ON");
  digitalWrite(LED_Pin, LOW);
}
//关闭指示灯
void turnOffLed() {
  Serial.println("Turn OFF");
  digitalWrite(LED_Pin, HIGH);
}
void Auto() {
  if (Echo() > 30) {
    Go();
  }
  else
  {
    Left();
    delay(1000);
  }
}
//舵机

void up1() {
  servo_4.write(int(servo_4.read() + 10));
  delay(100);
}
void down1() {
  servo_4.write(int(servo_4.read() - 10));
  delay(100);
}
void up2() {
  servo_5.write(int(servo_5.read() + 10));
  delay(100);
}
void down2() {
  servo_5.write(int(servo_5.read() - 10));
  delay(100);
}
void up3() {
  servo_16.write(int(servo_16.read() + 10));
  delay(100);
}
void down3() {
  servo_16.write(int(servo_16.read() - 10));
  delay(100);
}
void stopArm(){
  servo_4.write(int(servo_4.read()));
  servo_5.write(int(servo_5.read()));
  servo_16.write(int(servo_16.read()));
  }

float Echo() {
  // 清空发射口
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // 发射超声波10毫秒
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  //读取接收口以毫秒返回声波时间
  duration = pulseIn(echoPin, HIGH);

  //计算距离，时间*声速/2
  distanceCm = duration * SOUND_SPEED / 2;

  //打印距离到串口
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  delay(100);
  return (distanceCm);
}


void Left() {
  digitalWrite(M1_1, HIGH);
  digitalWrite(M1_2, LOW);
  digitalWrite(M2_1, HIGH);
  digitalWrite(M2_2, LOW);
}
void Back() {
  digitalWrite(M1_2, HIGH);
  digitalWrite(M1_1, LOW);
  digitalWrite(M2_2, HIGH);
  digitalWrite(M2_1, LOW);
}
void Go() {
  digitalWrite(M1_2, HIGH);
  digitalWrite(M1_1, LOW);
  digitalWrite(M2_1, HIGH);
  digitalWrite(M2_2, LOW);
}
void Right() {
  digitalWrite(M1_1, HIGH);
  digitalWrite(M1_2, LOW);
  digitalWrite(M2_2, HIGH);
  digitalWrite(M2_1, LOW);
}
void Stop() {
  digitalWrite(M1_1, LOW);
  digitalWrite(M1_2, LOW);
  digitalWrite(M2_1, LOW);
  digitalWrite(M2_2, LOW);
}
/************************************************************************setup********************************************************/
void setup() {
  //移动系统
  pinMode(M1_1, OUTPUT);
  pinMode(M1_2, OUTPUT);
  pinMode(M2_1, OUTPUT);
  pinMode(M2_2, OUTPUT);
  //超声测距
  Serial.begin(115200); // 初始波特率
  pinMode(trigPin, OUTPUT); // 设置发射口为OUTPUT
  pinMode(echoPin, INPUT);  // 设置接收口为INPUT
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  //舵机初始
  servo_4.attach(4, 500, 2500);
  servo_5.attach(5, 500, 2500);
  servo_16.attach(16, 500, 2500);

}
/**************************************************************************setup*******************************************************/
/**************************************************************************loop********************************************************/
void loop() {
  doWiFiTick();
  doTCPClientTick();
  if (autoState == 1) {
    Auto();
  }
}
/**************************************************************************loop********************************************************/
