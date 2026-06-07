#include "sd_card_spi.h"
#include "spi.h"
#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <string.h>

#define SPI_HANDLE (&hspi2)
#define SPI_TIMEOUT_MS 1000u

#define SD_CARD_SPI_DUMMY_BYTE 0xFFu

static bool initialized = false;

static inline void ncs_low(void) {
    HAL_GPIO_WritePin(SD_CARD_NCS_GPIO_Port, SD_CARD_NCS_Pin, GPIO_PIN_RESET);
}

static inline void ncs_high(void) {
    HAL_GPIO_WritePin(SD_CARD_NCS_GPIO_Port, SD_CARD_NCS_Pin, GPIO_PIN_SET);
}

static inline bool txrx_byte(uint8_t tx, uint8_t* rx) {

    if (HAL_SPI_TransmitReceive(SPI_HANDLE, &tx, rx, 1, SPI_TIMEOUT_MS) != HAL_OK) {
        return false;
    }

    return true;

}

static bool read_buffer(uint8_t* buffer, uint32_t len) {

    if (buffer == NULL)
        return false;

    for (uint32_t i = 0; i < len; i++) {
        if (!txrx_byte(SD_CARD_SPI_DUMMY_BYTE, &buffer[i]))
            return false;
    }

    return true;

}

#define SD_STARTUP_DUMMY_BYTES 10u
// SD Card needs a minimum of 74 clock signals on startup (with NCS high) before responding to commands
static bool send_dummy_clocks(void) {

    ncs_high();

    uint8_t rx;

    for (uint16_t i = 0; i < SD_STARTUP_DUMMY_BYTES; i++) {
        if (!txrx_byte(SD_CARD_SPI_DUMMY_BYTE, &rx))
            return false;
    }

    return true;

}

#define SD_DUMMY_READ_REQUESTS 16

static bool send_sd_command(uint8_t cmd, uint32_t argument, uint8_t crc, uint8_t* rx) {

    uint8_t tx;
    
    // Send Command Byte
    tx = 0x40 | cmd;
    if (!txrx_byte(tx, rx))
        return false;
    
    for (uint8_t i = 0; i < 4; i++) {
        tx = (uint8_t)(argument >> (8*(3-i)));
        if (!txrx_byte(tx, rx))
            return false;  
    }

    tx = (crc | 0x01);
    if (!txrx_byte(tx, rx))
        return false;

    // Send DUMMY bytes waiting for R1 response byte
    for (uint8_t i = 0; i < SD_DUMMY_READ_REQUESTS; i++) {
        if (!txrx_byte(SD_CARD_SPI_DUMMY_BYTE, rx))
            return false;
        
        if (!(*rx >> 7))
            return true;
    }

    *rx = 0xFF;
    return true;

}

sd_card_spi_status_e sd_card_spi_init() {
    ncs_high();
    initialized = true;
    return SD_CARD_SPI_STATUS_OK;
}

bool sd_card_spi_card_inserted() {
    return (HAL_GPIO_ReadPin(SD_CARD_NCD_GPIO_Port, SD_CARD_NCD_Pin) == false);
}

bool sd_card_spi_test() {
    bool ok = send_dummy_clocks();
    ncs_low();
    uint8_t rx;
    ok &= send_sd_command(0, 0, 0x95, &rx);
    ncs_high();
    return (ok && (rx == 0x01u));
}
