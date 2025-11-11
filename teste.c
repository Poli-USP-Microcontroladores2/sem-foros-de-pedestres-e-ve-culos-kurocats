#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

// Define o módulo de logging
LOG_MODULE_REGISTER(semaforo_veiculos, LOG_LEVEL_INF);

//Porta para sincronização
#define sync_pin 1
#define PORTA_SYNC DT_NODELABEL(gpioa)
const struct device *sync = DEVICE_DT_GET(PORTA_SYNC);

//Define parâmetros de interrupção
#define INTERRUPT_PRIORITY 0
#define input_pin 12
#define PORTA_INPUT DT_NODELABEL(gpioa)
const struct device *input = DEVICE_DT_GET(PORTA_INPUT);
static struct gpio_callback input_callback;

void input_loop(){
    LOG_INF("Entrou no loop\n");
}


void main(void){
    gpio_pin_configure(sync, sync_pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(input, input_pin, GPIO_INPUT | GPIO_PULL_UP);

    //Ativa a interrupção para a ativação do botão do semáforo
    gpio_pin_interrupt_configure(input, input_pin, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&input_callback, input_loop, BIT(input_pin));
    gpio_add_callback(input, &input_callback);

    while(1){
        gpio_pin_set(sync, sync_pin, 1);
        gpio_pin_set(sync, sync_pin, 0);
        LOG_INF("Mandou\n");
        k_msleep(1000);
    }
}