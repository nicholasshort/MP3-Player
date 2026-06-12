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

static inline bool start_transaction(void) {

    ncs_low();

    return true;

}

static inline bool end_transaction(void) {

    ncs_high();

    uint8_t rx;
    
    return txrx_byte(SD_CARD_SPI_DUMMY_BYTE, &rx);

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

static bool send_command_packet_and_read_r1(uint8_t cmd, uint32_t argument, uint8_t crc, uint8_t* r1) {

    if (r1 == NULL)
        return false;

    uint8_t tx;
    
    // Send Command Byte
    tx = 0x40 | cmd;
    if (!txrx_byte(tx, r1))
        return false;
    
    for (uint8_t i = 0; i < 4; i++) {
        tx = (uint8_t)(argument >> (8*(3-i)));
        if (!txrx_byte(tx, r1))
            return false;  
    }

    tx = (crc | 0x01);
    if (!txrx_byte(tx, r1))
        return false;

    // Send DUMMY bytes waiting for R1 response byte
    for (uint8_t i = 0; i < SD_DUMMY_READ_REQUESTS; i++) {
        if (!txrx_byte(SD_CARD_SPI_DUMMY_BYTE, r1))
            return false;
        
        if (!(*r1 >> 7))
            return true;
    }

    *r1 = 0xFF;
    return true;

}

static bool send_command_complete_transaction(uint8_t cmd, uint32_t argument, uint8_t crc, uint8_t* r1) {

    if (r1 == NULL)
        return false;
    
    bool ok = start_transaction();
    ok &= send_command_packet_and_read_r1(cmd, argument, crc, r1);
    ok &= end_transaction();

    return ok;

}

sd_card_spi_status_e sd_card_spi_init() {
    ncs_high();
    initialized = true;
    return SD_CARD_SPI_STATUS_OK;
}

bool sd_card_spi_card_inserted() {
    return (HAL_GPIO_ReadPin(SD_CARD_NCD_GPIO_Port, SD_CARD_NCD_Pin) == false);
}


#define SD_CMD_CMD0                 0u
#define SD_CMD0_ARG                 0u
#define SD_CMD0_CRC                 0x95

#define SD_CMD_CMD8                 8u
#define SD_CMD8_ARG                 0x000001AAu
#define SD_CMD8_CRC                 0x87u

#define SD_CMD_CMD55                55u
#define SD_CMD55_ARG                0u
#define SD_CMD55_CRC                0xFFu

#define SD_CMD_ACMD41               41u
#define SD_ACMD41_ARG               0x40000000u
#define SD_ACMD41_CRC               0xFFu

#define SD_CMD_CMD58                58u
#define SD_CMD58_ARG                0u
#define SD_CMD58_CRC                0xFFu

#define SD_R1_IDLE_STATE            0x01u
#define SD_R1_READY_STATE           0x00u

#define SD_INIT_HANDSHAKE_TIMEOUT_MS 1000u

bool sd_card_spi_test() {
    
    bool ok = send_dummy_clocks();
    if (!ok)
        return false;

    uint8_t r1;

    // --------------
    // ENTER SPI MODE 
    // --------------
    // Send CMD0
    ok &= send_command_complete_transaction(SD_CMD_CMD0, SD_CMD0_ARG, SD_CMD0_CRC, &r1);
    if (r1 != SD_R1_IDLE_STATE || !ok) // Ensure R1 response byte
        return false;
        
    // -------------
    // VOLTAGE CHECK
    // -------------
    // Send CMD8
    ok &= start_transaction();
    ok &= send_command_packet_and_read_r1(SD_CMD_CMD8, SD_CMD8_ARG, SD_CMD8_CRC, &r1);
    if (r1 != SD_R1_IDLE_STATE || !ok) {// Ensure R1 response byte
        end_transaction();
        return false;
    }
    uint8_t r7_buffer[4];
    ok &= read_buffer(r7_buffer, 4);
    ok &= end_transaction();
    if (!ok || !((r7_buffer[0] == 0x00) &&
                 (r7_buffer[1] == 0x00) &&
                 (r7_buffer[2] == 0x01) &&
                 (r7_buffer[3] == 0xAA))) { // Ensure expected R7 response
            return false;
    }

    // -----------------------------
    // INITIALIZATION HANDSHAKE LOOP
    // -----------------------------
    uint32_t start_ms = HAL_GetTick();
    do {
        // Send CMD55
        ok = send_command_complete_transaction(SD_CMD_CMD55, SD_CMD55_ARG, SD_CMD55_CRC, &r1);
        if (!ok)
            return false;
        
        if ((r1 != SD_R1_IDLE_STATE) && (r1 != SD_R1_READY_STATE))
            return false;


        // Send ACMD41
        ok = send_command_complete_transaction(SD_CMD_ACMD41, SD_ACMD41_ARG, SD_ACMD41_CRC, &r1);
        if (!ok)
            return false;
        
        if (r1 == SD_R1_READY_STATE)
            break; 
        
        if (r1 != SD_R1_IDLE_STATE)
            return false;

        // Check timeout
        if ((HAL_GetTick() - start_ms) > SD_INIT_HANDSHAKE_TIMEOUT_MS)
            return false;

    } while (r1 == SD_R1_IDLE_STATE);

    // -------------------
    // Check Card Capacity
    // -------------------
    ok &= start_transaction();
    ok &= send_command_packet_and_read_r1(SD_CMD_CMD58, SD_CMD58_ARG, SD_CMD58_CRC, &r1);
    if (r1 != SD_R1_READY_STATE || !ok) {// Ensure R1 response byte
        end_transaction();
        return false;
    }
    uint8_t ocr[4];
    ok &= read_buffer(ocr, 4);
    ok &= end_transaction();
    if (!ok)
        return false;
    bool high_extended_capacity = (ocr[0] & 0x40); // Check 30th bit for CCS


    return true;

}
