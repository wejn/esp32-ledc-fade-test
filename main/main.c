#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

#define TAG "LEDC_TEST"

#define BUTTON_GPIO 9 // 9 is boot

#define TRIGGER_GPIO 3
#define SIGNAL_GPIO 4
#define MY_LEDC_OUTPUT_GPIO1 5
#define MY_LEDC_OUTPUT_GPIO2 6
#define MY_LEDC_OUTPUT_GPIO3 18
#define MY_LEDC_OUTPUT_GPIO4 19
#define MY_LEDC_OUTPUT_GPIO5 20
#define GND_GPIO 7

#define MY_DUTY_RES        LEDC_TIMER_13_BIT
#define MY_LONG_FADE_TIME  300
#define MY_SHORT_FADE_TIME 100
#define MY_QUICK_SNOOZE    50

static void ledc_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = MY_DUTY_RES,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel0 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO1,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel0));

    ledc_channel_config_t ledc_channel1 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO2,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));

    ledc_channel_config_t ledc_channel2 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO3,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));

    ledc_channel_config_t ledc_channel3 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_3,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO4,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel3));

    ledc_channel_config_t ledc_channel4 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_4,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MY_LEDC_OUTPUT_GPIO5,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel4));
}

#define MEASURE(_label, what) do { \
    uint64_t start_time = esp_timer_get_time(); \
    what; \
    uint64_t end_time = esp_timer_get_time(); \
    ESP_LOGI(TAG, "... took: %lluμs", end_time - start_time); \
} while(0)

void test1(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;

    ESP_LOGI(TAG, "Test 1: Sequential fades on one channel");
    // Start with 0 duty
    MEASURE("setup",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            gpio_set_level(SIGNAL_GPIO, 0);
           );
    ESP_LOGI(TAG, "Initial duty: %lu", ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);

    // Start a long fade to max
    ESP_LOGI(TAG, "Starting long fade to max duty");
    MEASURE("fade",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, max_duty, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0)
            );

    // Record duty before interruption
    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);
    uint32_t duty_before_interrupt = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "Duty before interrupt: %lu", duty_before_interrupt);

    // Interrupt with a fade to 0
    ESP_LOGI(TAG, "Interrupting with fade to 0");
    MEASURE("interrupting",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, MY_SHORT_FADE_TIME, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    // Wait for the second fade to complete
    vTaskDelay(MY_LONG_FADE_TIME / portTICK_PERIOD_MS);

    // Check final duty
    uint32_t final_duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "Final duty: %lu (expected to be close to 0)", final_duty);
}

void test2(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;
    const uint32_t mid_duty = max_duty / 2;

    ESP_LOGI(TAG, "Test 2: Rapid successive fades on one channel");

    // Initialize to 0
    MEASURE("setup",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);

    // Trigger multiple fades in quick succession
    ESP_LOGI(TAG, "Starting fade to 25%% duty");
    MEASURE("25-fade",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, max_duty/4, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Quickly overriding with fade to 50%% duty");
    MEASURE("50-fade",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, mid_duty, MY_LONG_FADE_TIME/2, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Quickly overriding with fade to 75%% duty");
    MEASURE("75-fade",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 3*max_duty/4, MY_LONG_FADE_TIME/3, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Finally overriding with fade to 100%% duty");
    MEASURE("50-fade",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, max_duty, MY_QUICK_SNOOZE, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    // Wait for the last fade to complete
    vTaskDelay(MY_LONG_FADE_TIME / portTICK_PERIOD_MS);

    // Check final duty
    uint32_t final_duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "Final duty: %lu (expected to be close to max: %lu)", final_duty, max_duty);
}

void test3(void)
{
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;

    ESP_LOGI(TAG, "Test 3: Concurrent fades on different channels");
    // Initialize both channels
    MEASURE("setup",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, max_duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);

    uint32_t initial_duty1 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    uint32_t initial_duty2 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ESP_LOGI(TAG, "Initial duties - Channel 1: %lu (expected near 0), Channel 2: %lu (expected near max)", initial_duty1, initial_duty2);

    // Start fades in opposite directions
    ESP_LOGI(TAG, "Starting concurrent fades in opposite directions");
    MEASURE("start",
            gpio_set_level(SIGNAL_GPIO, 1);
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, max_duty, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
            ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0, MY_LONG_FADE_TIME, LEDC_FADE_NO_WAIT));
            gpio_set_level(SIGNAL_GPIO, 0);
           );

    // Check duties mid-fade
    vTaskDelay(MY_LONG_FADE_TIME/2 / portTICK_PERIOD_MS);
    uint32_t mid_duty1 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    uint32_t mid_duty2 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ESP_LOGI(TAG, "Mid-fade duties - Channel 1: %lu (increasing), Channel 2: %lu (decreasing)", mid_duty1, mid_duty2);

    // Wait for fades to complete
    vTaskDelay((MY_LONG_FADE_TIME/2 + MY_QUICK_SNOOZE) / portTICK_PERIOD_MS);

    // Check final duties
    uint32_t final_duty1 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    uint32_t final_duty2 = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ESP_LOGI(TAG, "Final duties - Channel 1: %lu (expected near max), Channel 2: %lu (expected near 0)", final_duty1, final_duty2);
}

static void test4_task(void *pvParameters) {
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;
    uint32_t duty = 0;
    while (1) {
        duty = esp_random() % max_duty;
        ESP_ERROR_CHECK(ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, MY_SHORT_FADE_TIME, LEDC_FADE_NO_WAIT));
        ESP_ERROR_CHECK(ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
        ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, max_duty - duty, MY_SHORT_FADE_TIME, LEDC_FADE_NO_WAIT));
        vTaskDelay((MY_SHORT_FADE_TIME) / portTICK_PERIOD_MS);
    }
}

void test4(void)
{
    /* XXX: This will trigger much more readily if you add:
     * vTaskDelay(10 / portTICK_PERIOD_MS);
     * right after https://github.com/espressif/esp-idf/blob/1f46216a7214e27c1e351b4310acf7fcd77029d4/components/esp_driver_ledc/src/ledc.c#L1500
     */
    ESP_LOGI(TAG, "Test 4: esp-idf issue #15580");
    ESP_LOGW(TAG, "Let go of the BOOT button (only press it when you want this to stop)...");
    uint64_t start_time = esp_timer_get_time();
    uint64_t grace_period = 3 * 1000 * 1000; // this many seconds we don't care about BOOT button state
    static TaskHandle_t th1;
    static TaskHandle_t th2;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));

    xTaskCreate(test4_task, "test4-1", 4096, NULL, 4, &th1);
    xTaskCreate(test4_task, "test4-2", 4096, NULL, 4, &th2);

    uint64_t iters = 0;
    while (esp_timer_get_time() - start_time < grace_period || gpio_get_level(BUTTON_GPIO)) {
        if (iters % 23 == 0) {
            ESP_LOGI(TAG, "Meep-meep...");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        iters++;
    }

    ESP_LOGI(TAG, "Stopped, let go of the button.");
    while (!gpio_get_level(BUTTON_GPIO)) {
        vTaskDelay(MY_QUICK_SNOOZE / portTICK_PERIOD_MS);
    }

    vTaskDelete(th1);
    vTaskDelete(th2);
}

#define PUSH_WAIT(why) do { \
    ESP_LOGW(TAG, "Press the BOOT button to start %s...", why); \
    while (gpio_get_level(BUTTON_GPIO)) { \
        vTaskDelay(100 / portTICK_PERIOD_MS); \
    } \
    vTaskDelay(500 / portTICK_PERIOD_MS); \
    while (!gpio_get_level(BUTTON_GPIO)) { \
        vTaskDelay(100 / portTICK_PERIOD_MS); \
    } \
    vTaskDelay(100 / portTICK_PERIOD_MS); \
} while(0)

static void test5_ch(uint32_t ch2, uint32_t ch3, uint32_t ch4) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, ch2));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, ch3));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, ch4));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4));
}

static void test5(void) {
    const uint32_t max_duty = (1 << MY_DUTY_RES) - 1;

    test5_ch(0, 0, 0);

    PUSH_WAIT("ch2 :: 100%");
    test5_ch(max_duty, 0, 0);

    PUSH_WAIT("ch3 :: 100%");
    test5_ch(0, max_duty, 0);

    PUSH_WAIT("ch4 :: 100%");
    test5_ch(0, 0, max_duty);

    /*
    PUSH_WAIT("ch2 + ch3 :: 100%");
    test5_ch(max_duty, max_duty, 0);

    PUSH_WAIT("ch2 + ch4 :: 100%");
    test5_ch(max_duty, 0, max_duty);

    PUSH_WAIT("ch3 + ch4 :: 100%");
    test5_ch(0, max_duty, max_duty);

    PUSH_WAIT("ch2 + ch3 + ch4 :: 100%");
    test5_ch(max_duty, max_duty, max_duty);
    */

    PUSH_WAIT("153 mired (6535K)");
    test5_ch(9.6022 * max_duty, max_duty, 0);

    PUSH_WAIT("366 mired (2732K)");
    test5_ch(35.4613 * max_duty, 15.7588*max_duty, max_duty);

    PUSH_WAIT("454 mired (2202K)");
    test5_ch(0, 0, max_duty);

    PUSH_WAIT("finished");
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
#if 0
        PUSH("test1 (sequential interrupted fades)");
        TRIGGER(test1);

        PUSH("test2 (multiple rapid fade changes)");
        TRIGGER(test2);

        PUSH("test3 (concurrent fades on two channels)");
        TRIGGER(test3);

        PUSH("test4 (esp-idf issue #15580)");
        TRIGGER(test4);
#endif
        ESP_LOGW(TAG, "test5 (different pwm%% strip0)");
        TRIGGER(test5);
    }
}
