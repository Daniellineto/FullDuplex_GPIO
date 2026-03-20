/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

// Armazena o byte que está sendo montado bit a bit
volatile uint8_t byte_recebido = 0;

// Conta quantos bits já chegaram (de 0 a 7)
volatile uint8_t contador_bits = 0;

// Sinaliza para o main que um novo byte foi montado
volatile uint8_t mensagem_pronta = 0;

// Flag de confirmacao de handshake (recebeu 0xAA)
volatile uint8_t ack_recebido = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Função que envia 8 bits usando Bit-Banging síncrono
void Transmitir_Byte_Sincrono(uint8_t dado) {
    for(int i = 0; i < 8; i++) {
        // 1. Aplica o bit atual no pino de Dados (PC1)
        if (dado & (1 << i)) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
        }
        HAL_Delay(1); // Tempo para estabilizar o sinal (Setup Time)

        // 2. Cria o pulso de Clock (PC0) para o receptor ler o bit
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_Delay(1);

        // 3. Desce o sinal de Clock para finalizar o ciclo do bit
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
}

// Função de espera que permite processar dados recebidos sem travar o código
void Delay_e_Escuta(uint32_t tempo_ms) {
	uint32_t inicio = HAL_GetTick();

	// Delay nao-bloqueante baseado no SysTick
	while((HAL_GetTick() - inicio) < tempo_ms) {

		// Se a interrupção avisar que chegou um dado, imprime no console
        if(mensagem_pronta == 1) {
            // Remove o 8º bit (máscara 0x7F) para mostrar o valor original (0-100)
            uint8_t valor_final = byte_recebido & 0x7F;
            printf("Recebido do outro modulo: %d\r\n", valor_final);

            mensagem_pronta = 0; // Reseta a flag para a próxima recepção
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  printf("\r\n--- INICIANDO TRANSMISSAO FULL-DUPLEX COM HANDSHAKE ---\r\n");

	  // Inicia o envio do array de 0 a 100
	  for(int i = 0; i <= 100; i++) {

		  // Prepara o dado garantindo o padrão de 8 bits (bit 7 sempre em 1)
		  uint8_t valor_para_enviar = (uint8_t)i | 0x80;

		  ack_recebido = 0; // Limpa a flag para esperar a confirmação deste novo dado
		  Transmitir_Byte_Sincrono(valor_para_enviar);

		  // Timeout de 1000ms aguardando o ACK
		  uint32_t tempo_inicio = HAL_GetTick();

		  while(ack_recebido == 0 && (HAL_GetTick() - tempo_inicio) < 1000) {

			  // Processa recebimentos concorrentes para nao travar o full-duplex
			  if(mensagem_pronta == 1) {
				  uint8_t valor_final = byte_recebido & 0x7F; // Remove máscara para exibir 0-100
				  printf(" [->] Recebido da outra placa: %d\r\n", valor_final);
				  mensagem_pronta = 0;

				  // Envia o ACK (0xAA) imediatamente para confirmar que recebi o dado DELA
				  Transmitir_Byte_Sincrono(0xAA);
			  }
		  }

		  // Verifica se a transmissão foi bem sucedida ou se houve perda de conexão
		  if (ack_recebido == 1) {
			  printf(" [<-] Meu dado %d foi enviado com sucesso (ACK OK)\r\n", i);
		  } else {
			  printf(" [XX] Falha ao enviar o dado %d (Timeout sem ACK)\r\n", i);
		  }

		  // Pequena pausa para estabilizar a comunicação e evitar conflitos nos fios
		  Delay_e_Escuta(50);
	  }

	  // --- MODO DE ESPERA (5 segundos) ---
	  // Após enviar os 100 números, a placa apenas escuta e responde ACKs
	  uint32_t espera_longa = HAL_GetTick();
	  while((HAL_GetTick() - espera_longa) < 5000) {
		  if(mensagem_pronta == 1) {
			  uint8_t valor_final = byte_recebido & 0x7F;
			  printf(" [->] Recebido da outra placa: %d\r\n", valor_final);
			  mensagem_pronta = 0;
			  Transmitir_Byte_Sincrono(0xAA); // Responde o ACK para a outra placa
		  }
	  }

	  /* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t ultimo_tempo = 0;
    uint32_t tempo_atual = HAL_GetTick();

    if(GPIO_Pin == GPIO_PIN_2) { // Monitorando o Clock de entrada (PC2)

        if ((tempo_atual - ultimo_tempo) > 50) {
            contador_bits = 0;
            byte_recebido = 0;
        }
        ultimo_tempo = tempo_atual;

        GPIO_PinState estado_dado = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3);

        if(estado_dado == GPIO_PIN_SET) {
            byte_recebido |= (1 << contador_bits);
        } else {
            byte_recebido &= ~(1 << contador_bits);
        }

        contador_bits++;

        if(contador_bits >= 8) {
        	// Verifica se o byte eh o sinal de ACK ou um dado de telemetria
            if (byte_recebido == 0xAA) {
                ack_recebido = 1; // Recebeu o "HandShake"
            } else {
                mensagem_pronta = 1; // Recebeu um número novo
            }
            contador_bits = 0;
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
