#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

// Define parâmetros para as threads concorrentes
#define PRIORITY 5
#define STACK_SIZE 1024
#define INTERRUPT_PRIORITY 2

//Parâmetros para interrupção de sincronização
#define SYNC_PRIORITY 1
#define sync_pin 4
#define PORTA_SYNC DT_NODELABEL(gpioa)
const struct device *sync = DEVICE_DT_GET(PORTA_SYNC);
static struct gpio_callback sync_callback;

// Modo noturno inativo/ativo
#define NOCTURNE_PRIORITY 1
#define noturno_pin 1
#define PORTA_NOTURNO DT_NODELABEL(gpioa)
const struct device *nocturne = DEVICE_DT_GET(PORTA_NOTURNO);
static struct gpio_callback nocturne_callback;

//Parâmetros para a interrupção do botão
#define BUTTON_PRIORITY 0
#define botao_pin 12
#define PORTA_BOTAO DT_NODELABEL(gpioa)
const struct device *botao = DEVICE_DT_GET(PORTA_BOTAO);
static struct gpio_callback botao_callback;


//Configuração do LED =========================================================
#define LED_VERDE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO, gpios);

//=============================================================================

void pisca_green();
void pisca_red();
void modo_noturno();

// Define as threads de controle de cada uma das cores do semáforo (verde, vermelho e amarelo, respectivamente)
K_THREAD_DEFINE(luz_verde, STACK_SIZE, pisca_green, NULL, NULL, NULL, PRIORITY, 0, 10);
K_THREAD_DEFINE(luz_vermelha, STACK_SIZE, pisca_red, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_noturno, STACK_SIZE, modo_noturno, NULL, NULL, NULL, NOCTURNE_PRIORITY, 0, 0);

//Define as funções das threads de controle de cada uma das cores do semáforo

void pisca_green(){
    while(1){
        gpio_pin_set_dt(&ledVermelho, 0);
        gpio_pin_set_dt(&ledVerde, 1);

        printk("Verde aceso!\n");

        k_msleep(4000);

        gpio_pin_toggle_dt(&ledVerde);

        k_thread_resume(luz_vermelha);
        k_thread_suspend(luz_verde);
    }
}

void pisca_red(){
    while(1){
        gpio_pin_set_dt(&ledVerde, 0);
        gpio_pin_set_dt(&ledVermelho, 1);

        printk("Vermelho aceso!\n");

        k_msleep(4000);
        
        gpio_pin_toggle_dt(&ledVermelho);

        k_thread_resume(luz_verde);
        k_thread_suspend(luz_vermelha);
    }
}


void botao_apertado(){
    printk("Botão apertado!\n");
    k_thread_suspend(luz_vermelha); 
    k_thread_resume(luz_verde);

}


void modo_noturno(){
    printk("Modo noturno!\n");
    gpio_pin_set_dt(&ledVerde, 0);
    while(1){
        gpio_pin_set_dt(&ledVermelho, 1);
        k_msleep(1000);
        gpio_pin_set_dt(&ledVermelho, 0);
        k_msleep(1000);
    }
}

void interrupt_noturno(){
    k_thread_suspend(luz_vermelha);
    k_thread_suspend(luz_verde);
    k_thread_resume(thread_noturno);
}

void sincronizacao(){

    printk("Sincronização!\n");
    k_thread_suspend(luz_verde);
    k_thread_resume(luz_vermelha);

}

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




// Verifica se a sincronização está pronta para uso
    if (!device_is_ready(sync)) {
        printk("Pino de sincronização não está pronto para inicialização");
        return;
    }

// Ativa o pino da sincronização como input com pull-up
    gpio_pin_configure(sync, sync_pin, GPIO_INPUT | GPIO_PULL_UP);

//Ativa a interrupção para a sincronização dos semáforos
    gpio_pin_interrupt_configure(sync, sync_pin, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&sync_callback, sincronizacao, BIT(sync_pin));
    gpio_add_callback(sync, &sync_callback);



// Verifica se o modo noturno está pronta para uso
    if (!device_is_ready(nocturne)) {
        printk("Pino do modo noturno não está pronto para inicialização");
        return;
    }

// Ativa o pino de modo noturno como input com pull-up
    gpio_pin_configure(nocturne, noturno_pin, GPIO_INPUT | GPIO_PULL_UP);

//Ativa a interrupção para a sincronização do modo noturno
    gpio_pin_interrupt_configure(nocturne, noturno_pin, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&nocturne_callback, interrupt_noturno, BIT(noturno_pin));
    gpio_add_callback(nocturne, &nocturne_callback);

    // Verifica se a porta do botão está pronta
    if (!device_is_ready(botao)) {
        printk("Porta do botão não está pronta");
        return;
    }

    // Ativa o pino do botão como input com pull-up
    gpio_pin_configure(botao, botao_pin, GPIO_INPUT | GPIO_PULL_UP);

    //Ativa a interrupção para a ativação do botão do semáforo
    gpio_pin_interrupt_configure(botao, botao_pin, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&botao_callback, botao_apertado, BIT(botao_pin));
    gpio_add_callback(botao, &botao_callback);

    k_thread_suspend(thread_noturno);

}