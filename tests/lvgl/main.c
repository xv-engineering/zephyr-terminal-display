#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <math.h>
#include <lvgl.h>
#include <lvgl_zephyr.h>

LOG_MODULE_REGISTER(test, CONFIG_TEST_LOG_LEVEL);

void timer_cb(lv_timer_t *timer)
{
    lv_obj_t* label = lv_timer_get_user_data(timer);
    lv_color_t color = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    lv_color_hsv_t hsv = lv_color_rgb_to_hsv(color.red, color.green, color.blue);
    hsv.h += 20;
    hsv.h %= 360;
    color = lv_color_hsv_to_rgb(hsv.h, 100, 100);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    int res = lvgl_init();
    if (res != 0)
    {
        LOG_ERR("Failed to initialize lvgl");
        return res;
    }

    lv_obj_t *screen = lv_screen_active();

    if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM))
    {
        // if on native sim, it's hard to connect fast enough, but
        // we want to catch the initial de-blanking and "Zephyr!"
        LOG_INF("Sleeping for 5 seconds");
        k_sleep(K_SECONDS(5));
    }

    BUILD_ASSERT(IS_ENABLED(CONFIG_LV_USE_FLEX));
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

    lv_obj_t *zephyr_label = lv_label_create(screen);
    lv_label_set_text(zephyr_label, "Zephyr!");
    lv_obj_set_style_text_color(zephyr_label, lv_color_make(128, 0, 128), 0);

    lv_obj_t *sub_label = lv_label_create(screen);
    lv_label_set_text(sub_label, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(sub_label, lv_color_make(255, 0, 0), 0);
    lv_timer_create(timer_cb, 500, sub_label);

    LOG_INF("Turning off display blanking");
    display_blanking_off(display);

    while (true)
    {
        uint32_t ms = lv_timer_handler();
        k_sleep(K_MSEC(ms));
    }
}
