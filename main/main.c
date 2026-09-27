#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_adc/adc_oneshot.h>
#include <hal/adc_types.h>

#define TAG "Main App"
#define READ_PERIOD UINT64_C(100000)
#define PIN_ADC 4
#define ATTENUATION ADC_ATTEN_DB_12
#define BITWIDTH ADC_BITWIDTH_DEFAULT
#define ADC_LEVELS 4096


uint64_t last_read_at = 0;
adc_oneshot_unit_handle_t adc_handle;
adc_unit_t unit;
adc_channel_t channel;
static adc_cali_handle_t adc_cali_handle;

void setup()
{
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(PIN_ADC, &unit, &channel));
 
    const adc_oneshot_unit_init_cfg_t config = {
        .unit_id = unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&config, &adc_handle));

    const adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = BITWIDTH,
        .atten = ATTENUATION,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &channel_config));

    const adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = unit,
        .chan     = channel,
        .atten    = ATTENUATION,
        .bitwidth = BITWIDTH,
    };


    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle));
}

int raw_level_to_mv(int raw) {
    return (int) ((float)raw / (float)ADC_LEVELS * 3300.0f);
}

float percentage_diff_relative_to_a(float a, float b) {
    if(a == 0) {
        ESP_LOGW(TAG, "Cannot calculate percentage diff related to 0");
        return 0;
    }

    float diff = a - b;

    return diff / (a / 100);
}

void app_main(void) 
{
    setup();

    while (1)
    {
        uint64_t now = esp_timer_get_time();

        if(now - last_read_at >= READ_PERIOD) {
            int raw;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &raw));
            int manual_mv = raw_level_to_mv(raw);
            int calibrated_mv = -1;
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &calibrated_mv));
            float diff = percentage_diff_relative_to_a(calibrated_mv, manual_mv);
            ESP_LOGI(TAG, "%d - %d - %d - %f%", raw, manual_mv, calibrated_mv, diff);
            last_read_at = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}
