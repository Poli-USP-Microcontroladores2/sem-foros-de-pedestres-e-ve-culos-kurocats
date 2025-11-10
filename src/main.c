#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

//Define parâmetros de interrupção
#define INTERRUPT_PRIORITY 0
#define botao_pin 12
#define PORTA_BOTAO DT_NODELABEL(gpioa)
const struct device *botao = DEVICE_DT_GET(PORTA_BOTAO);
static struct gpio_callback botao_callback;

// Define parâmetros para as threads concorrentes
#define PRIORITY 5
#define STACK_SIZE 1024

void pisca_verde(void);
void pisca_amarelo(void);
void pisca_vermelho(void);

// Define as threads de controle de cada uma das cores do semáforo (verde, vermelho e amarelo, respectivamente)
K_THREAD_DEFINE(luz_verde, STACK_SIZE, pisca_verde, NULL, NULL, NULL, PRIORITY, 0, 20);
K_THREAD_DEFINE(luz_amarela, STACK_SIZE, pisca_amarelo, NULL, NULL, NULL, PRIORITY, 0, 30);
K_THREAD_DEFINE(luz_vermelha, STACK_SIZE, pisca_vermelho, NULL, NULL, NULL, PRIORITY, 0, 30);

//Porta para sincronização
#define sync_pin 4
#define PORTA_SYNC DT_NODELABEL(gpioa)
const struct device *sync = DEVICE_DT_GET(PORTA_SYNC);

// Modo noturno inativo/ativo
int noturno = 0;
#define noturno_pin 5
#define PORTA_NOTURNO DT_NODELABEL(gpioa)
const struct device *nocturne = DEVICE_DT_GET(PORTA_NOTURNO);

// Define o módulo de logging
LOG_MODULE_REGISTER(semaforo_veiculos, LOG_LEVEL_INF);

// Definição do mutex para controle das threads do carro
K_MUTEX_DEFINE(carros_mutex);

// --- Configuração de LEDs via DeviceTree ---
#define LED_VERDE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO, gpios);

// Definir variavel digital para controle dos LEDs
int estado_semaforo = 0;

//Define as funções das threads de controle de cada uma das cores do semáforo

void pisca_verde(){
    while(1){
        LOG_INF("Verde iniciou!\n");
        gpio_pin_set(sync, sync_pin, 1);

        gpio_pin_set_dt(&ledVermelho, 0);
        gpio_pin_set_dt(&ledVerde, 1);

        LOG_INF("Verde aceso!\n");

        k_msleep(3000);

        gpio_pin_toggle_dt(&ledVerde);
        k_thread_resume(luz_amarela);
        estado_semaforo = 1;
        k_thread_suspend(luz_verde);
    }
}

void pisca_amarelo(){
    while(1){
        LOG_INF("Amarelo iniciou!\n");
        gpio_pin_set_dt(&ledVerde, 1);

        gpio_pin_set_dt(&ledVermelho, 1);

        LOG_INF("Amarelo aceso!\n");
        
        k_msleep(1000);

        gpio_pin_toggle_dt(&ledVerde);
        gpio_pin_toggle_dt(&ledVermelho);
        k_thread_resume(luz_vermelha);
        estado_semaforo = 2;
        k_thread_suspend(luz_amarela);
    }
}

void pisca_vermelho(){
    while(1){
        LOG_INF("Vermelho iniciou!\n");
        gpio_pin_set(sync, sync_pin, 0);

        gpio_pin_set_dt(&ledVerde, 0);
        gpio_pin_set_dt(&ledVermelho, 1);

        LOG_INF("Vermelho aceso!\n");

        k_msleep(4000);

        gpio_pin_toggle_dt(&ledVermelho);
        k_thread_resume(luz_verde);
        estado_semaforo = 0;
        k_thread_suspend(luz_vermelha);
    }
}

void botao_apertado(){
    LOG_INF("Botão apertado!\n");
    k_thread_suspend(luz_verde); 
    k_thread_suspend(luz_amarela);
    k_thread_resume(luz_vermelha);
    estado_semaforo = 2;
}

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

    // Verifica se a sincronização está pronta para uso
    if (!device_is_ready(sync)) {
        LOG_ERR("Os pinos de sincronização não está pronto para inicialização");
        return;
    }

    // Verifica se o modo noturno está pronta para uso
    if (!device_is_ready(nocturne)) {
        LOG_ERR("Os pinos do modo noturno não está pronto para inicialização");
        return;
    }

    // Verifica se a porta do botão está pronta
    if (!device_is_ready(botao)) {
    LOG_ERR("Porta do botão não está pronta");
    return;
    }

    //Ativa os pinos de sincronização e modo noturno como output
    gpio_pin_configure(sync, sync_pin, GPIO_OUTPUT);
    gpio_pin_configure(nocturne, noturno_pin, GPIO_OUTPUT);
    gpio_pin_set(sync, sync_pin, 0);
    gpio_pin_set(nocturne, noturno_pin, 0);

    // Ativa o pino do botão como input com pull-up
    gpio_pin_configure(botao, botao_pin, GPIO_INPUT | GPIO_PULL_UP);

    //Ativa a interrupção para a ativação do botão do semáforo
    gpio_pin_interrupt_configure(botao, botao_pin, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&botao_callback, botao_apertado, BIT(botao_pin));
    gpio_add_callback(botao, &botao_callback);

    k_thread_suspend(luz_amarela);
    k_thread_suspend(luz_vermelha);

    // Condicional para se o modo noturno está ativado
    if(noturno){
        gpio_pin_set(nocturne, noturno_pin, 1);
        LOG_INF("Modo noturno ativo... zzzzzzzzzzzz\n");
        while(1){
            k_mutex_lock(&carros_mutex, K_FOREVER);

            gpio_pin_set_dt(&ledVerde, 1);
            gpio_pin_set_dt(&ledVermelho, 1);

            LOG_INF("Amarelo aceso!\n");
            
            k_msleep(1000);

            gpio_pin_toggle_dt(&ledVerde);
            gpio_pin_toggle_dt(&ledVermelho);

            LOG_INF("Amarelo apagado!\n");

            k_msleep(1000);

        }
    }
}
