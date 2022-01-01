# MQTT bridge for TWELITE PAL/CUE/ARIA

TWELITE PAL (親機) の出力を MQTT に変換します。[環境センサーPAL](https://mono-wireless.com/jp/products/twelite-pal/sense/amb-pal.html)、[動作センサーPAL](https://mono-wireless.com/jp/products/twelite-pal/sense/motion-pal.html)、[開閉センサーPAL](https://mono-wireless.com/jp/products/twelite-pal/sense/openclose-pal.html)、[TWELITE CUE](https://mono-wireless.com/jp/products/twelite-cue/index.html)、[TWELITE ARIA](https://mono-wireless.com/jp/products/twelite-aria/index.html)に対応しています。サーバーに配置しなくてもセンサ出力を確認できる HTML のビューアーもついています。

## 例

|トピック|メッセージ|
|---|---|
|Temperature|{"Id":"AMB_02","Value":22.85}|
|Humidity|{"Id":"AMB_02","Value":77.38}|
|Illuminance|{"Id":"AMB_02","Value":100}|
|Voltage|{"Id":"AMB_02","Value":2.195}|
|Lqi|{"Id":"AMB_02","Value":-59.7}|
|Magnet|{"Id":"MAG_01","Value":2}|
|Acceleration|{"Id":"MOT_01_X","Value":-10.508}|
|Acceleration|{"Id":"MOT_01_Y","Value":-0.142}|
|Acceleration|{"Id":"MOT_01_Z","Value":0.255}|

- 参考: https://twitter.com/ksasao/status/1271816152935657472

![外観](bridge.jpg)
![ブラウザの表示イメージ](viewer.png)

## 準備するもの

TWELITE PAL以外に下記のものが必要です。

- 親機・中継機アプリ（App_Wings）[親機・中継機アプリ（App_Wings）を書き込み](https://mono-wireless.com/jp/products/TWE-APPS/App_pal/parent.html)済みの TWELITE DIP
- [M5 ATOM Lite](https://www.switch-science.com/catalog/6262/) (M5 ATOM Matrixも可)
- [ATOM HUB Proto Kit](https://m5stack.com/products/atom-hub-proto-kit) (ケースです。他のもので代用可能。)
- [Mosquitto](https://www.google.com/search?q=Mosquitto+mqtt&ie=&oe=) などの MQTT Broker (MQTT over WebSocket を有効化してください)

## TWELITE親機の設定

親機側の TWELITE DIP の[アプリケーションID](https://mono-wireless.com/jp/products/TWE-APPS/interactive.html) を TWELITE PAL と同じ、```0x67726305```に揃えます。また、周波数チャネルを15,18と指定します。これで、TWELITE PAL, TWELITE CUE, TWELITE ARIA のデータを一つの親機で受け取れるようになります。

### 設定例

```txt
[CONFIG MENU/App_Wings:0/v1-01-6/SID=81000134]
a:*(0x67726305) Application ID [HEX:32bit]
c:*(15,18     ) Channels Set
x: (      0x03) RF Power/Retry [HEX:8bit]
b: (38400,8N1 ) UART Baud [9600-230400]
o: (0x00000000) Option Bits [HEX:32bit]
k: (0xA5A5A5A5) Encryption Key [HEX:32bit]
m: (         0) Mode (Parent or Router)
A: (0x00000000) Access point address [HEX:32bit]

- [S]:Save [R]:Reload settings
 [ESC]:Back [!]:Reset System [M]:Extr Menu
```

## TWELITE CUE/ARIAの設定

CUE/ARIAを利用する場合は、同様に、アプリケーションIDを```0x67726305```に変更します。

```txt
--- CONFIG/App_ARIA V1-00-0/SID=0x8100XXXX/LID=0x0a ---
 a: set Application ID (0x67726305)
 i: set Device ID (10=0x0a)
 c: set Channels (18)
 x: set Tx Power (13)
 b: set UART baud (38400)
 B: set UART option (8N1)
 k: set Enc Key (0xA5A5A5A5)
 o: set Option Bits (0x00000001)
 t: set Transmission Interval (60)
 p: set Senser Parameter (0x00000000)
---
 S: save Configuration
 R: reset to Defaults
```

## 組み立て

1. M5 ATOM と TWELITE DIP を下記のように接続します。他のピンはそのままでかまいません。
|M5 ATOM|TWELITE DIP|
|---|---|
|3V3|VCC|
|GND|GND|
|G21|6|

2. Arduino IDE で TWELITE_MQTT.ino を開き、SSID, PASSWORD, MQTT ブローカーのIPアドレスを書き換えて、M5 ATOM に書き込みます。

3. ```viewer/index.html``` をエディタで開き、下記を環境に合わせて変更します。なお、MQTTブローカーは MQTT over WebSocket が使えるように設定しておく必要があります。

```txt
let mqttBrokerAddress = "192.168.3.40";
let mqttOverWebSocketPort = 8001;
```

4. ```viewer/index.html``` をブラウザで開くと TWELITE PAL からのデータを確認できます。ファイルはサーバーに配置する必要はありません。
