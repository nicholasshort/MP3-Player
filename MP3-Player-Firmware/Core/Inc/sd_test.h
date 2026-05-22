#ifndef SD_TEST_H
#define SD_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern volatile uint8_t sd_idle_read_before;
extern volatile uint8_t sd_cmd0_response;
extern volatile uint8_t sd_debug_resp[16];
extern volatile uint32_t sd_hal_error_count;
extern volatile uint32_t sd_last_hal_status;

uint8_t sd_basic_test(void);

#ifdef __cplusplus
}
#endif

#endif // SD_TEST_H