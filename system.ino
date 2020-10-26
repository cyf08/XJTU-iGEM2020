/*
更新日志

Version 1 初版 2020-10-08
        
Version 1.1 2020-10-25
    舵机异常，添加delay函数

Version 1.2 2020-10-16
    添加气候查询功能    
    失败， MSG LIMIT

*/
#define BLINKER_WIFI  //定义连接模式 wifi

#include <Blinker.h>
#include <Arduino.h>
//#include <U8g2lib.h>
/*
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
*/

char auth[] = "1796edb829de";   //设备密钥
char ssid[] = "123";      //wifi名
char pswd[] = "12345679";   //wifi密码

#include <Servo.h>
//U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 12, /* cs=*/ 13, /* dc=*/ 15);


//注册组件
BlinkerNumber Num_temp("num-temp");        //温度
BlinkerNumber Num_humi("num-humi");        //湿度
BlinkerNumber Num_smoi("num-smoi");        //土壤湿度
BlinkerButton Button_LIT("btn-lit");  //灯开关
BlinkerButton Button_WAR("btn-war");  //制热开关
BlinkerButton Button_RV("renovate");  //刷新按键
BlinkerButton Button_DTL("delete");   //删除云端历史数据（会有延迟）按键
BlinkerButton Button_MO("btn-mo");    //手动模式开关
BlinkerButton Button_RST("btn-rst");  //复位按键
BlinkerButton Button_COL("btn-col");           //制冷按键
BlinkerButton Button_WAT("btn-wat");           //喷雾按键
BlinkerNumber HUMI("humi");           //温度数据记录
BlinkerNumber TEMP("temp");           //湿度数据记录
BlinkerSlider Slider_SETTEMP("sli-settemp");    //恒温设置滑块
BlinkerText Text_WTP("tex-temp");      //获取目标位置实时温度
BlinkerText Text_WHM("tex-humi");      //获取目标位置实时湿度
BlinkerText Text_PLA("tex-pla");       //目标位置



#include <DHT.h>   //加载库

#define DHTPIN D4
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define LITPIN D1
#define WARPIN D2
#define COLPIN D3
#define WATPIN 14
#define AOPIN A0

#define sensorPin A0

DHT dht(DHTPIN, DHTTYPE);
Servo myservo;

uint32_t read_time = 0;
float humi_read = 0;
float temp_read = 0;
float temp_set = 0;
int sensorValue = 0;
int pos = 0;
int setAngle = 0;
int ep = 1;
String weatherTemp;
String weatherHumi;
String Autostate;
String Place = "Xian";
bool queryState = true;
//bool servoRun = false;

/*
void Display(u8g2_uint_t x, u8g2_uint_t y, const char *s)
{
    u8g2.clearBuffer();          // 清除内存
    u8g2.setFont(u8g2_font_helvR10_te); // 设置字体
    u8g2.drawStr(x, y, s); // 初始化输出
    u8g2.sendBuffer();          // 显示函数
    Blinker.delay(5000);  
}
   显示模块，GPIO口不够用？
*/
void autoVibrate(int n)
{
    if(Autostate == "off")
    {
      Blinker.vibrate(n);
    }
    else {}
    
}

void weatherData(const String & data)//天气获取
{
    BLINKER_LOG("weather: ", data);
    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, data);
    if(error)
    {
        weatherTemp == "404";
        weatherHumi == "404";
        //显示获取失败
        return;
    }
    const char* cloud = doc["cloud"]; // "0"
    const char* cond_code = doc["cond_code"]; // "101"
    const char* cond_txt = doc["cond_txt"]; // ""
    const char* fl = doc["fl"]; // "31"
    const char* hum = doc["hum"]; // "81"
    const char* pcpn = doc["pcpn"]; // "0.0"
    const char* pres = doc["pres"]; // "997"
    const char* tmp = doc["tmp"]; // "28"
    const char* vis = doc["vis"]; // "16"
    const char* wind_deg = doc["wind_deg"]; // "159"
    const char* wind_dir = doc["wind_dir"]; // 风向
    const char* wind_sc = doc["wind_sc"]; // "2"
    const char* wind_spd = doc["wind_spd"]; // "9"  
    weatherTemp = tmp;//atoi是将字符型转化为数字 详见菜鸟教程：https://www.runoob.com/cprogramming/c-function-atoi.html
    weatherHumi = hum;
    Place = data;
}

void Button_RV_callback(const String & state)
{
    Num_temp.print(temp_read);       //温度
    Num_humi.print(humi_read);       //湿度
    Num_smoi.print(sensorValue);       //土壤湿度
}                                    //刷新按钮


void Button_DTL_callback(const String & state)
{
  Blinker.dataDelete();
}                               //删除按钮


void Button_LIT_callback(const String & state) 
{
    BLINKER_LOG("get light button state: ", state);
    if (state == "on") 
    {
        digitalWrite(LITPIN, HIGH);
        Button_LIT.color("#FF0000");   //1#按钮按下时，app按键颜色状态显示是红色
        // 反馈开关状态
        Button_LIT.print("on");
        BLINKER_LOG ("生长灯已打开");  //串口打印
        //Display(0, 30, "Light is on");
    } 
    else if(state == "off")
    {
        digitalWrite(LITPIN, LOW);
        //Button_LIT.color("#FFFFFF");  2#按钮没有按下时，app按键颜色状态显示是黑色
               // 反馈开关状态
        Button_LIT.print("off");
        BLINKER_LOG ("生长灯已关闭");
        //Display(0, 30, "Light is off");
    }
    Blinker.vibrate();
}


void Button_WAR_callback(const String & state) {
    BLINKER_LOG("get warm button state: ", state);
    if (state=="on") 
    {
        digitalWrite(WARPIN, HIGH);
        Button_WAR.color("#FF0000");   //1#按钮按下时，app按键颜色状态显示是红色
        // 反馈开关状态
        Button_WAR.print("on");
        BLINKER_LOG ("开始制热");  //串口打印
        //Display(0, 30, "Heating, loading");
    } 
    else if(state=="off")
    {
        digitalWrite(WARPIN, LOW);
        //Button_WAR.color("#FFFFFF");  2#按钮没有按下时，app按键颜色状态显示是黑色
               // 反馈开关状态
        Button_WAR.print("off");
        BLINKER_LOG ("停止制热");
        //Display(0, 30, "Stop heating");
    }
    autoVibrate(500);
}

void Button_COL_callback(const String & state) {
    BLINKER_LOG("get cool button state: ", state);
    if (state=="on") 
    {
        digitalWrite(COLPIN, HIGH);
        Button_COL.color("#FF0000");   //1#按钮按下时，app按键颜色状态显示是红色
        // 反馈开关状态
        Button_COL.print("on");
        BLINKER_LOG ("开始制冷");  //串口打印
        //Display(0, 30, "Heating, loading");
    } 
    else if(state=="off")
    {
        digitalWrite(COLPIN, LOW);
        //Button_WAR.color("#FFFFFF");  2#按钮没有按下时，app按键颜色状态显示是黑色
               // 反馈开关状态
        Button_COL.print("off");
        BLINKER_LOG ("停止制冷");
        //Display(0, 30, "Stop heating");
    }
    autoVibrate(500);
}

void Button_WAT_callback(const String & state) {
    BLINKER_LOG("get water button state: ", state);
    if (state=="on") 
    {
        for(pos=0; pos <=setAngle; pos ++)
        {
          myservo.write(pos);
          delay(15);
        }
        Button_WAT.color("#FF0000");   //1#按钮按下时，app按键颜色状态显示是红色
        // 反馈开关状态
        Button_WAT.print("on");
        BLINKER_LOG ("开始制热");  //串口打印
        //Display(0, 30, "Heating, loading");
    } 
    else if(state=="off")
    {
        for(pos=setAngle; pos >= 0; pos --)
        {
          myservo.write(pos);
          delay(15);
        }
        //Button_WAR.color("#FFFFFF");  2#按钮没有按下时，app按键颜色状态显示是黑色
               // 反馈开关状态
        Button_WAT.print("off");
        BLINKER_LOG ("停止制热");
        //Display(0, 30, "Stop heating");
    }
    autoVibrate(500);
}

void Slider_SETTEMP_callback(int32_t value)
{
    temp_set = value;
    BLINKER_LOG("get settemp value: ", value);
    Blinker.vibrate();
}


void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);
    if(queryState)
    {
        weatherData(data);
        Place = data;
        queryState = false;
        Blinker.delay(60000);
        queryState = true;
    }
    Blinker.vibrate();
}


void heartbeat()
{
    Num_temp.print(temp_read);
    Num_humi.print(humi_read);
    Num_smoi.print(sensorValue);
    HUMI.print(humi_read);
    TEMP.print(temp_read);
    Autostate;
}              //心跳包

void dataStorage()
{
    Blinker.dataStorage("temp", temp_read);
    Blinker.dataStorage("humi", humi_read);
}                                     //存储历史数据



/*void u8g2Init()
{
    u8g2.begin();
    u8g2.setFlipMode(0);
    u8g2.clearBuffer();
    u8g2.enableUTF8Print();
}
*/

void PinInit(void)
{
    pinMode(LITPIN, OUTPUT);
    pinMode(WARPIN, OUTPUT);
    pinMode(COLPIN, OUTPUT);
    //pinMode(WATPIN, OUTPUT);
    pinMode(AOPIN, INPUT);
    digitalWrite(LITPIN, LOW);
    digitalWrite(WARPIN, LOW);
    //digitalWrite(WATPIN, LOW)
    //初始化IO口
    Button_LIT_callback("off");
    Button_WAR_callback("off");
    Button_COL_callback("off");
    Button_WAT_callback("off");
}

void Button_RST_callback(const String & state)
{
    PinInit();
    Blinker.vibrate(1000);
}


void temPinInit(void)
{
    Button_WAR_callback("off");
    Button_COL_callback("off");
}

void Button_MO_callback(const String & state) 
{
    BLINKER_LOG("get Button_MO state: ", state);
    Autostate = state;
    if (state=="on") 
    {
        //temPinInit();
        Button_MO.color("#FF0000");   //1#按钮按下时，app按键颜色状态显示是红色
        // 反馈开关状态
        Button_MO.print("on");
        Button_MO.text("手动模式");
        BLINKER_LOG ("恒温模式");  //串口打印
        //Display(0, 30, "auto mode");
    } 
    else if(state=="off")
    {
        //Button_LIT.color("#FFFFFF");  2#按钮没有按下时，app按键颜色状态显示是黑色
               // 反馈开关状态
        Button_MO.print("off");
        Button_MO.text("恒温模式");
        BLINKER_LOG ("手动模式");
        //Display(0, 30, "manual mode");
    }
    Blinker.vibrate();
}


void setup()
{
    Serial.begin(115200);        //初始化串口
    BLINKER_DEBUG.stream(Serial);        //开启调试信息
  
    PinInit();

    Button_LIT.attach(Button_LIT_callback);
    Button_WAR.attach(Button_WAR_callback);
    Button_WAT.attach(Button_WAT_callback);
    Button_COL.attach(Button_COL_callback);
    Button_RV.attach(Button_RV_callback);
    Button_DTL.attach(Button_DTL_callback);
    Button_MO.attach(Button_MO_callback);
    Button_RST.attach(Button_RST_callback);
    Slider_SETTEMP.attach(Slider_SETTEMP_callback);
          //注册回调函数
    
    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);
    Blinker.attachHeartbeat(heartbeat);   //注册心跳包
    Blinker.attachDataStorage(dataStorage);//历史记录
   
    //u8g2Init();   //oled显示模块启动
    
    dht.begin();      //dht模块启动
    myservo.attach(WATPIN);
    myservo.write(0);   //默认输出为0度
    delay(100);       //version 1.1 update
    weatherData(Place);

    //Wire.begin(D6, D7); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */

}

void loop()
{
    Blinker.run();

    if (read_time == 0 || (millis() - read_time) >= 2000)
    {
        read_time = millis();

        float h = dht.readHumidity();
        float t = dht.readTemperature();        

        if (isnan(h) || isnan(t)) {
            BLINKER_LOG("Failed to read from DHT sensor!");
            return;
        }

        float hic = dht.computeHeatIndex(t, h, false);

        humi_read = h;
        temp_read = t;

        BLINKER_LOG("Humidity: ", h, " %");
        BLINKER_LOG("Temperature: ", t, " *C");
        BLINKER_LOG("Heat index: ", hic, " *C");
    }
    //获取温度并在端口调试中显示
    sensorValue = analogRead(sensorPin);
    //Display(0, 15, "Welcome!"); //初始化显示
    if(Autostate == "on")
    {
      BLINKER_LOG("Autostate = on");
      if (temp_set - temp_read >= -ep && temp_set - temp_read <= ep)
      {
          temPinInit();
      }
      else if (temp_set - temp_read < -ep)
      {
          Button_WAR_callback("off");
          Button_COL_callback("on");
                   //设定温度低于室温则制冷
      }
      
      else if (temp_set - temp_read > ep)
      {
            Button_COL_callback("off");
            Button_WAR_callback("on");        //设定温度高于室温则制热
      }
      else if (isnan(humi_read) || isnan(temp_read))
      {
        temPinInit();
      }
     }
     else
     { 
         //
     }

    //Wire.beginTransmission(8); /* begin with device address 8 */
    //Wire.write("Hello Arduino");  /* sends hello string */
    //Wire.endTransmission();    /* stop transmitting */
    //Wire.requestFrom(8, 13); /* request & read data of size 13 from slave */
    //while(Wire.available())

    //    char c = Wire.read();
    //    Serial.print(c);
    //}
    //Serial.println();
    Text_PLA.print(Place);
    Text_WTP.print(weatherTemp);
    Text_WHM.print(weatherHumi);
    Blinker.delay(2000);
    
}
