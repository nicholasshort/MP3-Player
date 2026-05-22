#include "sd_test.h"

#include "main.h"
#include "spi.h"

extern SPI_HandleTypeDef hspi2;

volatile uint8_t sd_idle_read_before = 0x00;
volatile uint8_t sd_cmd0_response = 0x00;
volatile uint8_t sd_debug_resp[16] = {0};

volatile uint32_t sd_hal_error_count = 0;
volatile uint32_t sd_last_hal_status = 0;

#define SD_CS_HIGH() HAL_GPIO_WritePin(SD_CARD_NCS_GPIO_Port, SD_CARD_NCS_Pin, GPIO_PIN_SET)
#define SD_CS_LOW()  HAL_GPIO_WritePin(SD_CARD_NCS_GPIO_Port, SD_CARD_NCS_Pin, GPIO_PIN_RESET)

static uint8_t sd_spi_txrx(uint8_t tx)
{
    uint8_t rx = 0xFF;

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        &hspi2,
        &tx,
        &rx,
        1,
        HAL_MAX_DELAY
    );

    sd_last_hal_status = (uint32_t)status;

    if (status != HAL_OK)
    {
        sd_hal_error_count++;
        return 0xEE; // obvious debug marker
    }

    return rx;
}

static void sd_clear_debug(void)
{
    sd_idle_read_before = 0x00;
    sd_cmd0_response = 0x00;
    sd_hal_error_count = 0;
    sd_last_hal_status = 0;

    for (int i = 0; i < 16; i++)
    {
        sd_debug_resp[i] = 0xEE;
    }
}

static void sd_send_initial_clocks(void)
{
    SD_CS_HIGH();

    /*
     * SD cards need at least 74 clocks with CS high before CMD0.
     * 10 bytes = 80 clocks.
     */
    for (int i = 0; i < 10; i++)
    {
        sd_spi_txrx(0xFF);
    }
}

static uint8_t sd_cmd0(void)
{
    uint8_t r1 = 0xFF;

    SD_CS_LOW();

    /*
     * CMD0 packet:
     * 0x40 | 0 = 0x40
     * argument = 0x00000000
     * CRC = 0x95, required for CMD0
     */
    sd_spi_txrx(0x40);
    sd_spi_txrx(0x00);
    sd_spi_txrx(0x00);
    sd_spi_txrx(0x00);
    sd_spi_txrx(0x00);
    sd_spi_txrx(0x95);

    /*
     * Capture up to 16 response bytes.
     * A valid CMD0 response is usually 0x01.
     * The card may return 0xFF for a few bytes first.
     */
    for (int i = 0; i < 16; i++)
    {
        r1 = sd_spi_txrx(0xFF);
        sd_debug_resp[i] = r1;

        if (r1 != 0xFF)
        {
            break;
        }
    }

    SD_CS_HIGH();

    /*
     * Extra 8 clocks after deselect.
     */
    sd_spi_txrx(0xFF);

    return r1;
}

uint8_t sd_basic_test(void)
{
    sd_clear_debug();

    /*
     * Make sure card is deselected before startup clocks.
     */
    SD_CS_HIGH();
    HAL_Delay(10);

    /*
     * Idle read with CS high. Often this should read 0xFF.
     * If this is 0x00 or random, MISO may be stuck/floating.
     */
    sd_idle_read_before = sd_spi_txrx(0xFF);

    sd_send_initial_clocks();

    sd_cmd0_response = sd_cmd0();

    return sd_cmd0_response;
}