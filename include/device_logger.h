#pragma once

/**
 * @file device_logger.h
 * @brief Logger assíncrono: as tasks enfileiram as mensagens, e uma única task
 * escreve na saída serial.
 */
#include <Arduino.h>
#include <atomic>
#include <map>
#include <string>
#include <functional>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "device_log_format.h"

/** @brief Tamanho máximo de uma mensagem de log, incluindo o terminador nulo. */
static constexpr size_t LogMessageMaxLenght = 128;

/**
 * @brief Recebe os logs da ESP-IDF por um hook e os imprime em uma task dedicada,
 * para que só ela use a serial.
 *
 * O hook da ESP-IDF é global: só uma instância fica ativa, e ela precisa existir
 * até o reset do dispositivo.
 */
class DeviceLogger {
  public:
    /**
     * @brief Define as configurações do logger. A fila e a task só são criadas em begin().
     *
     * @param logMessageQueueLenght Capacidade da fila, em mensagens.
     * @param droppedLogsToWarn Quantidade de descartes que gera um aviso.
     * @param allowAttatchLogTagHandles Permite associar funções a tags. Se false,
     * a task imprime todas as mensagens, sem procurar por funções associadas.
     * @param serial Serial de saída dos logs. O logger não a inicia.
     */
    DeviceLogger(
        int logMessageQueueLenght = 4,
        int droppedLogsToWarn = 3,
        bool allowAttatchLogTagHandles = false,
        HardwareSerial& serial = Serial
    );

    /**
     * @brief Cria a fila e a task de logs e instala o hook em esp_log_set_vprintf.
     *
     * Chame depois de iniciar a serial: antes disso, a task não tem por onde escrever.
     *
     * @return true se a inicialização foi concluída; false se faltou memória ou
     * se já existe um logger ativo.
     */
    bool begin();
    
    /** @brief Item da fila: uma linha de log já formatada. */
    struct LogMessage {
        size_t lenght;                      ///< Quantidade de caracteres em text.
        char text[LogMessageMaxLenght];     ///< Texto da mensagem.
    };

    /**
     * @brief Obtém a tag do texto de uma mensagem, pois esp_log_write não a repassa ao hook.
     *
     * Reconhece o formato do firmware e o padrão das bibliotecas pré-compiladas.
     *
     * @param message Mensagem de log.
     * @return Tag da mensagem, com até 31 caracteres, ou string vazia se o texto não
     * tiver tag.
     */
    static std::string getTagFromMessage(const LogMessage& message);

    /**
     * @brief Função chamada para cada mensagem de uma tag, no lugar da impressão na serial.
     *
     * Roda na task de logs: enquanto ela não retorna, nenhum log é impresso. Por
     * isso não deve bloquear, nem gerar logs com a própria tag, o que a chamaria
     * de novo a cada mensagem.
     *
     * @param tag Tag da mensagem.
     * @param message Mensagem recebida, já formatada.
     * @return true se a mensagem foi tratada; false gera um erro na serial, e a
     * mensagem não é impressa.
     */
    using LogMessageFromTagHandle = std::function<bool(std::string, LogMessage)>;

    /**
     * @brief Associa uma função às mensagens de uma tag.
     *
     * Cada tag tem uma função: uma nova associação substitui a anterior. A tag vem
     * do texto da mensagem, então vale para o formato do firmware e para o padrão
     * das bibliotecas pré-compiladas, mas não para mensagens truncadas antes dela.
     *
     * @param tag Tag a ser tratada, com até 31 caracteres.
     * @param handleFunction Função chamada para cada mensagem da tag.
     * @return true se a função foi associada; false se as associações não foram
     * permitidas no construtor, se a função é vazia ou se begin() já foi chamado,
     * pois a task lê as associações sem sincronização.
     */
    bool attatchHandleFunctionToTag(std::string tag, LogMessageFromTagHandle handleFunction);

    /**
     * @brief Define em tempo de execução o nível de log de uma tag.
     *
     * Só alcança logs mantidos na compilação: um nível acima de LOG_LOCAL_LEVEL não
     * tem efeito. A tag "*" vale para todas as tags e descarta os níveis definidos
     * antes, por isso deve vir primeiro.
     *
     * @param tag Tag a ser configurada, ou "*" para todas.
     * @param level Nível máximo de log da tag, como esp_log_level_t ou como número:
     * 0 nenhum, 1 Error, 2 Warn, 3 Info, 4 Debug e 5 Verbose.
     */
    static void setTagLevel(const std::string& tag, int level);

    /**
     * @brief Formata a mensagem e a coloca na fila, sem bloquear.
     *
     * Recebe as mensagens de esp_log_write, em paralelo, de qualquer task. Por
     * isso deve ser reentrante e não pode gerar logs. Roda na pilha de quem gerou
     * o log. Com a fila cheia, a mensagem é descartada e contada; mensagens maiores
     * que LogMessageMaxLenght são truncadas, mantendo a quebra de linha.
     *
     * @param format Formato recebido da ESP-IDF, já com o prefixo da linha.
     * @param arg Argumentos do formato, válidos apenas durante a chamada.
     * @return Quantidade de caracteres formatados, ou valor negativo em caso de erro.
     */
    static int logHook(const char* format, va_list arg);

    /**
     * @brief Task que retira as mensagens da fila e chama a função associada à tag,
     * se as associações foram permitidas no construtor. As demais mensagens são
     * escritas na serial.
     *
     * Quando os descartes atingem droppedLogsToWarn, escreve o aviso direto na
     * serial, pois a fila pode estar cheia.
     *
     * @param pvParameters Não utilizado: a task usa a instância ativa.
     */
    static void vTaskLog(void* pvParameters);

  private:
    static DeviceLogger* _instance;

    bool printMessage(const LogMessage& message);
    bool callTagHandle(const LogMessage& message);
    void warnDroppedLogs();

    TaskHandle_t _logTaskHandle = nullptr;      
    QueueHandle_t _logMessageQueue = nullptr;   
    int _logMessageQueueLenght = 4;            
    
    std::atomic<int> _droppedLogs = 0;          
    int _droppedLogsToWarn = 3;         
    bool _allowAttatchLogTagHandles = false;
    HardwareSerial& _serial;
    std::map<std::string, LogMessageFromTagHandle> _tagToHandleAssociation;
};