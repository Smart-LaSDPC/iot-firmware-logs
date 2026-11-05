/*
    Uso básico: inicia o logger e gera logs de cada nível com ESP_LOGx.
*/
#include <Arduino.h>
#include "device_logger.h"

static constexpr const char* LogTag = "Example";
static constexpr int LogMessageQueueLenght = 16;

static DeviceLogger Logger(LogMessageQueueLenght);

void setup() {
    Serial.begin(115200);

    // O "*" vem primeiro: ele descarta os níveis definidos antes
    Logger.setTagLevel("*", ESP_LOG_INFO);
    Logger.setTagLevel("Quiet", ESP_LOG_ERROR);
    Logger.begin();

    ESP_LOGE(LogTag, "Error message");
    ESP_LOGW(LogTag, "Warning message");
    ESP_LOGI(LogTag, "Info message: %d", 42);

    // Abaixo do nível da tag: não aparece na serial
    ESP_LOGW("Quiet", "Filtered message");
}

void loop() {
    static unsigned int counter = 0;

    ESP_LOGI(LogTag, "Loop iteration: %u", counter++);
    vTaskDelay(pdMS_TO_TICKS(1000));
}
