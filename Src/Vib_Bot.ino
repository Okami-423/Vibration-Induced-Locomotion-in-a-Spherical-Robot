#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Wi-Fi credentials
const char* ssid = "ssid";
const char* password = "password";

// Motor control pins
const int IN1 = 2;
const int IN2 = 3;
const int IN3 = 4;
const int IN4 = 5;
const int LED = 6;
const int GAS_SENSOR_PIN = 1;

WebServer server(80);

// Direction flags
bool leftDirForward = true;
bool rightDirForward = true;

unsigned long lastBlink = 0;

// Motor control
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveLeftMotor() {
  digitalWrite(IN1, leftDirForward ? HIGH : LOW);
  digitalWrite(IN2, leftDirForward ? LOW : HIGH);
}

void moveRightMotor() {
  digitalWrite(IN3, rightDirForward ? HIGH : LOW);
  digitalWrite(IN4, rightDirForward ? LOW : HIGH);
}

// Web handlers
void handleMove() {
  String cmd = server.arg("dir");

  if (cmd == "forward") {
    moveLeftMotor();
    moveRightMotor();
  } else if (cmd == "left") {
    moveLeftMotor();
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  } else if (cmd == "right") {
    moveRightMotor();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  } else if (cmd == "stop") {
    stopMotors();
  }

  server.send(200, "text/plain", "OK");
}

void handleToggle() {
  String motor = server.arg("motor");
  if (motor == "left") {
    leftDirForward = !leftDirForward;
  } else if (motor == "right") {
    rightDirForward = !rightDirForward;
  }
  server.send(200, "text/plain", "Toggled");
}

void handleGas() {
  int gasValue = analogRead(GAS_SENSOR_PIN);
  server.send(200, "text/plain", String(gasValue));
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32S3 Bot Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 30px; background: #f0f0f0; }
    h1 { color: #333; }
    .btn {
      padding: 20px 40px;
      margin: 10px;
      font-size: 24px;
      background-color: #2196F3;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }
    .btn:hover { background-color: #0b7dda; }
    .switch { margin: 10px; font-size: 18px; }
  </style>
</head>
<body>
  <h1>ESP32S3 Bot Control</h1>
  <h2>Gas Sensor Value: <span id="gasVal">--</span></h2>

  <button class="btn" onmousedown="send('forward')" onmouseup="send('stop')">W (Forward)</button><br>
  <button class="btn" onmousedown="send('left')" onmouseup="send('stop')">A (Left)</button>
  <button class="btn" onmousedown="send('right')" onmouseup="send('stop')">D (Right)</button><br><br>

  <div class="switch">
    <label><input type="checkbox" onchange="toggleDir('left')"> Toggle Left Motor Direction</label>
  </div>
  <div class="switch">
    <label><input type="checkbox" onchange="toggleDir('right')"> Toggle Right Motor Direction</label>
  </div>

  <script>
    function send(direction) {
      fetch('/move?dir=' + direction);
    }

    function toggleDir(motor) {
      fetch('/toggle?motor=' + motor);
    }

    setInterval(() => {
      fetch('/gas')
        .then(response => response.text())
        .then(data => {
          document.getElementById("gasVal").innerText = data;
        });
    }, 1000);

    // Keyboard controls
    document.addEventListener('keydown', (e) => {
      if (e.repeat) return; // prevent multiple triggers while holding
      if (e.key === 'w') send('forward');
      else if (e.key === 'a') send('left');
      else if (e.key === 'd') send('right');
    });

    document.addEventListener('keyup', (e) => {
      if (['w', 'a', 'd'].includes(e.key)) {
        send('stop');
      }
    });
  </script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  stopMotors();

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("espbot")) {
    Serial.println("mDNS started at http://espbot.local/");
  }

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/toggle", handleToggle);
  server.on("/gas", handleGas);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  // Blink LED every second
  if (millis() - lastBlink >= 1000) {
    digitalWrite(LED, !digitalRead(LED));
    lastBlink = millis();
  }
}
