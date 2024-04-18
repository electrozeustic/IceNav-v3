/**
 * @file vector_maps.h
 * @author @aresta
 * @brief  Vector maps draw functions
 * @version 0.1.8
 * @date 2024-04
 */

#include <SPI.h>
#include <SD.h>
#include <StreamUtils.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <array>
#include <math.h>
#include <map>

/**
 * @brief Vector Map folder
 *
 */
const String baseFolder = "/mymap/"; // TODO: folder selection

/**
 * @brief Vectorized map size
 *
 */
#define MAP_HEIGHT 374
#define MAP_WIDTH 320

bool isPosMoved = false;

/**
 * @brief Vector map object colours
 *
 */
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;
// const uint16_t RED          =   0xFA45;
const uint16_t GREEN = 0x76EE;
const uint16_t GREENCLEAR = 0x9F93;
const uint16_t GREENCLEAR2 = 0xCF6E;
const uint16_t BLUE = 0x227E;
const uint16_t BLUECLEAR = 0x6D3E;
const uint16_t CYAN = 0xB7FF;
const uint16_t VIOLET = 0xAA1F;
const uint16_t ORANGE = 0xFCC2;
const uint16_t GRAY = 0x94B2;
const uint16_t GRAYCLEAR = 0xAD55;
const uint16_t GRAYCLEAR2 = 0xD69A;
const uint16_t BROWN = 0xAB00;
// const uint16_t YELLOW       =   0xFFF1;
const uint16_t YELLOWCLEAR = 0xFFF5;
const uint16_t BACKGROUND_COLOR = 0xEF5D;

/**
 * @brief Vector maps memory definition
 *
 */
#define MAPBLOCKS_MAX 6                                         // max blocks in memory
#define MAPBLOCK_SIZE_BITS 12                                   // 4096 x 4096 coords (~meters) per block
#define MAPFOLDER_SIZE_BITS 4                                   // 16 x 16 map blocks per folder
const int32_t MAPBLOCK_MASK = pow(2, MAPBLOCK_SIZE_BITS) - 1;   // ...00000000111111111111
const int32_t MAPFOLDER_MASK = pow(2, MAPFOLDER_SIZE_BITS) - 1; // ...00001111

#define DEG2RAD(a) ((a) / (180 / M_PI))
#define RAD2DEG(a) ((a) * (180 / M_PI))
#define EARTH_RADIUS 6378137
double lat2y(double lat) { return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS; }
double lon2x(double lon) { return DEG2RAD(lon) * EARTH_RADIUS; }

/**
 * @brief Point in 16 bits projected coordinates (x,y)
 *
 */
struct Point16
{
    Point16(){};
    Point16(int16_t x, int16_t y) : x(x), y(y){};
    Point16 operator-(const Point16 p) { return Point16(x - p.x, y - p.y); };
    Point16 operator+(const Point16 p) { return Point16(x + p.x, y + p.y); };
    Point16(char *coordsPair); // char array like:  11.222,333.44
    int16_t x;
    int16_t y;
};

/**
 * @brief Point in 32 bits projected coordinates (x,y)
 *
 */
struct Point32
{
    Point32(){};
    Point32(int32_t x, int32_t y) : x(x), y(y){};
    Point32(Point16 p) : x(p.x), y(p.y){};
    Point32 operator-(const Point32 p) { return Point32(x - p.x, y - p.y); };
    Point32 operator+(const Point32 p) { return Point32(x + p.x, y + p.y); };
    Point16 toPoint16() { return Point16(x, y); }; // TODO: check limits
    bool operator==(const Point32 p) { return x == p.x && y == p.y; };

    /// @brief Parse char array with the coordinates
    /// @param coordsPair char array like:  11.222,333.44

    int32_t x;
    int32_t y;
};

/**
 * @brief Bounding Box
 *
 */
struct BBox
{
    BBox(){};
    // @brief Bounding Box
    // @param min top left corner
    // @param max bottim right corner
    BBox(Point32 min, Point32 max) : min(min), max(max){};
    BBox operator-(const Point32 p) { return BBox(min - p, max - p); };
    bool containsPoint(const Point32 p);
    bool intersects(const BBox b);
    Point32 min;
    Point32 max;
};

/**
 * @brief Polyline
 *
 */
struct Polyline
{
    std::vector<Point16> points;
    BBox bbox;
    uint16_t color;
    uint8_t width;
    uint8_t maxZoom;
};

/**
 * @brief Poligon
 *
 */
struct Polygon
{
    std::vector<Point16> points;
    BBox bbox;
    uint16_t color;
    uint8_t maxZoom;
};

/**
 * @brief Vector map viewport
 *
 */
struct ViewPort
{
    void setCenter(Point32 pcenter);
    Point32 center;
    BBox bbox;
};

/**
 * @brief Set center coordinates of viewport
 *
 * @param pcenter
 */
void ViewPort::setCenter(Point32 pcenter)
{
    center = pcenter;
    bbox.min.x = pcenter.x - MAP_WIDTH * zoom / 2;
    bbox.min.y = pcenter.y - MAP_HEIGHT * zoom / 2;
    bbox.max.x = pcenter.x + MAP_WIDTH * zoom / 2;
    bbox.max.y = pcenter.y + MAP_HEIGHT * zoom / 2;
}

/**
 * @brief Points to screen coordinates
 *
 * @param pxy
 * @param screenCenterxy
 * @return int16_t
 */
int16_t toScreenCoord( const int32_t pxy, const int32_t screenCenterxy)
{
    return round((double)(pxy - screenCenterxy) / zoom) + (double)MAP_WIDTH / 2;
}

Point16::Point16(char *coordsPair)
{
    char *next;
    x = (int16_t)round(strtod(coordsPair, &next)); // 1st coord // TODO: change by strtol and test
    y = (int16_t)round(strtod(++next, NULL));       // 2nd coord
}

bool BBox::containsPoint(const Point32 p) { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }

bool BBox::intersects(BBox b)
{
    if (b.min.x > max.x ||
        b.max.x < min.x ||
        b.min.y > max.y ||
        b.max.y < min.y)
        return false;
    return true;
}

/**
 * @brief Map square area of aprox 4096 meters side. Correspond to one single map file.
 *
 */
struct MapBlock
{
    Point32 offset;
    // BBox bbox;
    bool inView = false;
    std::vector<Polyline> polylines;
    std::vector<Polygon> polygons;
};

/**
 * @brief MapBlocs memory store
 *
 */
struct MemCache
{
    std::vector<MapBlock *> blocks;
};

/**
 * @brief MapBlocks stored in memory
 *
 */
struct MemBlocks
{
    std::map<String, u_int16_t> blocks_map; // block offset -> block index
    std::array<MapBlock *, MAPBLOCKS_MAX> blocks;
};

/**
 * @brief Point in geografic (lat,lon) coordinates and other gps data
 *
 */
struct Coord
{
    Point32 getPoint32();
    double lat = 0;
    double lng = 0;
    int16_t altitude = 0;
    int16_t direction = 0;
    int16_t satellites = 0;
    bool isValid = false;
    bool isUpdated = false;
};

/**
 * @brief Vector file map memory blocks
 *
 */
MemBlocks memBlocks;

/**
 * @brief Vector map viewport
 *
 */
ViewPort viewPort;

/**
 * @brief Memory Cache
 *
 */
MemCache memCache;

/**
 * @brief Vector map GPS position point
 *
 */
Point32 point = viewPort.center;

/**
 * @brief Get vector map Position from GPS position and check if is moved
 *
 * @param lat
 * @param lon
 */
double prevLat = 0, prevLng = 0;
void getPosition(double lat, double lon)
{
    Coord pos;
    pos.lat = lat;
    pos.lng = lon;

    if (abs(pos.lat - prevLat) > 0.00005 &&
        abs(pos.lng - prevLng) > 0.00005)
    {
        point.x = lon2x(pos.lng);
        point.y = lat2y(pos.lat);
        prevLat = pos.lat;
        prevLng = pos.lng;
        isPosMoved = true;
        refreshMap = false;
    }
}

/**
 * @brief Returns int16 or 0 if empty
 *
 * @param file
 * @return int16_t
 */
int16_t parseInt16(ReadBufferingStream &file)
{
    char num[16];
    uint8_t i;
    char c;
    i = 0;
    c = (char)file.read();
    if (c == '\n')
        return 0;
    while (c >= '0' && c <= '9')
    {
        assert(i < 15);
        num[i++] = c;
        c = (char)file.read();
    }
    num[i] = '\0';
    if (c != ';' && c != ',' && c != '\n')
    {
        log_e("parseInt16 error: %c %i", c, c);
        log_e("Num: [%s]", num);
        while (1)
            ;
    }
    try
    {
        return std::stoi(num);
    }
    catch (std::invalid_argument)
    {
        log_e("parseInt16 invalid_argument: [%c] [%s]", c, num);
    }
    catch (std::out_of_range)
    {
        log_e("parseInt16 out_of_range: [%c] [%s]", c, num);
    }
    return -1;
}

/**
 * @brief Returns the string until terminator char or newline. The terminator character is not included but comsumed from stream.
 *
 * @param file
 * @param terminator
 * @param str
 */
void parseStrUntil(ReadBufferingStream &file, char terminator, char *str)
{
    uint8_t i;
    char c;
    i = 0;
    c = (char)file.read();
    while (c != terminator && c != '\n')
    {
        assert(i < 29);
        str[i++] = c;
        c = (char)file.read();
    }
    str[i] = '\0';
}

/**
 * @brief Parse vector file to coords
 *
 * @param file
 * @param points
 */
void parseCoords(ReadBufferingStream &file, std::vector<Point16> &points)
{
    char str[30];
    assert(points.size() == 0);
    Point16 point;
    while (true)
    {
        try
        {
            parseStrUntil(file, ',', str);
            if (str[0] == '\0')
                break;
            point.x = (int16_t)std::stoi(str);
            parseStrUntil(file, ';', str);
            assert(str[0] != '\0');
            point.y = (int16_t)std::stoi(str);
            // log_d("point: %i %i", point.x, point.y);
        }
        catch (std::invalid_argument)
        {
            log_e("parseCoords invalid_argument: %s", str);
        }
        catch (std::out_of_range)
        {
            log_e("parseCoords out_of_range: %s", str);
        }
        points.push_back(point);
    }
    // points.shrink_to_fit();
}

/**
 * @brief Parse Mapbox
 *
 * @param str
 * @return BBox
 */
BBox parseBbox(String str)
{
    char *next;
    int32_t x1 = (int32_t)strtol(str.c_str(), &next, 10);
    int32_t y1 = (int32_t)strtol(++next, &next, 10);
    int32_t x2 = (int32_t)strtol(++next, &next, 10);
    int32_t y2 = (int32_t)strtol(++next, NULL, 10);
    return BBox(Point32(x1, y1), Point32(x2, y2));
}

/**
 * @brief Read vector map file to memory block
 *
 * @param fileName
 * @return MapBlock*
 */
MapBlock *readMapBlock(String fileName)
{
    log_d("readMapBlock: %s", fileName.c_str());
    char c;
    char str[30];
    MapBlock *mblock = new MapBlock();
    fs::File file_ = SD.open(fileName + ".fmp");
    if (!file_)
    {
 //       header_msg("Map file not found in folder: " + baseFolder);
        while (true)
            ;
    }
    ReadBufferingStream file{file_, 2000};
    uint32_t line = 0;

    // read polygons
    parseStrUntil(file, ':', str);
    if (strcmp(str, "Polygons") != 0)
    {
        log_e("Map error. Expected Polygons instead of: %s", str);
        while (0)
            ;
    }
    int16_t count = parseInt16(file);
    assert(count > 0);
    line++;
    log_d("count: %i", count);

    uint32_t totalPoints = 0;
    Polygon polygon;
    Point16 p;
    int16_t maxZoom;
    while (count > 0)
    {
        // log_d("line: %i", line);
        parseStrUntil(file, '\n', str); // color
        assert(str[0] == '0' && str[1] == 'x');
        polygon.color = (uint16_t)std::stoul(str, nullptr, 16);
        // log_d("polygon.color: %i", polygon.color);
        line++;
        parseStrUntil(file, '\n', str); // maxZoom
        polygon.maxZoom = str[0] ? (uint8_t)std::stoi(str) : MAX_ZOOM;
        // log_d("polygon.maxZoom: %i", polygon.maxZoom);
        line++;

        parseStrUntil(file, ':', str);
        if (strcmp(str, "bbox") != 0)
        {
            log_e("bbox error tag. Line %i : %s", line, str);
            while (true)
                ;
        }
        polygon.bbox.min.x = parseInt16(file);
        polygon.bbox.min.y = parseInt16(file);
        polygon.bbox.max.x = parseInt16(file);
        polygon.bbox.max.y = parseInt16(file);

        line++;
        polygon.points.clear();
        parseStrUntil(file, ':', str);
        if (strcmp(str, "coords") != 0)
        {
            log_e("coords error tag. Line %i : %s", line, str);
            while (true)
                ;
        }
        parseCoords(file, polygon.points);
        line++;
        mblock->polygons.push_back(polygon);
        totalPoints += polygon.points.size();
        count--;
    }
    assert(count == 0);

    // read lines
    parseStrUntil(file, ':', str);
    if (strcmp(str, "Polylines") != 0)
        log_e("Map error. Expected Polylines instead of: %s", str);
    count = parseInt16(file);
    assert(count > 0);
    line++;
    log_d("count: %i", count);

    Polyline polyline;
    while (count > 0)
    {
        // log_d("line: %i", line);
        parseStrUntil(file, '\n', str); // color
        assert(str[0] == '0' && str[1] == 'x');
        polyline.color = (uint16_t)std::stoul(str, nullptr, 16);
        line++;
        parseStrUntil(file, '\n', str); // width
        polyline.width = str[0] ? (uint8_t)std::stoi(str) : 1;
        line++;
        parseStrUntil(file, '\n', str); // maxZoom
        polyline.maxZoom = str[0] ? (uint8_t)std::stoi(str) : MAX_ZOOM;
        line++;

        parseStrUntil(file, ':', str);
        if (strcmp(str, "bbox") != 0)
        {
            log_e("bbox error tag. Line %i : %s", line, str);
            while (true)
                ;
        }

        polyline.bbox.min.x = parseInt16(file);
        polyline.bbox.min.y = parseInt16(file);
        polyline.bbox.max.x = parseInt16(file);
        polyline.bbox.max.y = parseInt16(file);

        // if( line > 4050){
        //     log_e("polyline.bbox %i %i %i %i", polyline.bbox.min.x, polyline.bbox.min.y,polyline.bbox.max.x, polyline.bbox.max.y);
        // }
        line++;

        polyline.points.clear();
        parseStrUntil(file, ':', str);
        if (strcmp(str, "coords") != 0)
        {
            log_d("coords tag. Line %i : %s", line, str);
            while (true)
                ;
        }
        parseCoords(file, polyline.points);
        line++;
        // if( line > 4050 && fileName == "/mymap/3_77/6_9"){
        //     for( Point16 p: polyline.points){
        //         log_d("p.x, p.y %i %i", p.x, p.y);
        //     }
        // }
        mblock->polylines.push_back(polyline);
        totalPoints += polyline.points.size();
        count--;
    }
    assert(count == 0);
    file_.close();
    return mblock;
}

/**
 * @brief Get bounding objects in memory block
 *
 * @param memBlocks
 * @param bbox
 */
void getMapBlocks( BBox& bbox, MemCache& memCache)
{
    log_d("getMapBlocks %i", millis());
    for (MapBlock *block : memCache.blocks)
    {
        block->inView = false;
    }
    // loop the 4 corners of the bbox and find the files that contain them
    for (Point32 point : {bbox.min, bbox.max, Point32(bbox.min.x, bbox.max.y), Point32(bbox.max.x, bbox.min.y)})
    {
        bool found = false;
        int32_t blockMinX = point.x & (~MAPBLOCK_MASK);
        int32_t blockMinY = point.y & (~MAPBLOCK_MASK);

        // check if the needed block is already in memory
        for (MapBlock *memblock : memCache.blocks)
        {
            if (blockMinX == memblock->offset.x && blockMinY == memblock->offset.y)
            {
                memblock->inView = true;
                found = true;
                break;
            }
        }
        if (found)
            continue;

        log_d("load from disk (%i, %i) %i", blockMinX, blockMinY, millis());
        // block is not in memory => load from disk
        int32_t blockX = (blockMinX >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;
        int32_t blockY = (blockMinY >> MAPBLOCK_SIZE_BITS) & MAPFOLDER_MASK;
        int32_t folderNameX = blockMinX >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
        int32_t folderNameY = blockMinY >> (MAPFOLDER_SIZE_BITS + MAPBLOCK_SIZE_BITS);
        char folderName[12];
        snprintf(folderName, 9, "%+04d%+04d", folderNameX, folderNameY);         // force sign and 4 chars per number
        String fileName = baseFolder + folderName + "/" + blockX + "_" + blockY; //  /maps/123_456/777_888

        // check if cache is full
        if (memCache.blocks.size() >= MAPBLOCKS_MAX)
        {
            // remove first one, the oldest
            log_v("Deleteing freeHeap: %i", esp_get_free_heap_size());
            MapBlock *firstBlock = memCache.blocks.front();
            delete firstBlock;                             // free memory
            memCache.blocks.erase(memCache.blocks.begin()); // remove pointer from the vector
            log_v("Deleted freeHeap: %i", esp_get_free_heap_size());
        }

        MapBlock *newBlock = readMapBlock(fileName);
        newBlock->inView = true;
        newBlock->offset = Point32(blockMinX, blockMinY);
        memCache.blocks.push_back(newBlock); // add the block to the memory cache
        assert(memCache.blocks.size() <= MAPBLOCKS_MAX);

        log_d("Block readed from SD card: %p", newBlock);
        log_d("FreeHeap: %i", esp_get_free_heap_size());
    }
    log_d("memCache size: %i %i", memCache.blocks.size(), millis());
}

/**
 * @brief Fill polygon routine
 *
 * @param points
 * @param color
 */
void fillPoligon( Polygon p, TFT_eSprite &map) // scanline fill algorithm
{
    int16_t maxY = p.bbox.max.y;
    int16_t minY = p.bbox.min.y;

    if (maxY >= MAP_HEIGHT)
        maxY = MAP_HEIGHT - 1;
    if (minY < 0)
        minY = 0;
    if (minY >= maxY)
    {
        return;
    }
    int16_t nodeX[p.points.size()], pixelY;

    //  Loop through the rows of the image.
    int16_t nodes, i, swap;
    for (pixelY = minY; pixelY <= maxY; pixelY++)
    { //  Build a list of nodes.
        nodes = 0;
        for (int i = 0; i < (p.points.size() - 1); i++)
        {
            if ((p.points[i].y < pixelY && p.points[i + 1].y >= pixelY) ||
                (p.points[i].y >= pixelY && p.points[i + 1].y < pixelY))
            {
                nodeX[nodes++] =
                    p.points[i].x + double(pixelY - p.points[i].y) / double(p.points[i + 1].y - p.points[i].y) *
                                        double(p.points[i + 1].x - p.points[i].x);
            }
        }
        assert(nodes < p.points.size());

        //  Sort the nodes, via a simple “Bubble” sort.
        i = 0;
        while (i < nodes - 1)
        { // TODO: rework
            if (nodeX[i] > nodeX[i + 1])
            {
                swap = nodeX[i];
                nodeX[i] = nodeX[i + 1];
                nodeX[i + 1] = swap;
                i = 0;
            }
            else
            {
                i++;
            }
        }

        //  Fill the pixels between node pairs.
        for (i = 0; i <= nodes - 2; i += 2)
        {
            if (nodeX[i] > MAP_WIDTH)
                break;
            if (nodeX[i + 1] < 0)
                continue;
            if (nodeX[i] < 0)
                nodeX[i] = 0;
            if (nodeX[i + 1] > MAP_WIDTH)
                nodeX[i + 1] = MAP_WIDTH;
            map.drawLine(nodeX[i], MAP_HEIGHT - pixelY, nodeX[i + 1], MAP_HEIGHT - pixelY, p.color);
        }
    }
}

/**
 * @brief Generate vectorized map
 *
 * @param viewPort
 * @param memblocks
 * @param map -> Map Sprite
 */
void generateVectorMap(ViewPort &viewPort, MemCache& memCache, TFT_eSprite &map)
{
    Polygon newPolygon;
    map.fillScreen(BACKGROUND_COLOR);
    uint32_t totalTime = millis();
    log_d("Draw start %i", totalTime);
    int16_t p1x, p1y, p2x, p2y;
    for (MapBlock *mblock : memCache.blocks)
    {
        uint32_t blockTime = millis();
        if (!mblock->inView)
            continue;

        // block to draw
        Point16 screen_center_mc = viewPort.center.toPoint16() - mblock->offset.toPoint16(); // screen center with features coordinates
        BBox screen_bbox_mc = viewPort.bbox - mblock->offset;                                // screen boundaries with features coordinates

        ////// Polygons
        for (Polygon polygon : mblock->polygons)
        {
            if (zoom > polygon.maxZoom)
                continue;
            if (!polygon.bbox.intersects(screen_bbox_mc))
                continue;
            newPolygon.color = polygon.color;
            newPolygon.bbox.min.x = toScreenCoord(polygon.bbox.min.x, screen_center_mc.x);
            newPolygon.bbox.min.y = toScreenCoord(polygon.bbox.min.y, screen_center_mc.y);
            newPolygon.bbox.max.x = toScreenCoord(polygon.bbox.max.x, screen_center_mc.x);
            newPolygon.bbox.max.y = toScreenCoord(polygon.bbox.max.y, screen_center_mc.y);

            newPolygon.points.clear();
            for (Point16 p : polygon.points)
            { // TODO: move to fillPoligon
                newPolygon.points.push_back(Point16(
                    toScreenCoord(p.x, screen_center_mc.x),
                    toScreenCoord(p.y, screen_center_mc.y)));
            }
            fillPoligon(newPolygon, map);
        }
        log_d("Block polygons done %i ms", millis() - blockTime);
        blockTime = millis();

        ////// Lines
        for (Polyline line : mblock->polylines)
        {
            if (zoom > line.maxZoom)
                continue;
            if (!line.bbox.intersects(screen_bbox_mc))
                continue;

            p1x = -1;
            for (int i = 0; i < line.points.size() - 1; i++)
            { // TODO optimize
                p1x = toScreenCoord(line.points[i].x, screen_center_mc.x);
                p1y = toScreenCoord(line.points[i].y, screen_center_mc.y);
                p2x = toScreenCoord(line.points[i + 1].x, screen_center_mc.x);
                p2y = toScreenCoord(line.points[i + 1].y, screen_center_mc.y);
                // <                tft.drawWideLine(
                //                     p1x, SCREEN_HEIGHT - p1y,
                //                     p2x, SCREEN_HEIGHT - p2y,
                //                     line.width / zoomLevel ?: 1, line.color, line.color);>
                map.drawLine(
                    p1x, MAP_HEIGHT - p1y,
                    p2x, MAP_HEIGHT - p2y,
                    // line.width/zoom ?: 1, line.color, line.color);
                    line.color);
            }
        }
        log_d("Block lines done %i ms", millis() - blockTime);
    }
    log_d("Total %i ms", millis() - totalTime);

    // TODO: paint only in NAV mode
    map.fillTriangle(
        MAP_WIDTH / 2 - 4, MAP_HEIGHT / 2 + 5,
        MAP_WIDTH / 2 + 4, MAP_HEIGHT / 2 + 5,
        MAP_WIDTH / 2, MAP_HEIGHT / 2 - 6,
        RED);
    log_d("Draw done! %i", millis());
}