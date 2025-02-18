
# Atividade Prática: Controle de LEDs RGB e Display SSD1306 com Joystick no RP2040

## Link de descrição do vídeo de funcionamento

## Objetivos

- Compreender o funcionamento do conversor analógico-digital (ADC) no RP2040.
- Utilizar o PWM para controlar a intensidade de dois LEDs RGB com base nos valores do joystick.
- Representar a posição do joystick no display SSD1306 por meio de um quadrado móvel.
- Aplicar o protocolo de comunicação I2C na integração com o display.

## Descrição do Projeto

O joystick fornece valores analógicos correspondentes aos eixos X e Y, que serão usados para controlar os LEDs e a movimentação no display.

### LEDs RGB
- **LED Azul**: Controlado pelo eixo Y do joystick.
  - Quando o joystick está na posição central (valor 2048), o LED está apagado.
  - Movimento para cima (valores menores) ou para baixo (valores maiores) aumenta o brilho, atingindo intensidade máxima em 0 e 4095.
  
- **LED Vermelho**: Controlado pelo eixo X do joystick.
  - Padrão similar ao LED Azul.
  - Movimento para a esquerda ou direita altera o brilho do LED.

**Controle via PWM**: Para uma variação suave de intensidade dos LEDs.

### Display SSD1306
- Exibição de um quadrado de 8x8 pixels, movendo-se proporcionalmente aos valores do joystick.

### Funcionalidades do Botão do Joystick
- **LED Verde**: Alterna entre os estados (aceso/desligado) a cada pressionamento.
- **Borda do Display**: Modifica o estilo da borda a cada acionamento.

### Funcionalidade do Botão A
- **Ativar/Desativar LEDs PWM**: A cada pressionamento, alterna entre ativar e desativar o controle PWM dos LEDs.

## Componentes
- **LED RGB**
- **Botão do Joystick**
- **Joystick**
- **Botão A**
- **Display SSD1306**

## Como Executar

1. Conecte à placa BitDogLab no computador via cabo USB.
2. Compile o código.
3. Faça o BOOTSEL na placa e clique em run
4. Pressione ou movimente o JoyStick para ver o funcionamento do Led Azul e Vermelho e do display, ou para alternar o estado do Led Verde
5. Pressione o botão A para alternar o estados dos leds PWM
