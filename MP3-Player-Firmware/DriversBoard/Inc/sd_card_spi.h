#ifndef SD_CARD_SPI_H
#define SD_CARD_SPI_H

#include <stdint.h>
#include <stdbool.h>

#define SD_CARD_SPI_BLOCK_SIZE 512

typedef enum {
    SD_CARD_SPI_STATUS_OK = 0,
    SD_CARD_SPI_STATUS_ERR_NOT_INITIALIZED,
    SD_CARD_SPI_STATUS_ERR_INVALID_ARGUMENT,
    SD_CARD_SPI_STATUS_ERR_NO_CARD_DETECTED,
    SD_CARD_SPI_STATUS_ERR_SPI,
    SD_CARD_SPI_STATUS_ERR_TIMEOUT,
    SD_CARD_SPI_STATUS_ERR_UNSUPPORTED_CARD,
    SD_CARD_SPI_STATUS_ERR_CHECKSUM_INVALID,
    
    SD_CARD_SPI_STATUS_ERR_CMD0,
    SD_CARD_SPI_STATUS_ERR_CMD8,
    SD_CARD_SPI_STATUS_ERR_CMD55,
    SD_CARD_SPI_STATUS_ERR_ACMD41,
    SD_CARD_SPI_STATUS_ERR_CMD58,
    SD_CARD_SPI_STATUS_ERR_CMD17,
} sd_card_spi_status_e;

sd_card_spi_status_e sd_card_spi_init(void);
sd_card_spi_status_e sd_card_spi_read_block(uint32_t block_index, uint8_t* buffer);
sd_card_spi_status_e sd_card_spi_read_blocks(uint32_t start_block_index, uint8_t* buffer, uint32_t block_count);
bool sd_card_spi_card_is_inserted(void);
bool sd_card_spi_is_initialized(void);

#endif // SD_CARD_SPI_H