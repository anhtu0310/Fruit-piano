/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"

#define TOUCH_BUTTON_NUM 14
#define MAX_HOLD_KEYS 6
// #define TOUCH_CHANGE_CONFIG 0

static const touch_pad_t button[TOUCH_BUTTON_NUM] = {
    TOUCH_PAD_NUM1,
    TOUCH_PAD_NUM2,
    TOUCH_PAD_NUM3,
    TOUCH_PAD_NUM4,
    TOUCH_PAD_NUM5,
    TOUCH_PAD_NUM6,
    TOUCH_PAD_NUM7,
    TOUCH_PAD_NUM8,
    TOUCH_PAD_NUM9,
    TOUCH_PAD_NUM10,
    TOUCH_PAD_NUM11,
    TOUCH_PAD_NUM12,
    TOUCH_PAD_NUM13,
    TOUCH_PAD_NUM14};

uint8_t keycodes[TOUCH_BUTTON_NUM] = {
    HID_KEY_Q,
    HID_KEY_W,
    HID_KEY_E,
    HID_KEY_R,
    HID_KEY_T,
    HID_KEY_Y,
    HID_KEY_U,
    HID_KEY_I,

    HID_KEY_O,
    HID_KEY_P,
    HID_KEY_Z,
    HID_KEY_X,
    HID_KEY_C,
    HID_KEY_V

    // {HID_KEY_O},
    // {HID_KEY_E},
};

\
static void tp_example_read_task(void *pvParameter)
{

    uint8_t keys_pressed[MAX_HOLD_KEYS];
    uint8_t keys_pressed_ck = 0;
    uint8_t last_keys_pressed_ck = 0;
    uint32_t touch_value;
    /* Wait touch sensor init done */
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // printf("Touch Sensor read, the output format is: \nTouchpad num:[raw data]\n\n");
    uint8_t *temp_keys;
    while (1)
    {
        memset(&keys_pressed, 0x00, MAX_HOLD_KEYS);
        temp_keys = keys_pressed;
        keys_pressed_ck = 0;
        for (int i = 0; i < TOUCH_BUTTON_NUM; i++)
        {
            touch_pad_read_raw_data(button[i], &touch_value); // read raw data.
            if (touch_value > 70000)
            {
                keys_pressed_ck |= keycodes[i];
                *temp_keys = keycodes[i];
                temp_keys++;
                if (temp_keys - keys_pressed >= 6)
                {
                    break;

                } // key_array_bits = key_array_bits | (1 << i);
            }
            // key_array_bits = key_array_bits & (~(1 << i));

            // printf("T%d: [%4d] ", button[i], touch_value);
        }
        // printf("\n");

        if (keys_pressed_ck != last_keys_pressed_ck)
        {
            tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keys_pressed);

            last_keys_pressed_ck = keys_pressed_ck;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
// void key_process()
// {
//     if (keys_pressed_ck == last_keys_pressed_ck)
//         return;

    // tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
    // vTaskDelay(pdMS_TO_TICKS(50));

    // for (int i = 0; i < TOUCH_BUTTON_NUM; i++)
    // {
    //     if (1 & (key_array_bits >> i))
    //     {
    //         tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, &keycodes[i][0]);
    //     }
    // }
    // last_key_array_bits = key_array_bits;

//     tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keys_pressed);

//     last_keys_pressed_ck = keys_pressed_ck;
// }

// #define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
#define LED_BLUE (GPIO_NUM_15) // Use BOOT signal by default

static const char *TAG = "piano";

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

/********* Application ***************/

static void app_send_hid_demo(void)
{
    // Keyboard output: Send key 'a/A' pressed and released
    ESP_LOGI(TAG, "Sending Keyboard report");
    uint8_t keycode[6] = {HID_KEY_A, HID_KEY_B, HID_KEY_C, HID_KEY_D, HID_KEY_E, HID_KEY_F};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);

    vTaskDelay(pdMS_TO_TICKS(5000));
    uint8_t keycode2[6] = {HID_KEY_A, HID_KEY_B, HID_KEY_C, HID_KEY_D, HID_KEY_F, HID_KEY_Q};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode2);

    vTaskDelay(pdMS_TO_TICKS(5000));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
}

void app_main(void)
{

    touch_pad_init();
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++)
    {
        touch_pad_config(button[i]);
    }
    /* Denoise setting at TouchSensor 0. */
    touch_pad_denoise_t denoise = {
        /* The bits to be cancelled are determined according to the noise level. */
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();
    ESP_LOGI(TAG, "Denoise function init");

    /* Enable touch sensor clock. Work mode is "timer trigger". */
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();

    /* Start task to read values by pads. */
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);

    // Initialize button that will trigger HID reports
    const gpio_config_t led_pin_config = {
        .pin_bit_mask = BIT64(LED_BLUE),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = false,
        .pull_down_en = false,
    };
    gpio_config(&led_pin_config);

    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    gpio_set_level(LED_BLUE,1);

}
