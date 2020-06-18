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
int logicalId=0;
float lqi = 0;
int sensors = 0;

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
      parseData(rx_buffer);
    }else if(rx_buffer_pointer <MAX_RX_BUFFER_SIZE-2){
      rx_buffer_pointer++;
    }
  }
  M5.update();
}

void parseData(char* str){
  // Parse Logical Device ID & LQI
  logicalId = toByte(str+23);
  lqi = (7.0 * toByte(str+9)-1970.0)/20.0;

  int board = toByte(str+27) & 0x1F;
  sensors = toByte(str+29);
  char* p = str + 31;
  switch(board){ // None = 0, Mag = 0x01, Amb = 0x02, Mot = 0x03
    case 0x02:
      parseAsAmb(p);
      break;
    case 0x01:
      parseAsMag(p);
      break;
    default:
      break;
  }
}

// Parse sensor data
// https://mono-wireless.com/jp/products/TWE-APPS/App_pal/parent.html
void parseAsAmb(char* p){
  uint32_t voltage = 0;
  float temp = 0;
  float humi = 0;
  uint32_t lux = 0;
  for(int i=0; i<sensors;i++){
    int sta = toByte(p);
    int source = toByte(p+2);
    int ext = toByte(p+4);

    switch(source){
      case 0x30: // Voltage(mV)
        if (ext == 8)
        {
            voltage = toUInt16(p + 8);
        }
        p += 12;
        break;
      case 0x01: // Temperature
        temp = toInt16(p + 8) / 100.0;
        p += 12;
        break;
      case 0x02: // Humidity
        humi = toUInt16(p + 8) / 100.0;
        p += 12;
        break;
      case 0x03: // Lux
        lux = toUInt32(p + 8);
        p += 16;
        break;
      default:
        break;
    }
  }

  // Send to MQTT
  ac.reconnect();

  char idstr[4] = "00";
  snprintf(idstr, sizeof(idstr), "%02d", logicalId);
  String base = "{\"Id\":\"AMB_" + String(idstr) + "\",\"Value\":";  
  String s = "";
  
  ac.publish("Temperature", base + String(temp,2) + "}");
  ac.publish("Humidity", base + String(humi,2) + "}");
  ac.publish("Illuminance", base + String(lux) + "}");
  ac.publish("Voltage", base + String(voltage/1000.0,3) + "}");
  ac.publish("Lqi", base + String(lqi,1) + "}");
}

void parseAsMag(char* p){
  uint32_t voltage = 0;
  int mag = 0;
  for(int i=0; i<sensors;i++){
    int sta = toByte(p);
    int source = toByte(p+2);
    int ext = toByte(p+4);

    switch(source){
      case 0x30: // Voltage(mV)
        if (ext == 8)
        {
            voltage = toUInt16(p + 8);
        }
        p += 12;
        break;
      case 0x00: // Hall IC
        mag = toByte(p + 8);
        p += 10;
        break;
      default:
        break;
    }
  }

  // Send to MQTT
  ac.reconnect();

  char idstr[4] = "000";
  snprintf(idstr, sizeof(idstr), "%02d", logicalId);
  String base = "{\"Id\":\"MAG_" + String(idstr) + "\",\"Value\":";  
  String s = "";
  
  ac.publish("Magnet", base + String(mag & 3) + "}"); // 0: none / 1: N / 2: S / [bit7] 0: changed / 1: periodic transfer
  ac.publish("Voltage", base + String(voltage/1000.0,3) + "}");
  ac.publish("Lqi", base + String(lqi,1) + "}");
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
