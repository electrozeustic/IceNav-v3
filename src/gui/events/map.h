/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1.4
 * @date 2023-05-23
 */

/**
 * @brief Map tile coordinates and zoom
 *
 */
MapTile CurrentMapTile;
MapTile RoundMapTile;

/**
 * @brief Navitagion Arrow position on screen
 *
 */
ScreenCoord NavArrow_position;

/**
 * @brief Sprite for Navigation Arrow in map tile
 *
 */
TFT_eSprite sprArrow = TFT_eSprite(&tft);

/**
 * @brief Double Buffering Sprites for Map Tile
 *
 */
TFT_eSprite map_spr = TFT_eSprite(&tft);
TFT_eSprite map_buf = TFT_eSprite(&tft);

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
  lv_obj_t *screen = lv_event_get_current_target(event);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (act_tile == MAP)
  {
    switch (dir)
    {
    case LV_DIR_LEFT:
      break;
    case LV_DIR_RIGHT:
      break;
    case LV_DIR_TOP:
      if (zoom >= MIN_ZOOM && zoom < MAX_ZOOM)
      {
        zoom++;
        lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
        lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
      }
      break;
    case LV_DIR_BOTTOM:
      if (zoom <= MAX_ZOOM && zoom > MIN_ZOOM)
      {
        zoom--;
        lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
        lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
      }
      break;
    }
  }
}

/**
 * @brief return latitude from GPS or sys env pre-built variable
 * @return latitude or 0.0 if not defined
*/
static double getLat() {
  if (GPS.location.isValid()) return GPS.location.lat();
  else {
#ifdef DEFAULT_LAT
    return DEFAULT_LAT;
#else
    return 0.0;
#endif
  }
}

/**
 * @brief return longitude from GPS or sys env pre-built variable
 * @return longitude or 0.0 if not defined
*/
static double getLon() {
  if (GPS.location.isValid()) return GPS.location.lng();
  else {
#ifdef DEFAULT_LON
    return DEFAULT_LON;
#else
    return 0.0;
#endif
  }
}

/**
 * @brief Update map event
 *
 * @param event
 */
static void update_map(lv_event_t *event)
{
  CurrentMapTile = get_map_tile(getLon(), getLat(), zoom, 0, 0);

  if (strcmp(CurrentMapTile.file, OldMapTile.file) != 0 || CurrentMapTile.zoom != OldMapTile.zoom ||
      CurrentMapTile.tilex != OldMapTile.tilex || CurrentMapTile.tiley != OldMapTile.tiley)
  {
    is_map_draw = false;
    map_found = false;
  }

  if (!is_map_draw)
  {
    OldMapTile.zoom = CurrentMapTile.zoom;
    OldMapTile.tilex = CurrentMapTile.tilex;
    OldMapTile.tiley = CurrentMapTile.tiley;
    OldMapTile.file = CurrentMapTile.file;

    map_spr.deleteSprite();
    map_spr.createSprite(320, 335);
    map_buf.deleteSprite();
    map_buf.createSprite(320, 335);

    log_v("%s", CurrentMapTile.file);

    // Center Tile
    map_found = map_spr.drawPngFile(SD, CurrentMapTile.file, 32, 0);
    map_buf.drawPngFile(SD, CurrentMapTile.file, 32, 0);
    // Left Center Tile
    RoundMapTile = get_map_tile(getLon(), getLat(), zoom, -1, 0);
    map_spr.drawPngFile(SD, RoundMapTile.file, 0, 0, 32, 256, 224, 0);
    map_buf.drawPngFile(SD, RoundMapTile.file, 0, 0, 32, 256, 224, 0);
    // Right Center Tile
    RoundMapTile = get_map_tile(getLon(), getLat(), zoom, 1, 0);
    map_spr.drawPngFile(SD, RoundMapTile.file, 287, 0, 32, 256, 0, 0);
    map_buf.drawPngFile(SD, RoundMapTile.file, 287, 0, 32, 256, 0, 0);
    // Bottom Center Tile
    RoundMapTile = get_map_tile(getLon(), getLat(), zoom, 0, 1);
    map_spr.drawPngFile(SD, RoundMapTile.file, 32, 256, 256, 79, 0, 0);
    map_buf.drawPngFile(SD, RoundMapTile.file, 32, 256, 256, 79, 0, 0);
    // Left Bottom Center Tile
    RoundMapTile = get_map_tile(getLon(), getLat(), zoom, -1, 1);
    map_spr.drawPngFile(SD, RoundMapTile.file, 0, 256, 32, 79, 224, 0);
    map_buf.drawPngFile(SD, RoundMapTile.file, 0, 256, 32, 79, 224, 0);
    // Right Bottom Center Tile
    RoundMapTile = get_map_tile(getLon(), getLat(), zoom, 1, 1);
    map_spr.drawPngFile(SD, RoundMapTile.file, 287, 256, 32, 79, 0, 0);
    map_buf.drawPngFile(SD, RoundMapTile.file, 287, 256, 32, 79, 0, 0);

    // Arrow Sprite
    sprArrow.deleteSprite();
    sprArrow.createSprite(16, 16);
    sprArrow.fillSprite(TFT_BLACK);
    sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);

    is_map_draw = true;
  }

  if (map_found)
  {
    NavArrow_position = coord_to_scr_pos(32, 0, getLon(), getLat(), zoom);
    uint8_t arrow_bkg[1800];
    map_buf.readRect(NavArrow_position.posx - 12, NavArrow_position.posy - 12, 24, 24, (uint16_t *)arrow_bkg);
    map_spr.pushImage(NavArrow_position.posx - 12, NavArrow_position.posy - 12, 24, 24, (uint16_t *)arrow_bkg);

#ifdef ENABLE_COMPASS
    heading = read_compass();
    map_spr.setPivot(NavArrow_position.posx, NavArrow_position.posy);
    sprArrow.pushRotated(&map_spr, heading, TFT_BLACK);
#else
    sprArrow.pushSprite(&map_spr, NavArrow_position.posx, NavArrow_position.posy, TFT_BLACK);
#endif
  }
  map_spr.pushSprite(0, 64);
}
