#include <SPIFFS.h>
#include <Arduino.h>
#include "esp_camera.h"
//#include <WiFiManager.h>
//#include <ESPAsyncWebServer.h>
#include "NeuralNetwork.h"
#include "image_util.h"
#include "dl_lib_matrix3d.h"

//#include "esp_http_server.h"

NeuralNetwork *nn;
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#define LED_BUILTIN 4

#include "camera_pins.h"

camera_config_t config;


void config_init();

// Create AsyncWebServer object on port 80
// AsyncWebServer server(80);

boolean takeNewPhoto = false;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

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
  
  Serial.begin(115200);
  
  Serial.printf("\n 1. Free heap: %d  // %d  // MaxAllocHeap: %d \n ", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap() );
  Serial.printf("Total PSRAM: %d", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d", ESP.getFreePsram());
  
  // SPIFFS INIT ----------------------------------------------------------------------------------


  if (!SPIFFS.begin(true)) {
  Serial.println("An Error has occurred while mounting SPIFFS");
  ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  

  // WIFI INIT ----------------------------------------------------------------------------------
  
  /*
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  //WiFi.mode(WIFI_AP);
  //WiFi.softAP("ESP32WIFI", NULL);
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  //res = wm.autoConnect(); // password protected ap

  
  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("Connected...yeey :)");
  }
  
  Serial.printf("\n PRE init Free heap: %d  // %d  // MaxAllocHeap: %d \n ", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap() );
  
  
    // Route for root / web page
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send_P(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });

  // Start server
  server.begin();
  

  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  */
  // ---------------------------------------------------------------------------------------------
  //  ---------------------------------------CAMER INIT-------------------------------------------
  config_init(); 
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  } else {
    Serial.println("Camera init OK");
  }
  Serial.printf("\n AFTER init Free heap: %d  // %d  // MaxAllocHeap: %d \n ", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap() );
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 0);        //1-Upside down, 0-No operation
  s->set_hmirror(s, 0);      //1-Reverse left and right, 0-No operation
  s->set_brightness(s, -2);   //up the blightness just a bit
  s->set_saturation(s, -1);  //lower the saturation

  pinMode (LED_BUILTIN, OUTPUT);

  // ---------------------------------------------------------------------------------------------

  // NN INIT ----------------------------------------------------------------------------------

  nn = new NeuralNetwork();

  Serial.printf("Total PSRAM: %d", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d", ESP.getFreePsram());
  
}

bool isAllZeros(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (buffer[i] != 0) {
            return false;  // Si encuentra un byte distinto de cero, devuelve falso.
        }
    }
    return true;  // Si no se encontraron bytes distintos de cero, devuelve verdadero.
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo, classify and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  uint8_t *ei_buf;
  uint8_t *rgb888_buf = NULL;
  size_t out_len;
  bool s;
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");
    Serial.printf("\n photo init Free heap: %d  // %d  // MaxAllocHeap: %d \n ", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap() );
    Serial.printf("Total PSRAM: %d ", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d \n", ESP.getFreePsram());
    digitalWrite(LED_BUILTIN, HIGH);
    
    fb = esp_camera_fb_get();
    delay(3000);

    if (!fb) {
      Serial.println("Camera capture failed");
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }
    
    

  if (isAllZeros(fb->buf, fb->len)) {
    Serial.println("1. fb->buf contiene solo ceros. \n");
  } else {
    Serial.println("1. fb->buf contiene al menos un byte distinto de cero. \n");
  }
  
  Serial.printf("fb->len: %d \n", fb->len);
  
  // PROCESAMIENTO DE LA FOTO
  // Photo to RGB888:
  rgb888_buf = (uint8_t *) ps_malloc(fb->width * fb->height * 3); 
  if(rgb888_buf == NULL){
    printf("Malloc failed to allocate buffer for JPG.\n");
  }
  Serial.println("Converting to RGB888...\n");
  s = fmt2rgb888 (fb->buf, fb->len, fb->format, rgb888_buf);
  if (!s)
  {
      free(rgb888_buf);
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("To RGB888 failed. \n");
      return;
  }

  // Resize Image (800, 600, 3) -> (72, 72, 3):
  ei_buf = (uint8_t *) ps_malloc(72*72*3); 
  Serial.println("Resizing the frame buffer... \n");
  image_resize_linear(ei_buf, rgb888_buf, 72, 72, 3, fb->width, fb->height);
  free(rgb888_buf);
  Serial.printf("El size de ei_buf es: %d ", sizeof(ei_buf));
  
  //--------------------------FIN PRE-PROCESAMIENTO----------------------------------------------
  //---------------------------------------------------------------------------------------------

  // CLASIFICACIÓN 
  
  float result = nn->classify_image(ei_buf);

  const char *predicted = result > 0.5 ? "Cardboard/Paper" : "Plastic";
  
  if (result > 0.5){
    result = result;
  }
  else{
    result = 1 - result;
  }

  Serial.printf("Predicted Value: %f , Predicted %s\n", result, predicted);
  

  // CONVERSION A JPG Para ver en webpage
   
  // Descomentar file.write(jpg_buf, jpg_size); y free(jpg_buf);
  /*
  uint8_t * jpg_buf = (uint8_t *) heap_caps_malloc(100000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(jpg_buf == NULL){
    printf("Malloc failed to allocate buffer for JPG.\n");
  }
  size_t jpg_size = 0;
  fmt2jpg(ei_buf, sizeof(ei_buf), 48 , 48, PIXFORMAT_RGB888, 40, &jpg_buf, &jpg_size); // (rgb888 48x48) -> jpg 
  printf("Converted JPG size: %d bytes \n", jpg_size);
  */
 
  Serial.printf("Total PSRAM FIN: %d ", ESP.getPsramSize());
  Serial.printf("Free PSRAM FIN: %d \n", ESP.getFreePsram());


  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO);
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  
  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    //file.write(jpg_buf, jpg_size); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO);
    Serial.print(" - Size: ");
    Serial.print(file.size());
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  
  digitalWrite(LED_BUILTIN, LOW);
  //free(jpg_buf);
  //jpg_buf = NULL;
  //rgb888_buf = NULL;
  esp_camera_fb_return(fb);
  free(ei_buf);

  Serial.printf("After Free Memory - Total PSRAM: %d ", ESP.getPsramSize());
  Serial.printf("After Free Memory - Free PSRAM: %d \n", ESP.getFreePsram());

  // check if file has been correctly saved in SPIFFS
  //ok = checkPhoto(SPIFFS);
  Serial.printf("OK STATUS: %d\n", ok);
  ok = 1;
  } while ( !ok );
  
}

void loop() {
  
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }
  delay(5000);
  takeNewPhoto = true;
}


void config_init() {
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
  config.xclk_freq_hz = 16500000;
  config.frame_size = FRAMESIZE_SVGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;
}
