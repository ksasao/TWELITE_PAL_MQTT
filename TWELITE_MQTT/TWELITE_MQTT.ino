// TWELITE PAL - WiFi(MQTT) bridge
// 2020/6/14 @ksasao
//
// pin assignment
// ---------------------
// M5Atom  TWE-Lite Dip
// ---------------------
// 3V3    - VCC
// GND    - GND
// G21    - 6
// ---------------------
#include "M5Atom.h"
#include "AtomClient.h"

static char ssid[64]="your-ssid";  // SSID
static char pass[64]="your-wifi-password";  // password
static char mqtt[64]="your-MQTT-broker-ip-address"; // MQTT Address

AtomClient ac;
const int MAX_RX_BUFFER_SIZE = 1024;
char rx_buffer[MAX_RX_BUFFER_SIZE];
int rx_buffer_pointer = 0;

static void initialize_wifi()
{
  Serial.print("SSID:");
  Serial.println(ssid);
  Serial.print("MQTT Server:");
  Serial.println(mqtt);

  ac.setup("TWELite", ssid, pass, mqtt);
  Serial.println();
  Serial.print("MQTT Client ID: ");
  Serial.println(ac.getClientId());
}

void setup() {
  M5.begin(true,false,true);
  initialize_wifi();
  Serial1.begin(115200, SERIAL_8N1, 21, 25);
  rx_buffer_pointer = 0;
}

void loop() {
  while(Serial1.available()>0) {
    char d = Serial1.read();
    rx_buffer[rx_buffer_pointer] = d;
    if(d == 0x0a){
      rx_buffer[rx_buffer_pointer+1]='\0';
      rx_buffer_pointer = 0;
      Serial.print(rx_buffer);
      if(isValid(rx_buffer)){
        parseData(rx_buffer);
      }else{
        Serial.println("* parse error *");
      }
    }else if(rx_buffer_pointer <MAX_RX_BUFFER_SIZE-2){
      rx_buffer_pointer++;
    }
  }
  M5.update();
}

bool isValid(char* str){
  if(str[0] != ':'){
    return false;
  }
  int len = strlen(str);
  int sum = 0;
  for(int i=1; i<len; i+=2){
    sum += toByte(str+i);
  }
  return !(sum & 0xFF);
}

void parseData(char* str){
  const String boardType[4] = {"NONE","MAG","AMB","MOT"};
  
  // Parse Logical Device ID & LQI
  float lqi = (7.0 * toByte(str+9)-1970.0)/20.0;
  int logicalId = toByte(str+23);
  int board = toByte(str+27) & 0x1F;

  char idstr[4] = "00";
  snprintf(idstr, sizeof(idstr), "%02d", logicalId);
  String base = "{\"Id\":\"" + boardType[board] + "_" + String(idstr) + "\",\"Value\":";  

  // Send to MQTT
  ac.reconnect();
  ac.publish("Lqi", base + String(lqi,1) + "}");

  int sensors = toByte(str+29);
  char* p = str + 31;

  // Parse sensor data
  // https://mono-wireless.com/jp/products/TWE-APPS/App_pal/parent.html
  for(int i=0; i<sensors;i++){
    int sta = toByte(p);
    int source = toByte(p+2);
    int ext = toByte(p+4);

    switch(source){
      case 0x30: // Voltage(mV)
        if (ext == 8)
        {
            float voltage = toUInt16(p + 8) / 1000.0;
            ac.publish("Voltage", base + String(voltage,3) + "}");
        }
        p += 12;
        break;
      case 0x00: // Hall IC
        {
          int mag = toByte(p + 8);
          ac.publish("Magnet", base + String(mag & 3) + "}"); // 0: none / 1: N / 2: S / [bit7] 0: changed / 1: periodic transfer
          p += 10;
        }
        break;
      case 0x01: // Temperature
        {
          float temp = toInt16(p + 8) / 100.0;
          ac.publish("Temperature", base + String(temp,2) + "}");
          p += 12;
        }
        break;
      case 0x02: // Humidity
        {
          float humi = toUInt16(p + 8) / 100.0;
          ac.publish("Humidity", base + String(humi,2) + "}");
          p += 12;
        }
        break;
      case 0x03: // Lux
        {
          uint32_t lux = toUInt32(p + 8);
          ac.publish("Illuminance", base + String(lux) + "}");
          p += 16;
        }
        break;
      case 0x04: // Acc
        {
          float x = 0;
          float y = 0;
          float z = 0;
          p = p + 8;
          for(int j=0; j<16; j++){
            x += (int32_t)toInt16(p);
            y += (int32_t)toInt16(p+4);
            z += (int32_t)toInt16(p+8);
            p += 20;
          }
          x *= (9.80665/1000.0/16.0);
          y *= (9.80665/1000.0/16.0);
          z *= (9.80665/1000.0/16.0);
          String xs= "{\"Id\":\"" + boardType[board] + "_" + String(idstr) + "_X\",\"Value\":";  
          ac.publish("Acceleration", xs + String(x,3) + "}");
          String ys= "{\"Id\":\"" + boardType[board] + "_" + String(idstr) + "_Y\",\"Value\":";  
          ac.publish("Acceleration", ys + String(y,3) + "}");
          String zs= "{\"Id\":\"" + boardType[board] + "_" + String(idstr) + "_Z\",\"Value\":";  
          ac.publish("Acceleration", zs + String(z,3) + "}");
          sensors = 0;
        }
        break;
      default:
        break;
    }
  }
}

int16_t toInt16(char* str){
  return toByte(str)<<8 | toByte(str+2);
}
uint16_t toUInt16(char* str){
  return toByte(str)<<8 | toByte(str+2);
}
uint32_t toUInt32(char* str){
  return toByte(str)<<24 | toByte(str+2)<<16 | toByte(str+4)<<8 | toByte(str+6);
}

byte toByte(char* str){
  byte result = 0;
  for(int i=0; i<2; i++){
    byte c = str[i];
    if(c>='0' && c<='9'){
      result = (result << 4) | (c-'0');
      continue;
    }
    if(c>='A' && c<='F'){
      result = (result << 4) | (c-'A'+10);
      continue;
    }
    if(c>='a' && c<='f'){
      result = (result << 4) | (c-'a'+10);
      continue;
    }
  }
  return result;
}
