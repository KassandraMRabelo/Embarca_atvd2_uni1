/**********************************************
 * BIBLIOTECAS NECESSÁRIAS PARA IMPLEMENTAÇÃO
 **********************************************/

#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h" 

/**********************************************
 * DEFINIÇÕES DE HARDWARE E CONFIGURAÇÕES
 **********************************************/

// Configuração dos pinos
#define BUTTON_A_PIN   5   // Botão A conectado no GPIO 5 (LOW = pressionado)
#define BUTTON_B_PIN   6   // Botão B conectado no GPIO 6
#define RED_LED_PIN    13  // LED Vermelho no GPIO 13
#define GREEN_LED_PIN  11  // LED Verde no GPIO 11

/**********************************************
 * VARIÁVEIS GLOBAIS E ESTRUTURAS DE DADOS
 **********************************************/

// Fila para comunicação entre tarefas (armazena comandos para os LEDs)
QueueHandle_t xLedQueue;

// Enumeração que define os possíveis estados dos LEDs
typedef enum {
    LED_OFF,    // Todos os LEDs desligados
    LED_RED,    // LED Vermelho ligado
    LED_GREEN   // LED Verde ligado
} LedCommand;

/**********************************************
 * IMPLEMENTAÇÃO DAS TAREFAS
 **********************************************/

// Tarefa 1: Leitura dos Botões (executa a cada 100ms)
void vTaskButtonRead(void *pvParameters) {
    // Armazena o tempo da última execução para garantir periodicidade
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Lê o estado atual dos botões e converte para comandos
        LedCommand xCommandA = (gpio_get(BUTTON_A_PIN) == 0) ? LED_RED : LED_OFF;
        LedCommand xCommandB = (gpio_get(BUTTON_B_PIN) == 0) ? LED_GREEN : LED_OFF;
        
        // Prioriza o botão pressionado (se ambos forem pressionados, o último comando prevalece)
        if (xCommandA == LED_RED) {
            xQueueSend(xLedQueue, &xCommandA, portMAX_DELAY);
            printf("[Leitura] Botão A pressionado (Vermelho)\n");
        } else if (xCommandB == LED_GREEN) {
            xQueueSend(xLedQueue, &xCommandB, portMAX_DELAY);
            printf("[Leitura] Botão B pressionado (Verde)\n");
        } else {
            xQueueSend(xLedQueue, &xCommandA, portMAX_DELAY); // Envia LED_OFF
            printf("[Leitura] Nenhum botão pressionado\n");
        }
        
        // Aguarda exatamente 100ms desde a última execução
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

// Tarefa 2: Processamento 
void vTaskButtonProcess(void *pvParameters) {
    LedCommand xReceivedCommand;
    while (1) {
        if (xQueueReceive(xLedQueue, &xReceivedCommand, portMAX_DELAY) == pdTRUE) {
            // Exibe o processamento (
            printf("[Processamento] Comando recebido: %s\n", 
                   xReceivedCommand == LED_RED ? "Vermelho" : 
                   xReceivedCommand == LED_GREEN ? "Verde" : "OFF");
            
            // Envia diretamente para o controle 
  
        }
    }
}

// Tarefa 3: Controle dos LEDs (executa ações nos LEDs físicos)
void vTaskLedControl(void *pvParameters) {
    LedCommand xReceivedCommand;
    
    while (1) {
        // Aguarda por um comando na fila 
        if (xQueueReceive(xLedQueue, &xReceivedCommand, portMAX_DELAY) == pdTRUE) {
            // Atualiza o estado dos LEDs conforme o comando recebido
            gpio_put(RED_LED_PIN, (xReceivedCommand == LED_RED) ? 1 : 0);
            gpio_put(GREEN_LED_PIN, (xReceivedCommand == LED_GREEN) ? 1 : 0);
            
            printf("[Controle] LED: %s\n", 
                   xReceivedCommand == LED_RED ? "Vermelho ligado" : 
                   xReceivedCommand == LED_GREEN ? "Verde ligado" : "Todos desligados");
        }
    }
}

/**********************************************
 * CONFIGURAÇÃO INICIAL E MAIN
 **********************************************/

// Função de inicialização do hardware e sistema
void setup() {
    // Inicializa comunicação serial para debug
    stdio_init_all();
    printf("Sistema iniciado (3 tarefas)!\n");

    // Configuração dos pinos dos botões como entrada com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN); // Resistor pull-up interno ativado
    gpio_pull_up(BUTTON_B_PIN);

    // Configuração dos pinos dos LEDs como saída
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

    // Garante que todos os LEDs iniciam desligados
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);

    // Cria a fila de mensagens (capacidade para 3 comandos)
    xLedQueue = xQueueCreate(3, sizeof(LedCommand));

    // Cria as 3 tarefas do sistema com suas prioridades
    xTaskCreate(vTaskButtonRead,   "Leitura Botões",   256, NULL, 1, NULL); // Prioridade mais baixa
    xTaskCreate(vTaskButtonProcess, "Processamento",   256, NULL, 2, NULL); // Prioridade média
    xTaskCreate(vTaskLedControl,   "Controle LEDs",    256, NULL, 3, NULL); // Prioridade mais alta

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();
}

// Função principal (não deverá ser alcançada)
int main() {
    setup();
    while (1) {} // Loop infinito de fallback
}