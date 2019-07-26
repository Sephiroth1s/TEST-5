/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/
#include "vsf.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

//void uart_config(void)
//{
//#if defined(IOTKIT_SECURE_UART0)
//    IOTKIT_SECURE_UART0->CTRL = 0;         /* Disable UART when changing configuration */
//    IOTKIT_SECURE_UART0->BAUDDIV = 651;    /* 50MHz / 38400 = 651 */
//    IOTKIT_SECURE_UART0->CTRL = IOTKIT_UART_CTRL_TXEN_Msk| IOTKIT_UART_CTRL_RXEN_Msk;  
//     /* Update CTRL register to new value */
//#elif defined(CMSDK_UART0)

//    CMSDK_UART0->CTRL = 0;         /* Disable UART when changing configuration */
//    CMSDK_UART0->BAUDDIV = 651;    /* 25MHz / 38400 = 651 */
//    CMSDK_UART0->CTRL = CMSDK_UART_CTRL_TXEN_Msk|CMSDK_UART_CTRL_RXEN_Msk;  
//     /* Update CTRL register to new value */
//#else
//#error No defined USART
//#endif
//}

//void vsf_stdout_init(void)
//{
//    uart_config();
//}

//void vsf_stdin_init(void)
//{

//}

//char vsf_stdin_getchar(void)
//{
//#if defined(IOTKIT_SECURE_UART0)
//    while(!(IOTKIT_UART0->STATE & IOTKIT_UART_STATE_RXBF_Msk));
//    return (char)(IOTKIT_SECURE_UART0->DATA);
//#elif defined(CMSDK_UART0)
//    while(!(CMSDK_UART0->STATE & CMSDK_UART_STATE_RXBF_Msk));
//    return (char)(CMSDK_UART0->DATA);
//#else
//#error No defined USART
//#endif
//}

//int vsf_stdout_putchar(char txchar)
//{
//    if (txchar == 10) vsf_stdout_putchar((char) 13);

//#if defined(IOTKIT_SECURE_UART0)
//    while(IOTKIT_SECURE_UART0->STATE & IOTKIT_UART_STATE_TXBF_Msk);
//    IOTKIT_SECURE_UART0->DATA = (uint32_t)txchar;
//    return (int) txchar;
//#elif defined(CMSDK_UART0)
//    while(CMSDK_UART0->STATE & CMSDK_UART_STATE_TXBF_Msk);
//    CMSDK_UART0->DATA = (uint32_t)txchar;
//    (*(volatile uint32_t *)0x41000000) = txchar;
//    return (int) txchar;
//#else
//#error No defined USART
//#endif
//}
UART_HandleTypeDef huart1;

void vsf_stdin_init(void)
{
}

void vsf_stdout_init(void)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

char vsf_stdin_getchar(void)
{
    char chr;
    while (HAL_OK != HAL_UART_Receive(&huart1, (uint8_t *)&chr, 1, 100));
    return chr;
}

int vsf_stdout_putchar(char txchar)
{
    if (txchar == 10) vsf_stdout_putchar((char) 13);
    while (HAL_OK != HAL_UART_Transmit(&huart1, (uint8_t *)&txchar, 1, 100));
    return (int)txchar;
}

bool serial_out(uint8_t chByte)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE) != RESET) {
        huart1.Instance->DR = chByte;
        return true;
    } else {
        return false;
    }
}

bool serial_in(uint8_t *pchByte)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) {
        *pchByte = huart1.Instance->DR;
        return true;
    } else {
        return false;
    }
}
