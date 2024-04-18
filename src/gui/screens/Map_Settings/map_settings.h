/**
 * @file map_settings.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Map Settings
 * @version 0.1.8
 * @date 2024-04
 */

static lv_obj_t *mapSettingsOptions;
static lv_obj_t *mapSwitch;
static lv_obj_t *mapType;
static lv_obj_t *zoomLevel;

/**
 * @brief Map Settings events include
 *
 */
#include "gui/screens/Map_Settings/events/map_settings.h"

/**
 * @brief Create Map Settings screen
 *
 */
static void createMapSettingsScr()
{
    // Map Settings Screen
    mapSettingsScreen = lv_obj_create(NULL);
    mapSettingsOptions = lv_list_create(mapSettingsScreen);
    lv_obj_set_size(mapSettingsOptions, TFT_WIDTH, TFT_HEIGHT - 60);

    lv_obj_t *label;
    lv_obj_t *list;
    lv_obj_t *btn;
    lv_obj_t *checkCompass;
    lv_obj_t *checkSpeed;
    lv_obj_t *checkScale;

    // Map Type
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Map Type\nRENDER/VECTOR");
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    mapType = lv_switch_create(list);
    label = lv_label_create(mapType);
    lv_label_set_text_static(label, "V   R");
    lv_obj_center(label);
    if (isVectorMap)
        lv_obj_add_state(mapType, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(mapType, LV_STATE_CHECKED);
    lv_obj_align_to(mapType, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(mapType, configureMapType, LV_EVENT_VALUE_CHANGED, NULL);

    // Map Rotation
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Map Rotation Mode\nHEADING/COMPASS");
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    mapSwitch = lv_switch_create(list);
    label = lv_label_create(mapSwitch);
    lv_label_set_text_static(label, "C   H");
    lv_obj_center(label);
    if (isMapRotation)
        lv_obj_add_state(mapSwitch, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(mapSwitch, LV_STATE_CHECKED);
    lv_obj_align_to(mapSwitch, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(mapSwitch, configureMapRotation, LV_EVENT_VALUE_CHANGED, NULL);

    // Default zoom level
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Default\nZoom Level");
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);

    btn = lv_btn_create(list);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_align_to(btn, list, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_image_src(btn, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(btn, incrementZoom, LV_EVENT_ALL, NULL);

    zoomLevel = lv_spinbox_create(list);
    lv_spinbox_set_range(zoomLevel, MIN_ZOOM, MAX_ZOOM);
    lv_obj_set_width(zoomLevel, 40);
    lv_obj_clear_flag(zoomLevel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(zoomLevel, &lv_font_montserrat_20, 0);
    lv_spinbox_set_value(zoomLevel, defaultZoom);
    lv_spinbox_set_digit_format(zoomLevel, 2, 0);
    lv_obj_align_to(zoomLevel, list, LV_ALIGN_RIGHT_MID, 0, 0);
    objHideCursor(zoomLevel);

    btn = lv_btn_create(list);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_align_to(btn, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_image_src(btn, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(btn, decrementZoom, LV_EVENT_ALL, NULL);

    // Show Compass
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Compass");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    checkCompass = lv_checkbox_create(list);
    lv_obj_align_to(checkCompass, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_checkbox_set_text(checkCompass, " ");
    lv_obj_add_state(checkCompass, showMapCompass);
    lv_obj_add_event_cb(checkCompass, showCompass, LV_EVENT_VALUE_CHANGED, NULL);

    // Show Speed
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Speed");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    checkSpeed = lv_checkbox_create(list);
    lv_obj_align_to(checkSpeed, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_checkbox_set_text(checkSpeed, " ");
    lv_obj_add_state(checkSpeed, showMapSpeed);
    lv_obj_add_event_cb(checkSpeed, showSpeed, LV_EVENT_VALUE_CHANGED, NULL);

    // Show Map Scale
    list = lv_list_add_btn(mapSettingsOptions, NULL, "Show Map Scale");
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(list, LV_ALIGN_LEFT_MID);
    checkScale = lv_checkbox_create(list);
    lv_obj_align_to(checkScale, list, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_checkbox_set_text(checkScale, " ");
    lv_obj_add_state(checkScale, showMapScale);
    lv_obj_add_event_cb(checkScale, showScale, LV_EVENT_VALUE_CHANGED, NULL);

    // Back button
    btn = lv_btn_create(mapSettingsScreen);
    lv_obj_set_size(btn, TFT_WIDTH - 30, 40);
    label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_label_set_text_static(label, "Back");
    lv_obj_center(label);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, mapSettingsBack, LV_EVENT_CLICKED, NULL);
}