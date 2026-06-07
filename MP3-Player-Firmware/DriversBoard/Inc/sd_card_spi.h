#ifndef SD_CARD_SPI_H
#define SD_CARD_SPI_H

#include <stdbool.h>

typedef enum {
    SD_CARD_SPI_STATUS_OK = 0,
    SD_CARD_SPI_STATUS_ERR_NOT_INITIALIZED,
    SD_CARD_SPI_STATUS_NO_CARD_DETECTED,
} sd_card_spi_status_e;

sd_card_spi_status_e sd_card_spi_init();

bool sd_card_spi_card_is_inserted();

// Test function (TODO: remove)
bool sd_card_spi_test();

#endif // SD_CARD_SPI_H