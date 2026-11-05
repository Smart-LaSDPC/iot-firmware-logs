/*
    Saída em outra serial: o logger não inicia a serial recebida, então ela
    precisa ser iniciada antes de begin().
*/
#include <Arduino.h>
#include "device_logger.h"

static constexpr const char* LogTag = "Example";

static constexpr int LogMessageQueueLenght = 16;
static constexpr int DroppedLogsToWarn = 3;
static constexpr bool AllowTagHandles = false;

static constexpr int LogSerialRxPin = 16;
static constexpr int LogSerialTxPin = 17;

static DeviceLogger Logger(LogMessageQueueLenght, DroppedLogsToWarn, AllowTagHandles, Serial2);

void setup() {
    Serial2.begin(115200, SERIAL_8N1, LogSerialRxPin, LogSerialTxPin);

    Logger.setTagLevel("*", ESP_LOG_INFO);
    Logger.begin();
}

void loop() {
    ESP_LOGI(LogTag, "Message on Serial2");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
