#define DISPLAY_I2C_ADDRESS 0x3C
#define INA219_I2C_ADDRESS 0x40

#define MENU_FONT font5x7

#define FONT_W MENU_FONT[2]
#define FONT_H (MENU_FONT[3] + 1)

#define ENCODER_PIN_LEFT 7
#define ENCODER_PIN_RIGHT 8
#define ENCODER_PIN_BTN 6

// Limited by INA219 (+26V)
#define EMERGENCY_VOLTAGE 25000

#define VOLTAGE_REFERENCE 5.08 // 5v pin mesurment gave reference voltage

#define REFRESH_INTERVAL_MS 1000

#define THERMISTOR_PIN A7
#define THERMISTOR_NOMINAL 47900
#define TEMPERATURE_NOMINAL 25
#define B_COEFFICIENT 3950
#define THERMISTOR_SERIES_RESISTOR 17650

#define FAN_PWM_PIN 3 // can not be changed
#define FAN_ON_OFF_PIN 2
#define FAN_START_TEMPERATURE 35
#define FAN_STOP_TEMPERATURE 30
#define FAN_FULL_SPEED_TEMPERATURE 75
#define EMERGENCY_TEMPERATURE 85

#define UNUSED_ANALOG_PIN A0

// Should give 1MHz to process rotary encoder events every 1ms
#define MAX_PWM_DUTY_CYCLE 16000
#define MIN_CURRENT_MA 50 // 1 is munimum
// Max curret matches PWM resolution to have simple conversion
// in AmperagePinManager.
#define MAX_CURRENT_MA MAX_PWM_DUTY_CYCLE 
#define AMPERAGE_CHANGE_CORSE_STEP 100
#define AMPERAGE_CHANGE_FINE_STEP 1
#define CONTROL_CURRENT_PIN 10 // can not be changed

#define SD_CARD_DUMP_INTERVAL_CYCLES 5
#define SD_CARD_SC_PIN 9

#define AMPERAGE_ON_OFF_PIN A3
#define EMERGENCY_AMPERAGE 3150
#define NOICE_AMPERAGE 15
#define NOISE_VOLTAGE 15

#define ENABLE_AMPERAGE_CALIBRATION false
