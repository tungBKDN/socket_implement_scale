#include <HX711_ADC.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

const int HX711_dout = 12; //mcu > HX711 dout pin
const int HX711_sck = 13; //mcu > HX711 sck pin

const char *ssid = "LUN 2";
const char *password = "thitkhotrungcut";

const char index_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html>
    <body>
      <h1>WebSocket Test</h1>
      <script type="text/javascript">
        var url = window.location.host;
        var ws = new WebSocket('ws://' + url + '/ws');

        ws.onopen = function() {
          console.log('WebSocket connection established');
        };

        ws.onmessage = function(event) {
          console.log('Data received: ' + event.data);
        };

        ws.onclose = function() {
          console.log('WebSocket connection closed');
        };

        ws.onerror = function(error) {
          console.log('WebSocket error: ' + error);
        };
      </script>
    </body>
  </html>
)=====";

AsyncWebServer server(8001);
AsyncWebSocket ws("/ws");

void sendSensorData(float data) {
  // Get current timestampISO
  char timestampISO[25];
  sprintf(timestampISO, "%ld", millis());
  char message[50];
  sprintf(message, "{\"timestamp\": %s, \"data\": %f}", timestampISO, data);
  ws.textAll(message);
}

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

void setup() {
  Serial.begin(115200); delay(10);
  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html); // trả về file index.html trên giao diện browser khi browser truy cập vào IP của server
  });
  server.begin();

  Serial.print("\n IP Address: "); Serial.println(WiFi.localIP());
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 206; // uncomment this if you want to set the calibration value in the sketch //696
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
}

float prev = 0.00;
int samplingSize = 40;
int currentSample = 0;
void loop()
{
  static boolean newDataReady = 0;
  if (LoadCell.update()) newDataReady = true;
  if (newDataReady)
  {
      currentSample += 1;
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i / 1000);
      if (abs(i - prev) > 10 && currentSample >= samplingSize) {
        sendSensorData(i / 1000);
        prev = i;
        currentSample = 0;
      }
      newDataReady = 0;
      delay(0);
  }
}