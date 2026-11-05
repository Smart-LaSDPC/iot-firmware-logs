# 📝 iot-firmware-logs

Logs assíncronos para firmwares ESP32 com Arduino e FreeRTOS. As tasks geram logs pela biblioteca da ESP-IDF, um hook coloca as mensagens em uma fila, e uma única task escreve na serial.

```text
[timestamp] [level] [TAG] [arquivo:linha]: text
[10432] [I] [Sensor] [task_sensor.cpp:66]: Sensor list received with 3 types
```

## ⚙️ Instalação

Adicione a biblioteca em `lib/`:

```bash
git submodule add https://github.com/Smart-LaSDPC/iot-firmware-logs.git lib/iot-firmware-logs
```

Configure o `platformio.ini`:

```ini
build_unflags = -std=gnu++11
build_flags =
	-include $PROJECT_DIR/lib/iot-firmware-logs/include/device_log_format.h
	-D USE_ESP_IDF_LOG
	-D CORE_DEBUG_LEVEL=3
	-D LOG_LOCAL_LEVEL=3
	-D TAG=\"ARDUINO\"
	-std=gnu++17
```

O `-include` aplica o formato a todo arquivo compilado, sem precisar de `#include`.

## 🚀 Uso

Inicie o logger logo após a serial:

```cpp
#include "device_logger.h"

static DeviceLogger deviceLogger(16);   // capacidade da fila, em mensagens

void setup() {
    Serial.begin(115200);
    deviceLogger.setTagLevel("*", ESP_LOG_INFO);   // sem isso, só Error aparece
    deviceLogger.begin();                          // registra o próprio erro, se falhar
}
```

Depois, gere logs de qualquer task com as macros `ESP_LOGx` da ESP-IDF. **Esta é a forma recomendada:** não exige nenhum `#include` da biblioteca, e tudo o que usa `ESP_LOGx` passa pelo logger e sai no formato acima.

```cpp
static constexpr const char* LogTag = "Sensor";

ESP_LOGI(LogTag, "Temperature: %.2f", temperature);
```

| ESP-IDF | Biblioteca | Nível | `LOG_LOCAL_LEVEL` mínimo | Quando usar |
|---|---|---|---|---|
| `ESP_LOGE` | `DEVICE_LOGE` | Error | 1 | A operação falhou e o efeito se perdeu |
| `ESP_LOGW` | `DEVICE_LOGW` | Warn | 2 | Situação inesperada, com recuperação automática |
| `ESP_LOGI` | `DEVICE_LOGI` | Info | 3 | Operação normal |
| `ESP_LOGD` | `DEVICE_LOGD` | Debug | 4 | Depuração |
| `ESP_LOGV` | `DEVICE_LOGV` | Verbose | 5 | Detalhamento máximo |

As duas colunas de macros são equivalentes, e `log_x` também funciona. Não nomeie uma variável como `TAG`, pois ele é uma macro das `build_flags`.

Ajuste o nível de uma tag depois do `"*"`, que descarta os níveis definidos antes:

```cpp
deviceLogger.setTagLevel("Sensor", ESP_LOG_WARN);
```

Para usar outra serial, passe-a no construtor e inicie-a antes de `begin()`:

```cpp
static DeviceLogger deviceLogger(16, 3, false, Serial2);
```

## 🏷️ Handles por tag

Um handle recebe as mensagens de uma tag no lugar da serial. Permita o recurso no construtor e registre os handles antes de `begin()`:

```cpp
static DeviceLogger deviceLogger(16, 3, true);

deviceLogger.attatchHandleFunctionToTag("Sensor", [](std::string tag, DeviceLogger::LogMessage message) {
    return publishLog(message.text);
});
deviceLogger.begin();
```

O handle roda na task de logs, então não deve bloquear nem gerar logs com a própria tag. Se retornar `false`, a mensagem se perde e um erro é escrito na serial.

## 📚 Exemplos

Cada exemplo é um único `.cpp` em `examples/`:

| Exemplo | O que mostra |
|---|---|
| `basic` | Início do logger, níveis e filtro por tag |
| `multi_task` | Várias tasks gerando logs ao mesmo tempo |
| `tag_handle` | Mensagens de uma tag enviadas a uma função |
| `custom_serial` | Saída em outra serial |

Para compilar um exemplo, rode na raiz da biblioteca:

```bash
pio ci examples/basic --lib . --board esp32dev \
  -O "framework=arduino" \
  -O "build_unflags=-std=gnu++11" \
  -O "build_flags=-include $PWD/include/device_log_format.h -D USE_ESP_IDF_LOG -D CORE_DEBUG_LEVEL=3 -D LOG_LOCAL_LEVEL=3 -D TAG=\\\"ARDUINO\\\" -std=gnu++17"
```

Em um firmware com as `build_flags` da seção de instalação, basta usar o exemplo como `src/main.cpp`.

## ⚠️ Limitações

- **Tamanho:** cada linha tem até 127 caracteres, com o prefixo, e o excesso é truncado.
- **Fila cheia:** a mensagem é descartada. Um aviso sai a cada `droppedLogsToWarn` descartes.
- **Deep sleep e crash:** o que ainda está na fila se perde.
- **Instância única:** um segundo `begin()` retorna `false`.
- **Fora do logger:** `Serial.print*`, `printf`, `ets_printf` e `ESP_EARLY_LOGx`.
- **ISR:** não gere logs em interrupções.
- **Bibliotecas pré-compiladas:** passam pela fila, mas mantêm o formato padrão da ESP-IDF.
- **Recompilação:** ao alterar `device_log_format.h`, rode `pio run -t clean`.
