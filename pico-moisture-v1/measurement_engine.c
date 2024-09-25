#include "measurement_engine.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include <stdio.h>

static uint32_t window_sum = 0;
static uint8_t sample_count = 0;
static uint32_t average = 4095;
static bool is_data_available = false;
static repeating_timer_t adc_timer;
static submit_data_cb_t submit_data = NULL;

static bool sample_adc(repeating_timer_t *rt) {
    const uint16_t adc_reading = adc_read();

    sample_count++;
    window_sum = window_sum + adc_reading;

    if (sample_count % 5 == 0) {
        average = window_sum / sample_count;
        sample_count = 0;
        window_sum = 0;

        if (submit_data != NULL) {
            submit_data(average);
        }
    }

    return true;
}

void measurement_engine_start(submit_data_cb_t data_cb)
{
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);
    submit_data = data_cb;
    if (add_repeating_timer_ms(1000, sample_adc, NULL, &adc_timer)) {
        printf("ADC timer started\n\r");
    }
}

MEASUREMENT_ENGINE_STATUS measurement_engine_service()
{
    return MEASUREMENT_ENGINE_STATUS_IDLE;
}

MEASUREMENT_ENGINE_STATUS measurement_engine_get_status()
{
    return MEASUREMENT_ENGINE_STATUS_IDLE;
}