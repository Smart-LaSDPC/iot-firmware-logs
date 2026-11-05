/*
    Várias tasks geram logs ao mesmo tempo: só a task do logger usa a serial,
    então as linhas não se misturam.
*/
#include <Arduino.h>
#include "device_logger.h"

static constexpr int LogMessageQueueLenght = 16;
static constexpr uint32_t ProducerStackSize = 4096;
static constexpr UBaseType_t ProducerPriority = 1;

static DeviceLogger Logger(LogMessageQueueLenght);

static void vTaskProducer(void* pvParameters) {
    const char* tag = pcTaskGetName(nullptr);
    const TickType_t period = pdMS_TO_TICKS(reinterpret_cast<uintptr_t>(pvParameters));
    unsigned int counter = 0;

    while (true) {
        ESP_LOGI(tag, "Message: %u", counter++);
        vTaskDelay(period);
    }
}

void setup() {
    Serial.begin(115200);

    Logger.setTagLevel("*", ESP_LOG_INFO);
    Logger.begin();

    xTaskCreate(vTaskProducer, "Fast", ProducerStackSize, reinterpret_cast<void*>(250), ProducerPriority, nullptr);
    xTaskCreate(vTaskProducer, "Slow", ProducerStackSize, reinterpret_cast<void*>(1000), ProducerPriority, nullptr);
}

void loop() {
    vTaskDelete(nullptr);
}
