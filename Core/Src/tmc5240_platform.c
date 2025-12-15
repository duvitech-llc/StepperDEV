/**
  ******************************************************************************
  * @file           : tmc5240_platform.c
  * @brief          : Platform-specific SPI implementation for TMC5240
  ******************************************************************************
  * This file implements the low-level SPI communication functions for the
  * TMC5240 stepper motor driver on STM32L4 platform.
  ******************************************************************************
  */

#include "tmc5240.h"
#include "main.h"

/* External SPI handle - defined in main.c */
extern SPI_HandleTypeDef hspi1;

/* SPI timeout in milliseconds */
#define TMC5240_SPI_TIMEOUT  100

/**
  * @brief  Write data to TMC5240 via SPI
  * @param  address: Register address (will be OR'ed with 0x80 for write)
  * @param  data: Pointer to data buffer to write
  * @param  length: Number of bytes to write
  * @retval 0 on success, -1 on error
  */
int8_t tmc5240_platform_spi_write(uint8_t address, const uint8_t *data, size_t length)
{
    HAL_StatusTypeDef status;
    uint8_t write_addr = address | 0x80;  /* Set write bit */
    
    /* Validate parameters */
    if (data == NULL || length == 0 || length > 4) {
        return -1;
    }
    
    /* Pull CS low to start transaction */
    HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_RESET);
    
    /* Transmit address byte */
    status = HAL_SPI_Transmit(&hspi1, &write_addr, 1, TMC5240_SPI_TIMEOUT);
    if (status != HAL_OK) {
        HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_SET);
        return -1;
    }
    
    /* Transmit data bytes */
    status = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, length, TMC5240_SPI_TIMEOUT);
    
    /* Pull CS high to end transaction */
    HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_SET);
    
    return (status == HAL_OK) ? 0 : -1;
}

/**
  * @brief  Read data from TMC5240 via SPI
  * @param  address: Register address (without write bit)
  * @param  data: Pointer to buffer to store read data
  * @param  length: Number of bytes to read
  * @retval 0 on success, -1 on error
  */
int8_t tmc5240_platform_spi_read(uint8_t address, uint8_t *data, size_t length)
{
    HAL_StatusTypeDef status;
    uint8_t read_addr = address & 0x7F;  /* Clear write bit */
    uint8_t dummy_data[4] = {0};
    
    /* Validate parameters */
    if (data == NULL || length == 0 || length > 4) {
        return -1;
    }
    
    /* Pull CS low to start transaction */
    HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_RESET);
    
    /* Transmit address byte */
    status = HAL_SPI_Transmit(&hspi1, &read_addr, 1, TMC5240_SPI_TIMEOUT);
    if (status != HAL_OK) {
        HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_SET);
        return -1;
    }
    
    /* Transmit dummy bytes to receive data (full duplex SPI) */
    status = HAL_SPI_TransmitReceive(&hspi1, dummy_data, data, length, TMC5240_SPI_TIMEOUT);
    
    /* Pull CS high to end transaction */
    HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_SET);
    
    return (status == HAL_OK) ? 0 : -1;
}
