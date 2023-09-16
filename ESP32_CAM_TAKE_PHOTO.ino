#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <FS.h>
#include "SD.h"
#include "SPIFFS.h"

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi Config
static const char* ssid = "WIND_9138C1";
static const char* password = "4CR7QR3U";
static IPAddress static_ip(192, 168, 1, 111);
static IPAddress gateway(192, 168, 1, 254);
static IPAddress subnet(255, 255, 255, 0);

// Wifi HotSpot Config
static const char* ssid_h     = "ESP32-Access-Point";
static const char* password_h = "12345678";
static IPAddress static_ip_h(192, 168, 1, 1);
static IPAddress subnet_h(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;

// Photo File Name to save
const char * file_name = "/photo.jpg";

#define FLASS_LED_PIN 4 // Used In Cameras Pins

static fs::FS &fileSystem = SD_MMC; //SPIFFS;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Last Photo</h2>
    <p>It might take more than 5 seconds to capture a photo.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
  </div>
  <div><img src="saved-photo" id="photo" width="70%"></div>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";



void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Setup Led Pin
  //pinMode(FLASS_LED_PIN, OUTPUT);

  // Connect to Wi-Fi
  //SetupWiFi(static_ip, gateway, subnet, ssid, password);
  setupWiFiHotSpot(static_ip_h, subnet_h, ssid_h, password_h);

  // Setup SD
  setup_SD();

  // Setup SPIFFS File System
  //setupSPIFFS();

  // Turn-off the 'brownout detector'
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Setup camera
  setupCamera();

  // Run Server
  runServer();
}

void loop() {
  if (takeNewPhoto) {
    capturePhotoSave();
    takeNewPhoto = false;
  }
  delay(1);
}



// Run Server
void runServer() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    //request->send(fileSystem, "/index.html", "text/html");
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send_P(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(fileSystem, file_name, "image/jpg", false);
  });

  // Start server
  server.begin();
}

// Setup SPIFFS File System
void setupSPIFFS() {
  while(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    delay(1000);
  }
  Serial.println("SPIFFS Initilized OK!");
}

// Connect to Wi-Fi
void setupWiFi(IPAddress static_ip, IPAddress gateway, IPAddress subnet, const char* ssid, const char* password) {
  // WiFi Config
  if(!WiFi.config(static_ip, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi  ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");

  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
}

// Wifi HotSpot
void setupWiFiHotSpot(IPAddress static_ip, IPAddress subnet, const char* ssid_h, const char* password_h) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_h, password_h);
    delay(100);
    // Seting Fixed IP
    WiFi.softAPConfig(static_ip, static_ip, subnet);
    // Fixed IP
    //IPAddress static_ip = WiFi.softAPIP();
    Serial.print("\n\nHotSpot IP address: ");
    Serial.println(static_ip);
  }  
}

// Setup Camera
void setupCamera() {
  // Setup Cameras Pins
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif
  // Inti Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  // Setup Cameras Settings
  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);       //flip it back
    s->set_brightness(s, 1);  //up the blightness just a bit
    s->set_saturation(s, -2); //lower the saturation
  }
  //drop down frame size for higher initial frame rate
  //s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_framesize(s, (framesize_t)7);
  s->set_vflip(s, 1);
  #if defined(CAMERA_MODEL_M5STACK_WIDE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  #endif
}

// Capture Photo and Save it
void capturePhotoSave( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Save Image
    writeFilePic(fileSystem, file_name, fb);

    // Return Camera to Normaly Mode
    esp_camera_fb_return(fb);

    // check if file has been correctly saved
    ok = checkPhoto(fileSystem, file_name);
  } while ( !ok );
}
