
/* Edge Impulse Arduino examples
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0
#include <Snoring_Detection_3_inferencing.h>
#include <esp_now.h>
#include <Arduino.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s.h"
uint8_t broadcastAddress[] = {0x14, 0x33, 0x5c, 0x2e, 0x27, 0x98};
esp_now_peer_info_t peerInfo = {};

static int snore_detected_count = 0;
static unsigned long last_snore_time = 0;
bool first_prediction = true;
bool snore_detected_last_time = false;
bool snoring_active = false;
bool pump_cycle_active = false;
bool deflate_cycle_active = false;
static bool valve_on_sent = false;
static bool deflate_reset_pending = false;
int pump_times = 0;
int deflate_times = 0;  // ← biến đếm số lần xả

unsigned long pump_cycle_start_time = 0;
unsigned long deflate_start_time = 0;
unsigned long deflate_done_time = 0;

#define LED_BUILT_IN 2
#define MAX_INTERVAL 10000 // 10 giây
#define I2S_PORT I2S_NUM_1
#define PUMP_DURATION      5000  // 5 giây bơm
#define DEFLATE_DURATION   1000  // 5 giây xả
#define REST_DURATION      35000 // 30 giây xả
//#define MAX_INTERVAL       5000  // 5 giây giữa các tiếng ngáy
#define MAX_PUMP_COUNT     8     // 9 lần bơm
#define MAX_DEFLATE_COUNT   8



//const unsigned long PUMP_DURATION = 30000;  // 30s
//const unsigned long REST_DURATION = 30000;  // 30s


/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool record_status = true;

/**
 * @brief      Arduino setup function
 */
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");
    
    

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf(" ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    ei_printf("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    ei_printf("Recording...\n");
    
    // Thiết lập chế độ WiFi ở STA (Station mode)
    WiFi.mode(WIFI_STA);

    // Khởi tạo ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
    pinMode(LED_BUILT_IN, OUTPUT);
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
    /**/
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    int pred_index = 0;     // Initialize pred_index
    float pred_value = 0;   // Initialize pred_value

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
         result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          ei_printf("    %s: ", result.classification[ix].label);
          ei_printf_float(result.classification[ix].value);
          ei_printf("\n");
    
          if (result.classification[ix].value > pred_value){
             pred_index = ix;
             pred_value = result.classification[ix].value;
          }
    }
    
    
    // show the inference result on LED
    
    
    

    if (pred_index == 1) { // Phát hiện tiếng ngáy
      
    unsigned long current_time = millis();
     if (!snore_detected_last_time) {  // CHỈ ghi nhận nếu lần trước KHÔNG phải snoring
        snore_detected_last_time = true;
        
    if (first_prediction) {
        first_prediction = false;
        
    } else {
      const char *msg_snore_flag = "snoring";
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)msg_snore_flag, strlen(msg_snore_flag) + 1);
      
        if (result == ESP_OK) {
                    ei_printf("Sent: %s\n", msg_snore_flag);
                } else {
                    ei_printf("ESP-NOW send failed\n");
                }
        if (snore_detected_count == 0) {
            snore_detected_count = 1;
            last_snore_time = current_time;
        } else if (current_time - last_snore_time <= MAX_INTERVAL) {
          
            snore_detected_count++;
            last_snore_time = current_time;
            
            if (snore_detected_count >= 3) {
                const char *msg = "snoring episode";
                esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)msg, strlen(msg) + 1);
                if (result == ESP_OK) {
                    ei_printf("Sent: %s\n", msg);
                } else {
                    ei_printf("ESP-NOW send failed\n");
                }
                 if(!pump_cycle_active&&!deflate_cycle_active){
                  if(pump_times < MAX_PUMP_COUNT){
                    const char *msg2 = "pump";
                    const char *msg3 = "valve on";
                esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *)msg2, strlen(msg2) + 1);
                if (result2 == ESP_OK) {
                    ei_printf("Sent: %s\n", msg2);
                    deflate_times = 0;
                } else {
                    ei_printf("ESP-NOW send failed\n");
                }
                esp_err_t result3 = esp_now_send(broadcastAddress, (uint8_t *)msg3, strlen(msg3) + 1);
                if (result3 == ESP_OK) {
                    ei_printf("Sent: %s\n", msg3);
                } else {
                    ei_printf("ESP-NOW send failed\n");
                }
                  digitalWrite(LED_BUILT_IN, LOW); // Bật LED
                
                 //snoring_active = true; // <--- Kích hoạt chế độ snoring
                 pump_cycle_active = true;
                    pump_cycle_start_time = current_time;
                    pump_times++;
                    ei_printf("Pump count: %i\n", pump_times);
                  }/*else{
                    deflate_cycle_active = true;
                    deflate_start_time = current_time;
                    const char *msg4 = "valve off";
                    esp_err_t result4 = esp_now_send(broadcastAddress, (uint8_t *)msg4, strlen(msg4) + 1);
                    if (result4 == ESP_OK) {
                    ei_printf("Sent: %s\n", msg4);
                } else {
                    ei_printf("ESP-NOW send failed\n");
                }
                  }*/
                  else if (deflate_times < MAX_DEFLATE_COUNT) {
    deflate_cycle_active = true;
    deflate_start_time = current_time;
    deflate_times++;  // đếm số lần xả
    
    const char *msg4 = "valve off";
    esp_now_send(broadcastAddress, (uint8_t *)msg4, strlen(msg4) + 1);
    ei_printf("Sent: %s\n", msg4);
    ei_printf("Deflate count: %i\n", deflate_times);
}
                 }
             snore_detected_count = 0; // Reset đếm   
            }
        } else {
            
            snore_detected_count = 1;
            
            last_snore_time = current_time;
        }
    }
  }
} else {
   // Không phát hiện tiếng ngáy hiện tại
    unsigned long current_time = millis();

    // Nếu đang ở chế độ snoring, nhưng quá 10s không có tiếng ngáy -> gửi non_snoring
   /* if (snoring_active && (current_time - last_snore_time > MAX_INTERVAL)) {
        const char *msg = "no pump";
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)msg, strlen(msg) + 1);
        if (result == ESP_OK) {
            ei_printf("Sent: %s\n", msg);
        } else {
            ei_printf("ESP-NOW send failed\n");
        }
        
       
        snoring_active = false; // <--- Tắt trạng thái
        snore_detected_count = 0;
    }
*/

// Xử lý kết thúc chu kỳ pump + xả
    // Kết thúc chu kỳ bơm sau 5 giây
    if (pump_cycle_active && (current_time - pump_cycle_start_time >= PUMP_DURATION)) {
        pump_cycle_active = false;
        digitalWrite(LED_BUILT_IN, HIGH); // tắt LED
        // Gửi lệnh tắt bơm
    const char *msg = "no pump";
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)msg, strlen(msg) + 1);
    
    if (result == ESP_OK) {
            ei_printf("Sent: %s\n", msg);
        } else {
            ei_printf("ESP-NOW send failed\n");
        }
    }

    // Kết thúc chu kỳ xả sau 30 giây
   /* if (deflate_cycle_active && (current_time - deflate_start_time >= REST_DURATION)) {
        deflate_cycle_active = false;
        pump_times = 0;  // reset bộ đếm bơm để bắt đầu lại từ đầu
        ei_printf("Pump count reset to 0\n");
    }*/
    // Nếu đang trong chu kỳ xả, xử lý mở van 5s rồi đóng lại
if (deflate_cycle_active) {
    unsigned long elapsed = current_time - deflate_start_time;

   /* if (elapsed >= DEFLATE_DURATION && elapsed < DEFLATE_DURATION + 100) {
        const char *msg_on = "valve on";
        esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *)msg_on, strlen(msg_on) + 1);
        if(result2 == ESP_OK){
          ei_printf("Sent: %s\n", msg_on);
        }else{
          ei_printf("ESP-NOW send failed\n");
        }
        
    }*/
    // Sau đúng 5 giây thì gửi valve on nếu chưa gửi
    if (elapsed >= DEFLATE_DURATION && !valve_on_sent) {
        const char *msg_on = "valve on";
        esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *)msg_on, strlen(msg_on) + 1);
        if(result2 == ESP_OK){
            ei_printf("Sent: %s\n", msg_on);
        }else{
            ei_printf("ESP-NOW send failed\n");
        }

        valve_on_sent = true;
        //valve_sent_time = millis();
        //deflate_done_time = current_time; // đánh dấu mốc thời gian gửi valve on
    }

    if (elapsed >= (DEFLATE_DURATION+200)) {
        deflate_cycle_active = false;
        valve_on_sent = false;  // reset flag cho lần tiếp theo
        //ei_printf("Deflate cycle complete\n");

        // Nếu đã xả đủ thì reset đếm để quay lại bơm
        if (deflate_times >= MAX_DEFLATE_COUNT) {
            deflate_times = 0;
            pump_times = 0;
            ei_printf("Pump & Deflate count reset\n");
        }
    }
}

    // Reset nếu khoảng cách tiếng ngáy quá xa
    if (snore_detected_count > 0 && (current_time - last_snore_time > MAX_INTERVAL)) {
        snore_detected_count = 0;
    }
    digitalWrite(LED_BUILT_IN, HIGH); // Tắt LED
    snore_detected_last_time = false; // Reset trạng thái lần trước
}
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: ");
    ei_printf_float(result.anomaly);
    ei_printf("\n");
#endif
}

static void audio_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffer[inference.buf_count++] = sampleBuffer[i];

        if(inference.buf_count >= inference.n_samples) {
          inference.buf_count = 0;
          inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {

  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  

  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {

    /* read data at once from i2s */
    i2s_read((i2s_port_t)1, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      ei_printf("Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < i2s_bytes_to_read) {
        ei_printf("Partial I2S read");
        }
        
        // scale the data (otherwise the sound is too quiet)
        for (int x = 0; x < i2s_bytes_to_read/2; x++) {
            sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
            
        }
         // Tính giá trị trung bình RMS
    
    //delay(1000);
        
        if (record_status) {
            audio_inference_callback(i2s_bytes_to_read);
        }
        else {
            break;
        }
    }
  }
  vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start I2S!");
    }

    ei_sleep(100);

    record_status = true;

    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
    bool ret = true;

    while (inference.buf_ready == 0) {
        delay(10);
    }

    inference.buf_ready = 0;
    return ret;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    i2s_deinit();
    ei_free(inference.buffer);
}


static int i2s_init(uint32_t sampling_rate) {
  // Start listening for audio: MONO @ 8/16KHz
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // ESP32 la master va chon che do nhan du lieu tu INMP441
      .sample_rate = sampling_rate,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 1024,// gia tri ban dau: 512, tang len de thu duoc doan am thanh dai hon
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = 14,    // IIS_SCLK
      .ws_io_num = 15,     // IIS_LCLK
      .data_out_num = -1,  // IIS_DSIN
      .data_in_num = 32,   // IIS_DOUT
  };
  esp_err_t ret = 0;

  ret = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_driver_install");
  }

  ret = i2s_set_pin((i2s_port_t)1, &pin_config);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_set_pin");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)1);
  if (ret != ESP_OK) {
    ei_printf("Error in initializing dma buffer with 0");
  }

  return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)1); //stop & destroy i2s driver
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
