/*
    Handle por tag: as mensagens da tag "Alarm" vão para uma função, no lugar da
    impressão na serial. As demais tags seguem para a serial.
*/
#include <Arduino.h>
#include "device_logger.h"

static constexpr const char* LogTag = "Example";
static constexpr const char* AlarmTag = "Alarm";

static constexpr int LogMessageQueueLenght = 16;
static constexpr int DroppedLogsToWarn = 3;
static constexpr bool AllowTagHandles = true;

static DeviceLogger Logger(LogMessageQueueLenght, DroppedLogsToWarn, AllowTagHandles);

/*
    Roda na task de logs: não deve bloquear nem gerar logs com a própria tag.
*/
static bool handleAlarm(std::string tag, DeviceLogger::LogMessage message) {
    Serial.print("ALARM >> ");
    Serial.write(message.text, message.lenght);
    return true;
}

void setup() {
    Serial.begin(115200);

    Logger.setTagLevel("*", ESP_LOG_INFO);

    // Os handles precisam ser associados antes de begin()
    Logger.attatchHandleFunctionToTag(AlarmTag, handleAlarm);
    Logger.begin();
}

void loop() {
    ESP_LOGI(LogTag, "Goes to the serial");
    ESP_LOGW(AlarmTag, "Goes to the handle");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
