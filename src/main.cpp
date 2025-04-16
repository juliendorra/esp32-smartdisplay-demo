#include <Arduino.h>

#include <esp32_smartdisplay.h>
#include <ui/ui.h>

void OnAddOneClicked(lv_event_t *e)
{
    static uint32_t cnt = 0;
    cnt++;
    lv_label_set_text_fmt(ui_lblCountValue, "%u", cnt);
}

void OnRotateClicked(lv_event_t *e)
{
    auto disp = lv_disp_get_default();
    auto rotation = (lv_display_rotation_t)((lv_disp_get_rotation(disp) + 1) % (LV_DISPLAY_ROTATION_270 + 1));
    lv_display_set_rotation(disp, rotation);
}

char inputText[32] = "";
lv_obj_t *btn, *lblInputBox;

void event_btn(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        strcat(inputText, "a");
        lv_label_set_text(lblInputBox, inputText);
    }
}

void setup()
{
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(5000);
#endif
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    log_i("Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d, CPU Freq: %d Mhz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
    log_i("Free heap: %d bytes", ESP.getFreeHeap());
    log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
    log_i("SDK version: %s", ESP.getSdkVersion());

    smartdisplay_init();
    smartdisplay_lcd_set_brightness_cb(smartdisplay_lcd_adaptive_brightness_cds, 100);
    __attribute__((unused)) auto disp = lv_disp_get_default();
    // lv_disp_set_rotation(disp, LV_DISP_ROT_90);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_180);
    // lv_disp_set_rotation(disp, LV_DISP_ROT_270);

    // ui_init();

    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);

    auto scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    // To use third party libraries, enable the define in lv_conf.h: #define LV_USE_QRCODE 1
    // auto ui_qrcode = lv_qrcode_create(ui_scrMain);
    // lv_qrcode_set_size(ui_qrcode, 100);
    // lv_qrcode_set_dark_color(ui_qrcode, lv_color_black());
    // lv_qrcode_set_light_color(ui_qrcode, lv_color_white());
    // const char *qr_data = "https://ut7.fr";
    // lv_qrcode_update(ui_qrcode, qr_data, strlen(qr_data));
    // lv_obj_center(ui_qrcode);

    auto pnl = lv_obj_create(scr);
    lv_obj_set_width(pnl, lv_pct(90));
    lv_obj_set_height(pnl, lv_pct(95));
    lv_obj_set_align(pnl, LV_ALIGN_CENTER);
    lv_obj_remove_flag(pnl, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    btn = lv_button_create(pnl);
    lv_obj_set_size(btn, 80, 80);
    auto btnLabelTop = lv_label_create(btn);
    lv_label_set_text(btnLabelTop, "a");
    lv_obj_set_align(btnLabelTop, LV_ALIGN_TOP_MID);
    auto btnLabelLeft = lv_label_create(btn);
    lv_label_set_text(btnLabelLeft, "b");
    lv_obj_set_align(btnLabelLeft, LV_ALIGN_BOTTOM_LEFT);
    auto btnLabelRight = lv_label_create(btn);
    lv_label_set_text(btnLabelRight, "c");
    lv_obj_set_align(btnLabelRight, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_center(btn);

    lblInputBox = lv_label_create(pnl);
    lv_obj_set_align(lblInputBox, LV_ALIGN_TOP_MID);
    lv_label_set_text(lblInputBox, "abc");

    lv_obj_add_event_cb(btn, event_btn, LV_EVENT_ALL, NULL);

    lv_screen_load(scr);
}

ulong next_millis;
auto lv_last_tick = millis();

void loop()
{
    auto const now = millis();
    if (now > next_millis)
    {
        next_millis = now + 500;
    }

    // Update the ticker
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    // Update the UI
    lv_timer_handler();
}