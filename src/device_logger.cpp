#include "device_logger.h"

#include <cstdio>

static constexpr const char* LogTag = "Logger";
static constexpr uint32_t LogTaskStackSize = configMINIMAL_STACK_SIZE + 1024 * 3;
static constexpr UBaseType_t LogTaskPriority = 1;

#define log_task_write(letter, format, ...) \
    _serial.printf("[%u] [" #letter "] [%s] [%s:%d]: " format "\n", \
                   esp_log_timestamp(), LogTag, DEVICE_LOG_FILE, __LINE__, ##__VA_ARGS__)
#define log_task_warn(format, ...) log_task_write(W, format, ##__VA_ARGS__)
#define log_task_error(format, ...) log_task_write(E, format, ##__VA_ARGS__)

DeviceLogger* DeviceLogger::_instance = nullptr;

DeviceLogger::DeviceLogger(
    int logMessageQueueLenght,
    int droppedLogsToWarn,
    bool allowAttatchLogTagHandles,
    HardwareSerial& serial
)
    : _logMessageQueueLenght(logMessageQueueLenght),
      _droppedLogsToWarn(droppedLogsToWarn),
      _allowAttatchLogTagHandles(allowAttatchLogTagHandles),
      _serial(serial) {}

bool DeviceLogger::begin() {
    if (_instance != nullptr) {
        log_task_error("A logger is already active");
        return false;
    }

    _logMessageQueue = xQueueCreate(_logMessageQueueLenght, sizeof(LogMessage));
    if (_logMessageQueue == nullptr) {
        log_task_error("Failed to create log queue");
        return false;
    }

    _instance = this;
    
    if (xTaskCreate(
        this->vTaskLog, 
        "Logger", 
        LogTaskStackSize, 
        nullptr, 
        LogTaskPriority, 
        &_logTaskHandle
        ) != pdPASS
    ) {
        _instance = nullptr;
        vQueueDelete(_logMessageQueue);
        _logMessageQueue = nullptr;
        log_task_error("Failed to create log task");
        return false;
    }

    esp_log_set_vprintf(this->logHook);
    
    return true;
}

bool DeviceLogger::attatchHandleFunctionToTag(std::string tag, LogMessageFromTagHandle handleFunction) {
    if (!_allowAttatchLogTagHandles) {
        log_task_error("Log tag handles not allowed: cannot attach to tag %s", tag.c_str());
        return false;
    }

    if (_logTaskHandle != nullptr) {
        log_task_error("Cannot attach handle to tag %s after begin", tag.c_str());
        return false;
    }

    if (!handleFunction) {
        log_task_error("Empty handle for tag %s", tag.c_str());
        return false;
    }

    _tagToHandleAssociation[tag] = handleFunction;
    DEVICE_LOGI(LogTag, "Handle attached to tag %s", tag.c_str());
    return true;
}

void DeviceLogger::setTagLevel(const std::string& tag, int level) {
    esp_log_level_set(tag.c_str(), static_cast<esp_log_level_t>(level));
}

std::string DeviceLogger::getTagFromMessage(const LogMessage& message) {
    char tag[32];

    if (
        sscanf(message.text, "[%*u] [%*c] [%31[^]]]", tag) == 1 ||
        sscanf(message.text, "%*c (%*u) %31[^:]:", tag) == 1
    ) {
        return tag;
    }
    return "";
}

int DeviceLogger::logHook(const char* format, va_list arg) {
    LogMessage message;

    const int written = vsnprintf(message.text, LogMessageMaxLenght, format, arg);
    if (written < 0) {
        return written;
    }

    const bool truncated = written >= static_cast<int>(LogMessageMaxLenght);
    if (truncated) {
        message.text[LogMessageMaxLenght - 2] = '\n';
    }
    message.lenght = truncated ? LogMessageMaxLenght - 1 : written;

    if (xQueueSend(_instance->_logMessageQueue, &message, 0) != pdTRUE) {
        _instance->_droppedLogs++;
    }
    return written;
}

void DeviceLogger::vTaskLog(void*) {
    LogMessage message;

    while (true) {
        if (xQueueReceive(_instance->_logMessageQueue, &message, portMAX_DELAY) == pdTRUE) {
            if (!_instance->_allowAttatchLogTagHandles || !_instance->callTagHandle(message)) {
                _instance->printMessage(message);
            }
            _instance->warnDroppedLogs();
        }
    }
}

bool DeviceLogger::printMessage(const LogMessage& message) {
    return _serial.write(message.text, message.lenght) == message.lenght;
}

bool DeviceLogger::callTagHandle(const LogMessage& message) {
    if (_tagToHandleAssociation.empty()) {
        return false;
    }

    const std::string tag = this->getTagFromMessage(message);
    const auto association = _tagToHandleAssociation.find(tag);
    if (association == _tagToHandleAssociation.end()) {
        return false;
    }

    if (!association->second(tag, message)) {
        log_task_error("Handle of tag %s failed", tag.c_str());
    }
    return true;
}

void DeviceLogger::warnDroppedLogs() {
    const int dropped = _droppedLogs.load();

    if (dropped > 0 && dropped >= _droppedLogsToWarn) {
        log_task_warn("%d logs dropped: queue full", _droppedLogs.exchange(0));
    }
}
