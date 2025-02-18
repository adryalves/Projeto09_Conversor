#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/time.h"    //biblioteca para gerenciamento de tempo
#include "hardware/irq.h" //biblioteca para gerenciamento de interrupções
#include "hardware/pwm.h" //biblioteca para controlar o hardware de PWM

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 26 // GPIO para eixo X
#define JOYSTICK_Y_PIN 27 // GPIO para eixo Y
#define JOYSTICK_PB 22    // GPIO para botão do Joystick
#define Botao_A 5         // GPIO para botão A

#define LEDRed 13 // pino do LED conectado a GPIO como PWM
#define LedBlue 12
#define LedGreen 11

#include "pico/bootrom.h"
#define botaoB 6

// Variável global para o estado do PWM
static bool pwm_enabled = true;

// Variável para debouncing
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// tratamento da interrupção do PWM
void wrapHandler()
{
    static int fade = 0;     // Nível de iluminação
    static bool rise = true; // Flag para elevar ou reduzir a iluminação

    // Reseta o flag de interrupção para ambos os slices
    pwm_clear_irq(pwm_gpio_to_slice_num(LedBlue)); // Reseta o flag de interrupção para o LED Azul
    pwm_clear_irq(pwm_gpio_to_slice_num(LEDRed));  // Reseta o flag de interrupção para o LED Vermelho

    // Define o ciclo ativo (Ton) de forma quadrática para ambos os LEDs
    pwm_set_gpio_level(LedBlue, fade * fade); // LED Azul
    pwm_set_gpio_level(LEDRed, fade * fade);  // LED Vermelho
}

void updateLEDs(uint16_t adc_value_x, uint16_t adc_value_y)
{
    // Mapeia o valor do ADC (0-4095) para o valor de PWM (0-255)
    uint16_t blue_intensity = abs(adc_value_y - 2048) / 8; // Divisão por 8 para ajustar a faixa (aumenta o brilho)
    uint16_t red_intensity = abs(adc_value_x - 2048) / 8;

    // Limita o valor máximo de intensidade para 255
    if (blue_intensity > 255)
        blue_intensity = 255;
    if (red_intensity > 255)
        red_intensity = 255;

    // Ajusta o brilho dos LEDs
    pwm_set_gpio_level(LedBlue, blue_intensity * blue_intensity); // Quadrático para suavizar a transição
    pwm_set_gpio_level(LEDRed, red_intensity * red_intensity);
}

void moveSquare(ssd1306_t *ssd, uint16_t adc_value_x, uint16_t adc_value_y, bool cor)
{
    static int x = 60; // Posição inicial centralizada no eixo X
    static int y = 28; // Posição inicial centralizada no eixo Y

    // Calcula o deslocamento com base nos valores do joystick
    int delta_x = (adc_value_x - 2048) / 512; // Ajuste a sensibilidade conforme necessário
    int delta_y = (adc_value_y - 2048) / 512;

    // Atualiza a posição do quadrado
    x += delta_x;
    y += delta_y;

    // Limita a posição do quadrado dentro dos limites do display
    if (x < 0)
        x = 0;
    if (x > WIDTH - 8)
        x = WIDTH - 8;
    if (y < 0)
        y = 0;
    if (y > HEIGHT - 8)
        y = HEIGHT - 8;

    // Desenha o quadrado na nova posição
    ssd1306_fill(ssd, false);                 // Limpa o display
    ssd1306_rect(ssd, x, y, 8, 8, cor, !cor); // Desenha o quadrado
    ssd1306_send_data(ssd);                   // Atualiza o display
}

void drawBorder(ssd1306_t *ssd, int border_style)
{
    ssd1306_fill(ssd, false); // Limpa o display

    switch (border_style)
    {
    case 0:
        // Sem borda
        break;
    case 1:
        // Borda simples
        ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false);
        break;
    case 2:
        // Borda dupla
        ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false);
        ssd1306_rect(ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false);
        break;
    }

    ssd1306_send_data(ssd); // Atualiza o display
}

void handleJoystickButton(ssd1306_t *ssd, bool *green_led_state, int *border_style)
{
    static bool last_button_state = true; // Assume que o botão está solto inicialmente
    bool current_button_state = gpio_get(JOYSTICK_PB);

    // Verifica se o botão foi pressionado (transição de alto para baixo)
    if (last_button_state && !current_button_state)
    {
        *green_led_state = !(*green_led_state); // Alterna o estado do LED Verde
        gpio_put(LedGreen, *green_led_state);

        // Alterna o estilo da borda
        *border_style = (*border_style + 1) % 3; // 3 estilos de borda diferentes
        drawBorder(ssd, *border_style);
    }

    last_button_state = current_button_state;
}

// Função de interrupção para o botão A
void handleButtonA(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Debouncing: verifica se o tempo desde o último evento é maior que 500 ms
    if (current_time - last_time > 20000) // 500 ms de debouncing
    {
        last_time = current_time;

        // Alterna o estado do PWM
        pwm_enabled = !pwm_enabled;

        if (pwm_enabled)
        {
            // Liga os LEDs com intensidade máxima
            pwm_set_gpio_level(LedBlue, 255 * 255); // Intensidade máxima
            pwm_set_gpio_level(LEDRed, 255 * 255);  // Intensidade máxima
        }
        else
        {
            // Desliga os LEDs
            pwm_set_gpio_level(LedBlue, 0);
            pwm_set_gpio_level(LEDRed, 0);
        }
    }
}

// Configuração do PWM com interrupção
uint pwm_setup_irq()
{
    // Configuração do PWM para o LED Azul
    gpio_set_function(LedBlue, GPIO_FUNC_PWM);          // Habilita o pino GPIO como PWM
    uint sliceNumBlue = pwm_gpio_to_slice_num(LedBlue); // Obtém o canal PWM da GPIO

    // Configuração do PWM para o LED Vermelho
    gpio_set_function(LEDRed, GPIO_FUNC_PWM);         // Habilita o pino GPIO como PWM
    uint sliceNumRed = pwm_gpio_to_slice_num(LEDRed); // Obtém o canal PWM da GPIO

    // Configuração do PWM para o LED Azul
    pwm_clear_irq(sliceNumBlue);             // Reseta o flag de interrupção para o slice
    pwm_set_irq_enabled(sliceNumBlue, true); // Habilita a interrupção de PWM para o slice do LED Azul

    // Configuração do PWM para o LED Vermelho
    pwm_clear_irq(sliceNumRed);             // Reseta o flag de interrupção para o slice
    pwm_set_irq_enabled(sliceNumRed, true); // Habilita a interrupção de PWM para o slice do LED Vermelho

    // Configuração do handler de interrupção
    irq_set_exclusive_handler(PWM_IRQ_WRAP, wrapHandler); // Define o handler de interrupção
    irq_set_enabled(PWM_IRQ_WRAP, true);                  // Habilita a interrupção

    // Configuração do PWM para o LED Azul
    pwm_config configBlue = pwm_get_default_config(); // Obtém a configuração padrão para o PWM
    pwm_config_set_clkdiv(&configBlue, 4.f);          // Define o divisor de clock do PWM
    pwm_init(sliceNumBlue, &configBlue, true);        // Inicializa o PWM com as configurações

    // Configuração do PWM para o LED Vermelho
    pwm_config configRed = pwm_get_default_config(); // Obtém a configuração padrão para o PWM
    pwm_config_set_clkdiv(&configRed, 4.f);          // Define o divisor de clock do PWM
    pwm_init(sliceNumRed, &configRed, true);         // Inicializa o PWM com as configurações

    return sliceNumBlue; // Retorna o slice do LED Azul (ou qualquer um que você preferir)
}

int main()
{
    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);

    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);

    gpio_init(LedGreen);
    gpio_set_dir(LedGreen, GPIO_OUT);

    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    uint16_t adc_value_x;
    uint16_t adc_value_y;
    char str_x[5]; // Buffer para armazenar a string
    char str_y[5]; // Buffer para armazenar a string

    bool cor = true;

    bool green_led_state = false;
    int border_style = 0;

    pwm_setup_irq(); // Configura o PWM com interrupção

    // Configura a interrupção para o botão A
    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, handleButtonA);

    while (true)
    {
        adc_select_input(0); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
        adc_value_x = adc_read();
        adc_select_input(1); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
        adc_value_y = adc_read();
        sprintf(str_x, "%d", adc_value_x); // Converte o inteiro em string
        sprintf(str_y, "%d", adc_value_y); // Converte o inteiro em string

        if (pwm_enabled)
        {
            updateLEDs(adc_value_x, adc_value_y);
        }

        moveSquare(&ssd, adc_value_x, adc_value_y, cor);
        handleJoystickButton(&ssd, &green_led_state, &border_style);

        sleep_ms(100);
    }
}