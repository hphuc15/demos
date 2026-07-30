/**
  ******************************************************************************
  * @file    retarget.c
  * @brief   Retargets printf/_write to UART for debug logging output
  ******************************************************************************
  */
#include "main.h"
#include "usart.h"

int _write(int file, char *ptr, int len){
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}