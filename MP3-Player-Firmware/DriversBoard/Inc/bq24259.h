#ifndef BQ24259_H
#define BQ24259_H

#include <stdbool.h>

typedef enum {
    BQ24259_RESULT_OK,
    BQ24259_RESULT_ERROR_I2C,
    BQ24259_RESULT_ERROR_VERIFY,
    BQ24259_RESULT_BAD_ID,
    BQ24259_RESULT_INVALID_ARG,
} bq24259_result_e;

typedef enum {
    BQ24259_VBUS_STATUS_UNKNOWN,
    BQ24259_VBUS_STATUS_USB_HOST,
    BQ24259_VBUS_STATUS_ADAPTER_PORT,
    BQ24259_VBUS_STATUS_OTG,
} bq24259_vbus_status_e;

typedef enum {
    BQ24259_CHARGE_STATUS_NOT_CHARGING,
    BQ24259_CHARGE_STATUS_PRE_CHARGE,
    BQ24259_CHARGE_STATUS_FAST_CHARGING,
    BQ24259_CHARGE_STATUS_CHARGE_TERMINATION_DONE,
} bq24259_charge_status_e;


typedef struct {
    bq24259_vbus_status_e       vbus_status;
    bq24259_charge_status_e     charge_status;

    bool                        dpm_status; // True either means VBUS is sagging (VINDPM) or IINLIM reached (IINDPM)
    bool                        pg_status;
    bool                        therm_status;
    bool                        vsys_status; // 0: BAT > VSYS_MIN, 1: BAT < VSYSMIN
} bq24259_status_t;

typedef enum {
    BQ24259_CHARGE_FAULT_NORMAL,
    BQ24259_CHARGE_FAULT_INPUT_FAULT,
    BQ24259_CHARGE_FAULT_THERMAL_SHUTDOWN,
    BQ24259_CHARGE_FAULT_CHARGE_TIMER_EXPIRATION,
} bq24259_charge_fault_e;

typedef struct {
    bq24259_charge_fault_e      charge_fault;

    bool                        watchdog_fault;
    bool                        otg_fault;
    bool                        bat_fault;
    bool                        ntc_fault_cold;
    bool                        ntc_fault_hot;
} bq24259_fault_t;

bq24259_result_e bq24259_read_status(bq24259_status_t* status);
bq24259_result_e bq24259_read_fault (bq24259_fault_t*  fault ); // The registers latch events (e.g. unplugging and replugging in usb will show input fault)
bq24259_result_e bq24259_read_current_fault (bq24259_fault_t*  fault ); // Reads fault register twice to delatch old values to leave only current fault states
bq24259_result_e bq24259_enable_charging(void);
bq24259_result_e bq24259_disable_charging(void);
bq24259_result_e bq24259_enter_shipping_mode(void);
bq24259_result_e bq24259_init(void);

#endif // BQ24259_H