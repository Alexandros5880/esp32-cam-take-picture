#include "get_frames.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"

void do_somthing_with_frames() {
  camera_fb_t * fb = NULL; // pointer

  do {

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    uint8_t * buf = NULL;
    size_t buf_len = 0;
    bool converted = frame2bmp(fb, &buf, &buf_len);

    // Return the frame buffer to be reused again.
    esp_camera_fb_return(fb);

    if(!converted){
        Serial.println("BMP Conversion failed!");
    }

    Serial.write((const char *)buf, buf_len);

    free(buf);
  } while ( fb );
}