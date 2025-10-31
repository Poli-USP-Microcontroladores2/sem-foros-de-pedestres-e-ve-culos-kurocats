#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

// Define parâmetros para as threads concorrentes
#define PRIORITY 5
#define STACK_SIZE 1024

// Modo noturno inativo/ativo
int noturno = 0;

// Define o módulo de logging
LOG_MODULE_REGISTER(semaforo_veiculos, LOG_LEVEL_INF);

// Definição do mutex para controle das threads do carro
K_MUTEX_DEFINE(carros_mutex);

// --- Configuração de LEDs via DeviceTree ---
#define LED_VERDE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO, gpios);

//Define as funções das threads de controle de cada uma das cores do semáforo

void pisca_verde(){
    while(1){
        k_mutex_lock(&carros_mutex, K_FOREVER);

        if(!gpio_pin_get_dt(&ledVerde)){
            gpio_pin_set_dt(&ledVerde, 1);
        }

        LOG_INF("Verde aceso!\n");

        k_msleep(3000);

        gpio_pin_toggle_dt(&ledVerde);

        k_mutex_unlock(&carros_mutex);
    }
}

void pisca_amarelo(){
    while(1){
        k_mutex_lock(&carros_mutex, K_FOREVER);

        if(!gpio_pin_get_dt(&ledVerde)){
            gpio_pin_set_dt(&ledVerde, 1);
        }
        if(!gpio_pin_get_dt(&ledVermelho)){
            gpio_pin_set_dt(&ledVermelho, 1);
        }

        LOG_INF("Amarelo aceso!\n");
        
        k_msleep(1000);

        gpio_pin_toggle_dt(&ledVerde);
        gpio_pin_toggle_dt(&ledVermelho);

        k_mutex_unlock(&carros_mutex);
    }
}

void pisca_vermelho(){
    while(1){
        k_mutex_lock(&carros_mutex, K_FOREVER);

        if(!gpio_pin_get_dt(&ledVermelho)){
            gpio_pin_set_dt(&ledVermelho, 1);
        }

        LOG_INF("Vermelho aceso!\n");

        k_msleep(4000);
        
        gpio_pin_toggle_dt(&ledVermelho);

        k_mutex_unlock(&carros_mutex);
    }
}

// Define as threads de controle de cada uma das cores do semáforo (verde, vermelho e amarelo, respectivamente)
K_THREAD_DEFINE(luz_verde, STACK_SIZE, pisca_verde, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(luz_amarela, STACK_SIZE, pisca_amarelo, NULL, NULL, NULL, PRIORITY, 0, 10);
K_THREAD_DEFINE(luz_vermelha, STACK_SIZE, pisca_vermelho, NULL, NULL, NULL, PRIORITY, 0, 10);

void main(void)
{
    // Verifica se os LEDs estão prontos para uso
    if (!device_is_ready(ledVerde.port)) {
        LOG_ERR("Led verde não está pronto para inicialização");
        return;
    }

    if (!device_is_ready(ledVermelho.port)) {
        LOG_ERR("Led vermelho não está pronto para inicialização");
        return;
    }

    // Ativa os LEDS como output inativo (ativa os LEDS em estado desligado)
    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);

    // Condicional para se o modo noturno está ativado
    if(noturno){
        LOG_INF("Modo noturno ativo... zzzzzzzzzzzz\n");
        while(1){
            k_mutex_lock(&carros_mutex, K_FOREVER);

            if(!gpio_pin_get_dt(&ledVerde)){
                gpio_pin_set_dt(&ledVerde, 1);
            }
            if(!gpio_pin_get_dt(&ledVermelho)){
                gpio_pin_set_dt(&ledVermelho, 1);
            }

            LOG_INF("Amarelo aceso!\n");
            
            k_msleep(1000);

            gpio_pin_toggle_dt(&ledVerde);
            gpio_pin_toggle_dt(&ledVermelho);

            LOG_INF("Amarelo apagado!\n");

            k_msleep(1000);

        }
    }
}
