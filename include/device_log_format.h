#pragma once

/**
 * @file device_log_format.h
 * @brief Formato dos logs: [timestamp] [level] [TAG] [arquivo:linha]: text
 *
 * Injetado em todo arquivo compilado pelo -include das build_flags, sem
 * precisar de \#include. Bibliotecas pré-compiladas mantêm o formato padrão.
 */
#ifndef __ASSEMBLER__
    #include <esp_log.h>

    /** @brief Nome do arquivo atual, sem o caminho. */
    #define DEVICE_LOG_FILE \
        (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

    /**
     * @brief Escreve uma mensagem no formato do firmware.
     *
     * @param level Nível da mensagem (esp_log_level_t), usado no filtro por tag.
     * @param letter Letra do nível impressa na linha, sem aspas.
     * @param tag Tag da mensagem.
     * @param format Formato no estilo printf: precisa ser um literal de string.
     * @param ... Argumentos do formato.
     */
    #define DEVICE_LOG_WRITE(level, letter, tag, format, ...) \
        esp_log_write(level, tag, "[%u] [" #letter "] [%s] [%s:%d]: " format "\n", \
                    esp_log_timestamp(), tag, DEVICE_LOG_FILE, __LINE__, ##__VA_ARGS__)

    #undef ESP_LOG_LEVEL
    /**
     * @brief Substitui a macro da ESP-IDF para que ESP_LOGx e log_x usem o formato do firmware.
     *
     * Níveis desconhecidos são tratados como Info, como na macro original.
     *
     * @param level Nível da mensagem (esp_log_level_t).
     * @param tag Tag da mensagem.
     * @param format Formato no estilo printf: precisa ser um literal de string.
     * @param ... Argumentos do formato.
     */
    #define ESP_LOG_LEVEL(level, tag, format, ...) \
        do { \
            switch (level) { \
                case ESP_LOG_ERROR:   DEVICE_LOG_WRITE(ESP_LOG_ERROR,   E, tag, format, ##__VA_ARGS__); break; \
                case ESP_LOG_WARN:    DEVICE_LOG_WRITE(ESP_LOG_WARN,    W, tag, format, ##__VA_ARGS__); break; \
                case ESP_LOG_DEBUG:   DEVICE_LOG_WRITE(ESP_LOG_DEBUG,   D, tag, format, ##__VA_ARGS__); break; \
                case ESP_LOG_VERBOSE: DEVICE_LOG_WRITE(ESP_LOG_VERBOSE, V, tag, format, ##__VA_ARGS__); break; \
                default:              DEVICE_LOG_WRITE(ESP_LOG_INFO,    I, tag, format, ##__VA_ARGS__); break; \
            } \
        } while (0)

    /** @brief Erro: operação que falhou e cujo efeito se perde. Equivale a ESP_LOGE. */
    #define DEVICE_LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
    /** @brief Aviso: situação inesperada da qual o nó se recupera sozinho. Equivale a ESP_LOGW. */
    #define DEVICE_LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
    /** @brief Informação sobre a operação normal. Equivale a ESP_LOGI. */
    #define DEVICE_LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
    /** @brief Depuração: exige LOG_LOCAL_LEVEL >= 4. Equivale a ESP_LOGD. */
    #define DEVICE_LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
    /** @brief Detalhamento máximo: exige LOG_LOCAL_LEVEL >= 5. Equivale a ESP_LOGV. */
    #define DEVICE_LOGV(tag, format, ...) ESP_LOGV(tag, format, ##__VA_ARGS__)
#endif
