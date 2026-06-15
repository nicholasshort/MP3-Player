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
static bool high_capacity_card = false;

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

static inline uint8_t get_cmd_crc7(uint8_t cmd, uint32_t argument) {

    uint8_t crc = 0x00;

    // Loop through bytes
    for (uint8_t i = 0; i < 5; i++) {
        
        uint8_t byte = 0x00;
        if (i == 0)
            byte = 0x40 | (cmd & 0x3F);
        else
            byte = argument >> (8 * (3 - (i - 1)));
        
        // Loop through byte bits
        for (uint8_t bit = 0; bit < 8; bit++) {

            uint8_t input_bit = (byte >> (7 - bit)) & 0x01;
            uint8_t crc_msb = (crc >> 6) & 0x01;
            uint8_t mix = crc_msb ^ input_bit;

            crc = (uint8_t)((crc << 1) & 0x7F);

            if (mix) {
                crc ^= 0x09;
            }
        }
    }

    return crc & 0x07F;


}

#define SD_DUMMY_READ_REQUESTS 16

static bool send_command_packet_and_read_r1(uint8_t cmd, uint32_t argument, uint8_t* r1) {

    if (r1 == NULL)
        return false;

    uint8_t tx;
    
    // Send Command Byte
    tx = 0x40 | (cmd & 0x3F);
    if (!txrx_byte(tx, r1))
        return false;
    
    for (uint8_t i = 0; i < 4; i++) {
        tx = (uint8_t)(argument >> (8*(3-i)));
        if (!txrx_byte(tx, r1))
            return false;  
    }

    uint8_t crc = get_cmd_crc7(cmd, argument);

    tx = (uint8_t)((crc << 1) | 0x01);
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

static bool send_command_complete_transaction(uint8_t cmd, uint32_t argument, uint8_t* r1) {

    if (r1 == NULL)
        return false;
    
    bool ok = start_transaction();
    ok &= send_command_packet_and_read_r1(cmd, argument, r1);
    ok &= end_transaction();

    return ok;

}


// SDSC uses byte addressing. SDHC/SDXC use block addressing
static inline uint32_t get_card_address(uint32_t block_address) {

    if (high_capacity_card)
        return block_address;
    else 
        return block_address * 512;

}


bool sd_card_spi_card_is_inserted() {
    return (HAL_GPIO_ReadPin(SD_CARD_NCD_GPIO_Port, SD_CARD_NCD_Pin) == false);
}


#define SD_CMD_CMD0                 0u
#define SD_CMD0_ARG                 0u

#define SD_CMD_CMD8                 8u
#define SD_CMD8_ARG                 0x000001AAu

#define SD_CMD_CMD55                55u
#define SD_CMD55_ARG                0u

#define SD_CMD_ACMD41               41u
#define SD_ACMD41_ARG               0x40000000u

#define SD_CMD_CMD58                58u
#define SD_CMD58_ARG                0u

#define SD_R1_IDLE_STATE            0x01u
#define SD_R1_READY_STATE           0x00u

#define SD_INIT_HANDSHAKE_TIMEOUT_MS 1000u

sd_card_spi_status_e sd_card_spi_init() {

    if (!sd_card_spi_card_is_inserted())
        return SD_CARD_SPI_STATUS_ERR_NO_CARD_DETECTED;
    
    bool ok = send_dummy_clocks();
    if (!ok)
        return SD_CARD_SPI_STATUS_ERR_SPI;

    uint8_t r1;

    // --------------
    // ENTER SPI MODE 
    // --------------
    // Send CMD0
    ok &= send_command_complete_transaction(SD_CMD_CMD0, SD_CMD0_ARG, &r1);
    if (!ok)
        return SD_CARD_SPI_STATUS_ERR_SPI;
    if (r1 != SD_R1_IDLE_STATE) // Ensure R1 response byte
        return SD_CARD_SPI_STATUS_ERR_CMD0;
        
    // -------------
    // VOLTAGE CHECK
    // -------------
    // Send CMD8
    ok &= start_transaction();
    ok &= send_command_packet_and_read_r1(SD_CMD_CMD8, SD_CMD8_ARG, &r1);
    if (!ok) {
        end_transaction();
        return SD_CARD_SPI_STATUS_ERR_SPI;
    }
    if (r1 != SD_R1_IDLE_STATE) {// Ensure R1 response byte
        end_transaction();
        return SD_CARD_SPI_STATUS_ERR_CMD8;
    }

    uint8_t r7_buffer[4];
    ok &= read_buffer(r7_buffer, 4);
    ok &= end_transaction();

    if (!ok)
        return SD_CARD_SPI_STATUS_ERR_SPI;

    if (!((r7_buffer[0] == 0x00) &&
          (r7_buffer[1] == 0x00) &&
          (r7_buffer[2] == 0x01) &&
          (r7_buffer[3] == 0xAA))) { // Ensure expected R7 response
            return SD_CARD_SPI_STATUS_ERR_CMD8;
    }

    // -----------------------------
    // INITIALIZATION HANDSHAKE LOOP
    // -----------------------------
    uint32_t start_ms = HAL_GetTick();
    do {
        // Send CMD55
        ok = send_command_complete_transaction(SD_CMD_CMD55, SD_CMD55_ARG, &r1);
        if (!ok)
            return SD_CARD_SPI_STATUS_ERR_SPI;
        
        if ((r1 != SD_R1_IDLE_STATE) && (r1 != SD_R1_READY_STATE))
            return SD_CARD_SPI_STATUS_ERR_CMD55;


        // Send ACMD41
        ok = send_command_complete_transaction(SD_CMD_ACMD41, SD_ACMD41_ARG, &r1);
        if (!ok)
            return SD_CARD_SPI_STATUS_ERR_SPI;
        
        if (r1 == SD_R1_READY_STATE)
            break; 
        
        if (r1 != SD_R1_IDLE_STATE)
            return SD_CARD_SPI_STATUS_ERR_ACMD41;

        // Check timeout
        if ((HAL_GetTick() - start_ms) > SD_INIT_HANDSHAKE_TIMEOUT_MS)
            return SD_CARD_SPI_STATUS_ERR_TIMEOUT;

    } while (r1 == SD_R1_IDLE_STATE);

    // -------------------
    // Check Card Capacity
    // -------------------
    ok &= start_transaction();
    // Send CMD58
    ok &= send_command_packet_and_read_r1(SD_CMD_CMD58, SD_CMD58_ARG, &r1);
    if (r1 != SD_R1_READY_STATE || !ok) {// Ensure R1 response byte
        end_transaction();
        return SD_CARD_SPI_STATUS_ERR_CMD58;
    }
    uint8_t ocr[4];
    ok &= read_buffer(ocr, 4);
    ok &= end_transaction();
    if (!ok)
        return SD_CARD_SPI_STATUS_ERR_SPI;
    high_capacity_card = (ocr[0] & 0x40); // Check 30th bit for CCS
    
    // TODO: Increase SPI Clock speed here

    initialized = true;

    return SD_CARD_SPI_STATUS_OK;

}

#define SD_CMD_CMD17               17u

#define SD_READ_BLOCK_START_TOKEN  0xFEu

#define SD_READ_BLOCK_TIMEOUT_MS   1000u


sd_card_spi_status_e sd_card_spi_read_block(uint32_t block_index, uint8_t* buffer) {

    if (!initialized)
        return SD_CARD_SPI_STATUS_ERR_NOT_INITIALIZED;

    if (buffer == NULL)
        return SD_CARD_SPI_STATUS_ERR_INVALID_ARGUMENT;
    
    uint8_t r1;
    uint8_t crc_bytes[2];

    uint32_t address = get_card_address(block_index);

    // Send CMD17
    bool ok = start_transaction();
    ok &= send_command_packet_and_read_r1(SD_CMD_CMD17, address, &r1);
    if (!ok) {
        end_transaction();
        return SD_CARD_SPI_STATUS_ERR_SPI;
    }
    if (r1 != SD_R1_READY_STATE) {
        end_transaction();
        return SD_CARD_SPI_STATUS_ERR_CMD17; 
    }
        
    uint8_t rx;
    uint32_t start_ms = HAL_GetTick();
    do {

        ok = txrx_byte(SD_CARD_SPI_DUMMY_BYTE, &rx);
        if (!ok){
            end_transaction();
            return SD_CARD_SPI_STATUS_ERR_SPI;
        }

            
        if ((HAL_GetTick() - start_ms) > SD_READ_BLOCK_TIMEOUT_MS) {
            end_transaction();
            return SD_CARD_SPI_STATUS_ERR_TIMEOUT;
        }
  
    } while (rx != SD_READ_BLOCK_START_TOKEN);

    // Read 512 Bytes + 2 CRC bytes
    ok =  read_buffer(buffer, SD_CARD_SPI_BLOCK_SIZE);
    ok &= read_buffer(crc_bytes, 2);

    ok &= end_transaction();

    if (!ok)
        return SD_CARD_SPI_STATUS_ERR_SPI;

    // TODO: Verify CRC bytes

    return SD_CARD_SPI_STATUS_OK;

}


