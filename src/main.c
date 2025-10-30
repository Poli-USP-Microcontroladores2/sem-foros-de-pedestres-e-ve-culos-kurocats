#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

// Define o módulo de logging
LOG_MODULE_REGISTER(semaforo_veiculos, LOG_LEVEL_DBG);

// --- Configuração de LEDs via DeviceTree ---
#define LED_VERDE DT_ALIAS(led0)  // LED verde
#define LED_VERMELHO DT_ALIAS(led2)  // LED vermelho

static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED_VERDE, gpios);
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED_VERMELHO, gpios);

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
   
}
