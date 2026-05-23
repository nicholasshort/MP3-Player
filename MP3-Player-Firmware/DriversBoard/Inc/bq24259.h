#ifndef BQ24259_H
#define BQ24259_H

typedef enum {
    BQ24259_RESULT_OK,
    BQ24259_RESULT_ERROR_I2C,
    BQ24259_RESULT_ERROR_VERIFY,
    BQ24259_RESULT_BAD_ID,
    BQ24259_RESULT_INVALID_ARG,
} bq24259_result_e;

/*
TODO: 
    bq24259_read_status()
    bq24259_read_faults()
    bq24259_enter_ship_mode()
    bq24259_set_input_current_limit()   // maybe, if you adapt to USB source
    bq24259_enable_charging() / disable_charging()
*/ 

bq24259_result_e bq24259_init(void);

#endif // BQ24259_H