/*!
    \file  main.c
    \brief led spark with systick, USART print and key example

    \version 2017-06-06, V1.0.0, firmware for GD32F3x0
    \version 2019-06-01, V2.0.0, firmware for GD32F3x0
*/

/*
    Copyright (c) 2019, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f3x0.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

/* output test pin 1 */
#define LED1_PIN            GPIO_PIN_2
#define LED1_GPIO_PORT      GPIOB
#define LED1_GPIO_CLK       RCU_GPIOB

/* ooutput test pin 2*/
#define LED2_GPIO_PORT      GPIOB
#define LED2_PIN            GPIO_PIN_9
#define LED2_GPIO_CLK       RCU_GPIOB

/* input test pin */
#define SENSOR1_GPIO_Port   GPIOA
#define SENSOR1_Pin         GPIO_PIN_4

void init_led()
{
    /* enable the led clock */
    rcu_periph_clock_enable(LED1_GPIO_CLK);
    /* configure led GPIO port */
    gpio_mode_set(LED1_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_PIN);
    gpio_output_options_set(LED1_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            LED1_PIN);
    GPIO_BC(LED1_GPIO_PORT) = LED1_PIN; /* clear port / set to 0*/

    rcu_periph_clock_enable(LED2_GPIO_CLK);
    gpio_mode_set(LED2_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED2_PIN);
    gpio_output_options_set(LED2_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            LED2_PIN);
    GPIO_BC(LED2_GPIO_PORT) = LED2_PIN; /* clear port / set to 0*/

    //GPIOA already turned on for this sensor pin
    rcu_periph_clock_enable(RCU_GPIOA);
    /* configure GPIO port */
    rcu_periph_clock_enable(RCU_CFGCMP);
    gpio_mode_set(SENSOR1_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                  SENSOR1_Pin);
}

void toggle_led()
{
    GPIO_OCTL(LED1_GPIO_PORT) ^= LED1_PIN;
}

//Use USART0 on PA9, PA10 for connection on main board
#define EVAL_COM1 USART0
#define EVAL_COM1_CLK RCU_USART0

#define EVAL_COM1_TX_PIN GPIO_PIN_9
#define EVAL_COM1_RX_PIN GPIO_PIN_10

#define EVAL_COM_GPIO_PORT GPIOA
#define EVAL_COM_GPIO_CLK RCU_GPIOA
#define EVAL_COM_AF GPIO_AF_1

void init_uart()
{
    rcu_periph_clock_enable(EVAL_COM_GPIO_CLK);
    /* enable USART clock */
    rcu_periph_clock_enable(EVAL_COM1_CLK);

    /* connect port to USARTx_Tx */
    gpio_af_set(EVAL_COM_GPIO_PORT, EVAL_COM_AF, EVAL_COM1_TX_PIN);

    /* connect port to USARTx_Rx */
    gpio_af_set(EVAL_COM_GPIO_PORT, EVAL_COM_AF, EVAL_COM1_RX_PIN);

    /* configure USART Tx as alternate function push-pull */
    gpio_mode_set(EVAL_COM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                  EVAL_COM1_TX_PIN);
    gpio_output_options_set(EVAL_COM_GPIO_PORT, GPIO_OTYPE_PP,
                            GPIO_OSPEED_10MHZ, EVAL_COM1_TX_PIN);

    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(EVAL_COM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                  EVAL_COM1_RX_PIN);
    gpio_output_options_set(EVAL_COM_GPIO_PORT, GPIO_OTYPE_PP,
                            GPIO_OSPEED_10MHZ, EVAL_COM1_RX_PIN);

    /* USART configure */
    usart_deinit(EVAL_COM1);
    /* 8N1 @ 115200 baud (standard) */
    usart_baudrate_set(EVAL_COM1, 115200U);
    usart_parity_config(EVAL_COM1, USART_PM_NONE);
    usart_word_length_set(EVAL_COM1, USART_WL_8BIT);
    usart_stop_bit_set(EVAL_COM1, USART_STB_1BIT);
    usart_transmit_config(EVAL_COM1, USART_TRANSMIT_ENABLE);
    usart_receive_config(EVAL_COM1, USART_RECEIVE_ENABLE);
    usart_enable(EVAL_COM1);
}

void print_str(const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        usart_data_transmit(EVAL_COM1, str[i]);
        //wait for transmission buffer empty (TBE) before sending next data
        while (RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE))
        {
        }
    }
}

#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
/* pipe for printf to USART peripheral from GCC C library */
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    for (int i = 0; i < len; i++)
    {
        usart_data_transmit(EVAL_COM1, data[i]);
        while (RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE))
        {
        }
    }
    return len;
}

int main(void)
{
    systick_config();
    init_led();
    init_uart();

    while (1)
    {
        toggle_led();
        //leightweight
        print_str("Test string for UART! Blinky blinky\n");
        //uses large printf() implementation
        //printf("Testing printf() function\n");
        delay_1ms(1000);
        /*if (gpio_input_bit_get(SENSOR1_GPIO_Port, SENSOR1_Pin) == RESET) {
			print_str("PA4 = 0\n");
			gpio_bit_set(LED2_GPIO_PORT, LED2_PIN);
		} else {
			print_str("PA4 = 1\n");
			gpio_bit_reset(LED2_GPIO_PORT, LED2_PIN);
		}*/
    }
    return 0; //never reached but for safety
}
