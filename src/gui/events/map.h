/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1.5
 * @date 2023-06-04
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
static double getLat()
{
  if (GPS.location.isValid())
    return GPS.location.lat();
  else
  {
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
static double getLon()
{
  if (GPS.location.isValid())
    return GPS.location.lng();
  else
  {
#ifdef DEFAULT_LON
    return DEFAULT_LON;
#else
    return 0.0;
#endif
  }
}

/**
 * @brief Delete map screen sprites and release PSRAM
 *
 */
static void delete_map_scr_sprites()
{
  sprArrow.deleteSprite();
  compass_spr.deleteSprite();
  compass_rot.deleteSprite();
  map_rot.deleteSprite();
}

/**
 * @brief Create a map screen sprites
 *
 */
static void create_map_scr_sprites()
{
  // Map Sprite
  map_rot.createSprite(320, 335);
  map_rot.pushSprite(0, 64);
  // Arrow Sprite
  sprArrow.createSprite(16, 16);
  sprArrow.setColorDepth(16);
  sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
  // Mini Compass
#ifdef ENABLE_COMPASS
  compass_spr.createSprite(48, 48);
  compass_spr.setColorDepth(16);
  compass_spr.pushImage(0, 0, 48, 48, (uint16_t *)mini_compass);
#endif
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
    // map_spr.deleteSprite();
    // map_spr.createSprite(768, 768);
  }

  if (!is_map_draw)
  {
    OldMapTile.zoom = CurrentMapTile.zoom;
    OldMapTile.tilex = CurrentMapTile.tilex;
    OldMapTile.tiley = CurrentMapTile.tiley;
    OldMapTile.file = CurrentMapTile.file;

    log_v("TILE: %s", CurrentMapTile.file);
    log_v("ZOOM: %d", zoom);

    // Center Tile
    map_found = map_spr.drawPngFile(SD, CurrentMapTile.file, 256, 256);

    uint8_t centerX = 0;
    uint8_t centerY = 0;
    int8_t startX = centerX - 1;
    int8_t startY = centerY - 1;
    bool tileFound = false;

    if (map_found)
    {
      for (int y = startY; y <= startY + 2; y++)
      {
        for (int x = startX; x <= startX + 2; x++)
        {
          if (x == centerX && y == centerY)
          {
            // Skip Center Tile
            continue;
          }
          RoundMapTile = get_map_tile(getLon(), getLat(), zoom, x, y);
          tileFound = map_spr.drawPngFile(SD, RoundMapTile.file, (x - startX) * tileSize, (y - startY) * tileSize);
          if (!tileFound)
            map_spr.fillRect((x - startX) * tileSize, (y - startY) * tileSize, tileSize, tileSize, LVGL_BKG);
        }
      }
    }

    is_map_draw = true;
  }

  if (map_found)
  {
    NavArrow_position = coord_to_scr_pos(getLon(), getLat(), zoom);
    map_spr.setPivot(tileSize + NavArrow_position.posx, tileSize + NavArrow_position.posy);
    map_rot.pushSprite(0, 64);

#ifdef ENABLE_COMPASS
    heading = read_compass();
    map_spr.pushRotated(&map_rot, 360 - heading, TFT_TRANSPARENT);
    sprArrow.setPivot(8, 8);
    sprArrow.pushRotated(&map_rot, 0, TFT_BLACK);

    compass_rot.deleteSprite();
    compass_rot.createSprite(48, 48);
    compass_rot.setColorDepth(16);
    compass_spr.pushRotated(&compass_rot, 360 - heading, TFT_BLACK);
    compass_rot.pushSprite(&map_rot, 264, 10, TFT_BLACK);
#else
    map_spr.pushRotated(&map_rot, 0, TFT_TRANSPARENT);
#endif

    sprArrow.setPivot(8, 8);
    sprArrow.pushRotated(&map_rot, 0, TFT_BLACK);
  }
}
