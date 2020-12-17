** 本リポジトリは作成途中のため、予告なく大幅な修正がされる可能性があります。 **

# AK-030

- AK-030 は ESP32 をメイン CPU とする LTE-M 対応の IoT デバイスです。
- LTE CatM1 モジュールを搭載し、ESP32 の WiFi、Bluetooth に加えて、LTE CatM1 により省電力なセルラー通信が可能となっています。

# 特徴

- Arduino 環境でプログラミングが可能。
- 標準的な ESP32 のハードウェア構成となっており、既存の ESP32 用ライブラリをそのまま流用可能。
- 4 ピンの外部接続端子を有し、I2C や GPIO インターフェースによるさまざまなセンサーデバイスを接続可能。
- USB ドングル型のコンパクトな形状。
- 電源は USB Type-A 端子から供給。

# I/O マップ

| IO No | 説明                   |
| ----- | ---------------------- |
| IO27  | Status LED(青) ※負論理 |
| IO32  | LTE LED(緑) ※負論理    |
| IO22  | I2C SCL                |
| IO21  | I2C SDA                |
| SD0   | ESP32 UART2※ RTS       |
| SD1   | ESP32 UART2※ CTS       |
| IO16  | ESP32 UART2※ RX        |
| IO17  | ESP32 UART2※ TX        |

※ LTE-M モジュールとの通信用シリアルポート。Arduino からは Serial2 として認識。

# プログラミング環境のセットアップ

1. Arduino IDE をインストール
1. Arduino core for the ESP32 をインストール
1. CP210x ドライバをインストール(Windows 10の場合には不要)

# Arduino IDE の設定

1. "Tools"メニューの"Board"から"ESP32 Dev Module"を選択 
1. "Tools"メニューの"Port"を選択
  - Macの場合， "/dev/cu.SLAB_USBtoUART(Arduino Tian)"を選択
  - Windowsの場合 デバイスマネージャにて， "Silicon Lab Dual CP2105 USB to UART Bridge Enhanced COM Port" を選択

# Arduino ライブラリのインストール

1. GitHub リポジトリ( https://github.com/ABIT-crop/AK-030-Arduino )をブラウザで開き、「Clone or Download」ボタンをクリック。さらに「Donwload ZIP」をクリックして、AK-030-master.zip をローカル PC にダウンロードする。
1. Arduino IDE の"Sketch"-"Include Library"から"Add .ZIP Library"を選択して、ダウンロードした AK-030-master.zip を指定。
1. 以後、"File"-"Examples"-"AK-030"からサンプルプログラムを開けるようになる。

# Arduino ライブラリ( AK-030.h )の使い方

## ライブラリの初期化

```
#include "AK-030.h"

AK030 *ak030 = new AK030(); // AK030オブジェクトの生成

setup(){
    Serial.begin(115200); // デバッグ表示用

    ak030->begin("soracom.io");    // 利用するキャリア(SIMカード)のAPNを指定
}

```

## TCP 通信の方法（例)

以下の例ではわかりやすさを優先し、エラー処理などを一部省略しています。より詳細なサンプルは、examples/LTE/http_example を参照ください。

```
void loop(){
  // LTE網に接続する
  if (!ak030->connected()) { // 既に接続しているか確認
    Serial.println("connecting to LTE network...");
    ak030->connect();  // 接続する
    if (!ak030->ok()) {
      Serial.println("cannot connect LTE network");
      return;
    }
    Serial.println("...connected");
  }

  // example.comのIPアドレスを取得
  const char *ipaddr = ak030->dnsLookup("example.com");
  if (ak030->ng()) { // ここは !ak030->ok() としてもよい
    Serial.println("dns lookup failed");
    return;
  }
  Serial.printf("=> %s\n", ipaddr);

  // TCPソケットを開く
  Serial.printf("open tcp: ipaddr=%s, port=%d\n", ipaddr, 80);
  ak030->openTcp(ipaddr, 80);
  if (ak030->ng()) {
    Serial.printf("cannot open tcp\n");
    return;
  } else {
    Serial.println("...opened");
  }

  // HTTPリクエストを送信する
  const char req[] =
      "GET / HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  ak030->send(req,strlen(req)); // TCP送信

  // HTTPレスポンスを受信する
  const char data[3000];
  ak030->waitEvent(30);  // 最大で30秒間、受信イベントが来るの待つ
  if (ak030->eventDataReceived()) {
    int n;
    ak030->receive(data, sizeof(data), &n); // TCP受信
    Serial.printf("received %d bytes\n", n);
    Serial.println("===== recieved data begin =====");
    Serial.println(data);
    Serial.println("===== recieved data end =======");
  }
  ak030->close(); // TCPソケットを閉じる

  delay(1000*30); // 30秒間ウェイトする
}

```

# AK-030 ライブラリの制約

- 現在のバージョンでは、複数の TCP および UDP ソケットを同時に使うことはできません。
