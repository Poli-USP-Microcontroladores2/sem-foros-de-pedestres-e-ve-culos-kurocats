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

// Criação do Mutex para pedestres 

K_MUTEX_DEFINE(pedestres_mutex);

//Configuração do LED =========================================================
#define LED_VERDE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO, gpios);

//=============================================================================

//Define as funções das threads de controle de cada uma das cores do semáforo

void pisca_green(){
    while(1){
        k_mutex_lock(&pedestres_mutex, K_FOREVER);

        if(!gpio_pin_get_dt(&ledVerde)){
            gpio_pin_set_dt(&ledVerde, 1);
        }

        printk("Verde aceso!\n");

        k_msleep(4000);

        gpio_pin_toggle_dt(&ledVerde);

        k_mutex_unlock(&pedestres_mutex);
    }
}

void pisca_red(){
    while(1){
        k_mutex_lock(&pedestres_mutex, K_FOREVER);

        if(!gpio_pin_get_dt(&ledVermelho)){
            gpio_pin_set_dt(&ledVermelho, 1);
        }

        printk("Vermelho aceso!\n");

        k_msleep(4000);
        
        gpio_pin_toggle_dt(&ledVermelho);

        k_mutex_unlock(&pedestres_mutex);
    }
}

// Define as threads de controle de cada uma das cores do semáforo (verde, vermelho e amarelo, respectivamente)
K_THREAD_DEFINE(luz_verde, STACK_SIZE, pisca_green, NULL, NULL, NULL, PRIORITY, 0, 10);
K_THREAD_DEFINE(luz_vermelha, STACK_SIZE, pisca_red, NULL, NULL, NULL, PRIORITY, 0, 0);

void main(void)
{

    // Verifica se os LEDs estão prontos para uso
    if (!device_is_ready(ledVerde.port)) {
        printk("Led verde não está pronto para inicialização");
        return;
    }

    if (!device_is_ready(ledVermelho.port)) {
        printk("Led vermelho não está pronto para inicialização");
        return;
    }

    // Ativa os LEDS como output inativo (ativa os LEDS em estado desligado)
    gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_INACTIVE);

    if (noturno == 1){
        printk("Modo noturno ativo!\n");
    
        while(1){
            // Loop principal vazio, as threads cuidam do funcionamento dos LED
            // Caso o modo noturno esteja ativo, pisca o LED vermelho a cada 1 segundo durante a duração da thread do LED vermelho 
                if(gpio_pin_get_dt(&ledVermelho) == 1){
                    gpio_pin_toggle_dt(&ledVermelho);
                    k_msleep(1000);
                    gpio_pin_toggle_dt(&ledVermelho);
                    k_msleep(1000);
                } 

        }

    }

}