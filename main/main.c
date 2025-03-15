#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "LEDC_TEST"

#define BUTTON_GPIO 9 // 9 is boot

#define TRIGGER_GPIO 3
#define SIGNAL_GPIO 4
#define MY_LEDC_OUTPUT_GPIO1 5
#define MY_LEDC_OUTPUT_GPIO2 6
#define GND_GPIO 7

#define MY_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define MY_FIRST_CHAN      LEDC_CHANNEL_0
#define MY_SECOND_CHAN     LEDC_CHANNEL_1
#define MY_DUTY_RES        LEDC_TIMER_13_BIT
#define MY_LONG_FADE_TIME  3000
#define MY_SHORT_FADE_TIME 1000

static void ledc_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = MY_LEDC_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = MY_DUTY_RES,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel1 = {
        .speed_mode     = MY_LEDC_MODE,
        .channel        = MY_FIRST_CHAN,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO1,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));

    ledc_channel_config_t ledc_channel2 = {
        .speed_mode     = MY_LEDC_MODE,
        .channel        = MY_SECOND_CHAN,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO2,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
}

void test1(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;
    
    ESP_LOGI(TAG, "Test 1: Sequential fades on one channel");
    // Start with 0 duty
    ESP_ERROR_CHECK(ledc_set_duty(MY_LEDC_MODE, MY_FIRST_CHAN, 0));
    ESP_ERROR_CHECK(ledc_update_duty(MY_LEDC_MODE, MY_FIRST_CHAN));
    ESP_LOGI(TAG, "Initial duty: %lu", ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN));

    // Start a long fade to max
    ESP_LOGI(TAG, "Starting long fade to max duty");
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, max_duty, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
    
    // Record duty before interruption
    vTaskDelay(500 / portTICK_PERIOD_MS);
    uint32_t duty_before_interrupt = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    ESP_LOGI(TAG, "Duty before interrupt: %lu", duty_before_interrupt);
    
    // Interrupt with a fade to 0
    ESP_LOGI(TAG, "Interrupting with fade to 0");
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, 0, MY_SHORT_FADE_TIME, LEDC_FADE_NO_WAIT));
    
    // Wait for the second fade to complete
    vTaskDelay(MY_LONG_FADE_TIME / portTICK_PERIOD_MS);
    
    // Check final duty
    uint32_t final_duty = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    ESP_LOGI(TAG, "Final duty: %lu (expected to be close to 0)", final_duty);
}

#define PULSE_SIGNAL_GPIO() do { \
    gpio_set_level(SIGNAL_GPIO, 1); \
    gpio_set_level(SIGNAL_GPIO, 0); \
} while(0)

void test2(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;
    const uint32_t mid_duty = max_duty / 2;
    
    ESP_LOGI(TAG, "Test 2: Rapid successive fades on one channel");
    
    // Initialize to 0
    ESP_ERROR_CHECK(ledc_set_duty(MY_LEDC_MODE, MY_FIRST_CHAN, 0));
    ESP_ERROR_CHECK(ledc_update_duty(MY_LEDC_MODE, MY_FIRST_CHAN));
    

    // Trigger multiple fades in quick succession
    ESP_LOGI(TAG, "Starting fade to 25%% duty");
    PULSE_SIGNAL_GPIO();
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, max_duty/4, 2000, LEDC_FADE_NO_WAIT));
    PULSE_SIGNAL_GPIO();
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Quickly overriding with fade to 50%% duty");
    PULSE_SIGNAL_GPIO();
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, mid_duty, 1500, LEDC_FADE_NO_WAIT));
    PULSE_SIGNAL_GPIO();
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Quickly overriding with fade to 75%% duty");
    PULSE_SIGNAL_GPIO();
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, 3*max_duty/4, 1000, LEDC_FADE_NO_WAIT));
    PULSE_SIGNAL_GPIO();
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Finally overriding with fade to 100%% duty");
    PULSE_SIGNAL_GPIO();
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, max_duty, 500, LEDC_FADE_NO_WAIT));
    PULSE_SIGNAL_GPIO();
    
    // Wait for the last fade to complete
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // Check final duty
    uint32_t final_duty = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    ESP_LOGI(TAG, "Final duty: %lu (expected to be close to max: %lu)", final_duty, max_duty);
}

void test3(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;
    
    ESP_LOGI(TAG, "Test 3: Concurrent fades on different channels");
    // Initialize both channels
    ESP_ERROR_CHECK(ledc_set_duty(MY_LEDC_MODE, MY_FIRST_CHAN, 0));
    ESP_ERROR_CHECK(ledc_set_duty(MY_LEDC_MODE, MY_SECOND_CHAN, max_duty));
    ESP_ERROR_CHECK(ledc_update_duty(MY_LEDC_MODE, MY_FIRST_CHAN));
    ESP_ERROR_CHECK(ledc_update_duty(MY_LEDC_MODE, MY_SECOND_CHAN));
    
    uint32_t initial_duty1 = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    uint32_t initial_duty2 = ledc_get_duty(MY_LEDC_MODE, MY_SECOND_CHAN);
    ESP_LOGI(TAG, "Initial duties - Channel 1: %lu (expected near 0), Channel 2: %lu (expected near max)", initial_duty1, initial_duty2);
    
    // Start fades in opposite directions
    ESP_LOGI(TAG, "Starting concurrent fades in opposite directions");
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_FIRST_CHAN, max_duty, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(MY_LEDC_MODE, MY_SECOND_CHAN, 0, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
    
    // Check duties mid-fade
    vTaskDelay(MY_LONG_FADE_TIME/2 / portTICK_PERIOD_MS);
    uint32_t mid_duty1 = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    uint32_t mid_duty2 = ledc_get_duty(MY_LEDC_MODE, MY_SECOND_CHAN);
    ESP_LOGI(TAG, "Mid-fade duties - Channel 1: %lu (increasing), Channel 2: %lu (decreasing)", mid_duty1, mid_duty2);
    
    // Wait for fades to complete
    vTaskDelay((MY_LONG_FADE_TIME/2 + 500) / portTICK_PERIOD_MS);
    
    // Check final duties
    uint32_t final_duty1 = ledc_get_duty(MY_LEDC_MODE, MY_FIRST_CHAN);
    uint32_t final_duty2 = ledc_get_duty(MY_LEDC_MODE, MY_SECOND_CHAN);
    ESP_LOGI(TAG, "Final duties - Channel 1: %lu (expected near max), Channel 2: %lu (expected near 0)", final_duty1, final_duty2);
}

void app_main(void)
{
    // Initialize LEDC hardware
    ledc_init();
    
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    
    ESP_LOGI(TAG, "==== LEDC Concurrent Fades Test ====");
    
    ESP_LOGI(TAG, "Boot button config...");
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL<<BUTTON_GPIO;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Trigger button (& GND+SIGNAL pins) config...");
    io_conf.pin_bit_mask = 1ULL<<TRIGGER_GPIO | 1ULL<<GND_GPIO | 1ULL<<SIGNAL_GPIO;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

#define PUSH(why) do { \
    ESP_LOGW(TAG, "Punch the BOOT button to start %s...", why); \
    while (gpio_get_level(BUTTON_GPIO)) { \
        vTaskDelay(100 / portTICK_PERIOD_MS); \
    } \
} while(0)

#define TRIGGER(what) do { \
    gpio_set_level(TRIGGER_GPIO, 1); \
    what(); \
    gpio_set_level(TRIGGER_GPIO, 0); \
} while(0)

    while (1) {
        PUSH("test1 (sequential interrupted fades)");
        TRIGGER(test1);
        
        PUSH("test2 (multiple rapid fade changes)");
        TRIGGER(test2);
        
        PUSH("test3 (concurrent fades on two channels)");
        TRIGGER(test3);
    }
}
