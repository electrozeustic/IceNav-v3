/**
 * @file renderMaps.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Render maps draw functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "renderMaps.hpp"

MapTile oldMapTile = {"", 0, 0, 0};     // Old Map tile coordinates and zoom
MapTile currentMapTile = {"", 0, 0, 0}; // Curreng Map tile coordinates and zoom
MapTile roundMapTile = {"", 0, 0, 0};   // Boundaries Map tiles

bool isMapFound = false;
bool refreshMap = false;
bool isMapDraw = false;

/**
 * @brief Tile size for position calculation
 *
 */
uint16_t tileSize = 256;

TFT_eSprite sprArrow = TFT_eSprite(&tft);      // Sprite for Navigation Arrow in map tile
TFT_eSprite mapTempSprite = TFT_eSprite(&tft); // Temporary Map Sprite 9x9 tiles 768x768 pixels
TFT_eSprite mapSprite = TFT_eSprite(&tft);     // Screen Map Sprite (Viewed in LVGL Tile)

/**
 * @brief Navitagion Arrow position on screen
 *
 */
ScreenCoord navArrowPosition;

/**
 * @brief Get TileY for OpenStreeMap files
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return X value (folder)
 */
uint32_t lon2tilex(double f_lon, uint8_t zoom)
{
    return (uint32_t)(floor((f_lon + 180.0) / 360.0 * pow(2.0, zoom)));
}

/**
 * @brief Get TileY for OpenStreetMap files
 *
 * @param f_lat -> latitude
 * @param zoom  -> zoom
 * @return Y value (file)
 */
uint32_t lat2tiley(double f_lat, uint8_t zoom)
{
    return (uint32_t)(floor((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, zoom)));
}

/**
 * @brief Get pixel X position from OpenStreetMap
 *
 * @param f_lon -> longitude
 * @param zoom -> zoom
 * @return X position
 */
uint16_t lon2posx(float f_lon, uint8_t zoom)
{
    return ((uint16_t)(((f_lon + 180.0) / 360.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Get pixel Y position from OpenStreetMap
 *
 * @param f_lat -> latitude
 * @param zoom -> zoom
 * @return Y position
 */
uint16_t lat2posy(float f_lat, uint8_t zoom)
{
    return ((uint16_t)(((1.0 - log(tan(f_lat * M_PI / 180.0) + 1.0 / cos(f_lat * M_PI / 180.0)) / M_PI) / 2.0 * (pow(2.0, zoom)) * tileSize)) % tileSize);
}

/**
 * @brief Convert GPS Coordinates to screen position (with offsets)
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> Zoom level
 * @return ScreenCoord -> Screen position
 */
ScreenCoord coord2ScreenPos(double lon, double lat, uint8_t zoomLevel)
{
    ScreenCoord data;
    data.posX = lon2posx(lon, zoomLevel);
    data.posY = lat2posy(lat, zoomLevel);
    return data;
}

/**
 * @brief Get the map tile structure from GPS Coordinates
 *
 * @param lon -> Longitude
 * @param lat -> Latitude
 * @param zoomLevel -> zoom level
 * @param offsetX -> Tile Offset X
 * @param offsetY -> Tile Offset Y
 * @return MapTile -> Map Tile structure
 */
MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY)
{
    static char tileFile[40] = "";
    uint32_t x = lon2tilex(lon, zoomLevel) + offsetX;
    uint32_t y = lat2tiley(lat, zoomLevel) + offsetY;

    sprintf(tileFile, mapFolder, zoomLevel, x, y);
    MapTile data;
    data.file = tileFile;
    data.tilex = x;
    data.tiley = y;
    data.zoom = zoomLevel;
    return data;
}

/**
 * @brief Generate render map
 *
 */
void generateRenderMap()
{
    currentMapTile = getMapTile(getLon(), getLat(), zoom, 0, 0);

    // Detects if tile changes from actual GPS position
    if (strcmp(currentMapTile.file, oldMapTile.file) != 0 ||
        currentMapTile.zoom != oldMapTile.zoom ||
        currentMapTile.tilex != oldMapTile.tilex ||
        currentMapTile.tiley != oldMapTile.tiley)
    {
        isMapFound  = mapTempSprite.drawPngFile(SD, currentMapTile.file, tileSize, tileSize);

        if (!isMapFound)
        {
            log_v("No Map Found!");
            oldMapTile.file = (char*)noMapFile;
            showNoMap(mapTempSprite);
        }
        else
        {
            log_v("Map Found!");
            oldMapTile.file = currentMapTile.file;

            static const uint8_t centerX = 0;
            static const uint8_t centerY = 0;
            int8_t startX = centerX - 1;
            int8_t startY = centerY - 1;

            for (int y = startY; y <= startY + 2; y++)
            {
                for (int x = startX; x <= startX + 2; x++)
                {
                    if (x == centerX && y == centerY)
                    {
                        // Skip Center Tile
                        continue;
                    }
                    roundMapTile = getMapTile(getLon(), getLat(), zoom, x, y);
                    mapTempSprite.drawPngFile(SD, roundMapTile.file, (x - startX) * tileSize, (y - startY) * tileSize);
                }

            }

            oldMapTile.zoom = currentMapTile.zoom;
            oldMapTile.tilex = currentMapTile.tilex;
            oldMapTile.tiley = currentMapTile.tiley;
        }

        log_v("TILE: %s", oldMapTile.file);
    }

    // Display
    mapSprite.pushSprite(0, 27);

    if (isMapFound)
    {
        // Draw Map in screen
        mapSprite.pushSprite(0, 27);

        navArrowPosition = coord2ScreenPos(getLon(), getLat(), zoom);
        mapTempSprite.setPivot(tileSize + navArrowPosition.posX, tileSize + navArrowPosition.posY);

        #ifdef ENABLE_COMPASS

        if (isMapRotation)
            mapHeading = heading;
        else
            mapHeading = GPS.course.deg();

        mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
        #else

        mapHeading = GPS.course.deg();
        mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
        // mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);

        #endif

        sprArrow.pushRotated(&mapSprite, 0, TFT_BLACK);
        drawMapWidgets();
    }
    else
        mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
}
