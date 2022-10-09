/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass definition and functions
 * @version 0.1
 * @date 2022-10-09
 */

int Read_Mag_data();

#ifdef ENABLE_COMPASS

#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(12345);
float declinationAngle = 0.2200;

/**
 * @brief Compass read delay
 *
 */
#define COMPASS_UPDATE_TIME 10
MyDelay COMPASStime(COMPASS_UPDATE_TIME);

/**
 * @brief Read compass data
 * 
 * @return int -> compass heading
 */
int Read_Mag_data()
{
    if (COMPASStime.update())
    {
        sensors_event_t event;
        compass.getEvent(&event);
        float heading = atan2(event.magnetic.y, event.magnetic.x);
        heading += declinationAngle;
        if (heading < 0)
            heading += 2 * PI;
        if (heading > 2 * PI)
            heading -= 2 * PI;
        return (int)(heading * 180 / M_PI);
    }
    else
        return 0;
}

#endif
