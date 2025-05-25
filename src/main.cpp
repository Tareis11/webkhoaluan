#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Arduino.h>
// Thư viện cần thiết cho ESP8266
const char *ssid = "Locker QR";

// DNS và WebServer
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wi-Fi Config</title>
  <style>
    body {
      font-family: 'Roboto', Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: #f4f6f8;
      color: #333;
    }

    h1, h2 {
      text-align: center;
      color: #2c3e50;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 20px;
      background: #fff;
      box-shadow: 0 2px 8px rgba(0,0,0,0.05);
      border-radius: 8px;
      overflow: hidden;
    }

    th, td {
      padding: 14px 16px;
      text-align: left;
      font-size: 16px;
    }

    th {
      background-color: #e0e0e0;
    }

    tr {
      transition: background-color 0.2s ease;
    }

    tr:hover {
      background-color: #d0f0ff;
      cursor: pointer;
    }

    #wifiForm {
      display: none;
      margin-top: 30px;
      background: #fff;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      max-width: 500px;
      margin-left: auto;
      margin-right: auto;
    }

    form {
      display: flex;
      flex-direction: column;
    }

    input[type='text'],
    input[type='password'] {
      padding: 12px;
      margin-top: 8px;
      margin-bottom: 16px;
      border: 1px solid #ccc;
      border-radius: 6px;
      font-size: 16px;
    }

    input[type='submit'] {
      padding: 14px;
      font-size: 16px;
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      transition: background-color 0.2s ease;
    }

    input[type='submit']:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <h1>Danh sách Wi-Fi gần bạn</h1>
  <div style="text-align:center; margin-bottom: 20px;">
  <form method="GET" action="/">
    <input type="submit" value="🔄 Quét lại Wi-Fi" style="
      padding: 10px 20px;
      font-size: 16px;
      background-color: #2196F3;
      color: white;
      border: none;
      border-radius: 6px;
      cursor: pointer;
    ">
  </form>
</div>

  <table>
    <tr><th>Tên Wi-Fi</th><th>Tín hiệu</th><th>Bảo mật</th></tr>
)rawliteral";

  int n = WiFi.scanNetworks();

  if (n == 0) {
  page += "<tr><td colspan='3' style='text-align:center;'>Không tìm thấy mạng Wi-Fi nào</td></tr>";
} else {
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    String cleanSSID = ssid;
    cleanSSID.replace("'", "\\'");

    String rssi = String(WiFi.RSSI(i)) + " dBm";
    String secure = WiFi.encryptionType(i) == ENC_TYPE_NONE ? "Mở" : "Có mã hóa";

    page += "<tr onclick=\"selectSSID('" + cleanSSID + "')\">";
    page += "<td>" + ssid + "</td>";
    page += "<td>" + rssi + "</td>";
    page += "<td>" + secure + "</td>";
    page += "</tr>";
  }
}


  page += R"rawliteral(
  </table>

  <div id="wifiForm">
    <h2>Kết nối đến Wi-Fi</h2>
    <form action='/connect' method='POST'>
      <label for='ssid'>Tên Wi-Fi (SSID):</label>
      <input type='text' id='ssid' name='ssid' readonly>

      <label for='password'>Mật khẩu:</label>
      <input type='password' name='password' id='password'>

      <input type='submit' value='Kết nối'>
    </form>
  </div>

  <script>
    function selectSSID(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('wifiForm').style.display = 'block';
      document.getElementById('wifiForm').scrollIntoView({ behavior: 'smooth' });
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}



void handleConnect() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    Serial.println("Đang kết nối đến Wi-Fi:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);

    WiFi.softAPdisconnect(true);   // Tắt chế độ AP
    WiFi.mode(WIFI_STA);           // Chuyển sang chế độ client
    WiFi.begin(ssid.c_str(), password.c_str());

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
      delay(500);
      Serial.print(".");
      retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      dnsServer.stop();  // Dừng DNS giả khi đã kết nối thật
      server.send(200, "text/html", "<h1>Kết nối thành công!</h1><p>IP: " + WiFi.localIP().toString() + "</p>");
    } else {
      server.send(200, "text/html", "<h1>Kết nối thất bại!</h1><p>Thử lại.</p>");
      delay(3000);
      ESP.restart(); // Quay lại từ đầu
    }
  } else {
    server.send(400, "text/html", "Thiếu thông tin");
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);


  delay(100); // Cho ổn định

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Bắt mọi domain trỏ về ESP
  server.onNotFound(handleRoot); // Bất kỳ URL nào cũng trả về trang root
  server.on("/", handleRoot);
  server.on("/connect", HTTP_POST, handleConnect); // Xử lý form gửi từ trang chọn Wi-Fi

  server.begin();
  Serial.println("Captive portal đã sẵn sàng.");
  Serial.print("Truy cập: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  
  dnsServer.processNextRequest();
  server.handleClient();
}
