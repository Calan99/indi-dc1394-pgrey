/**
 * INDI driver for Point Grey Chameleon USB2 camera
 *
 * Copyright (C) 2017 Andy Nikolenko
 * 				2013 Ben Gilsrud 
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef DC1394_PGREY_H
#define DC1394_PGREY_H

#include <indiccd.h>
#include <dc1394/dc1394.h>

using namespace std;

class DC1394_PGREY: public INDI::CCD
{
public:
    DC1394_PGREY();

    bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    void ISGetProperties(const char *dev);

protected:
    // General device functions
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

    // CCD specific functions
    bool StartExposure(float duration);
    bool AbortExposure();
    void TimerHit();
    /*void addFITSKeywords(fitsfile *fptr, CCDChip *targetChip);*/
    void addFITSKeywords(INDI::CCDChip *targetChip);
    bool UpdateCCDBin(int binx, int biny);

    IPState GuideNorth(float ms);
    IPState GuideSouth(float ms);
    IPState GuideWest(float ms);
    IPState GuideEast(float ms);

private:
    // Utility functions
    float CalcTimeLeft();
    void  setupParams();
    void  grabImage();
    float GetTemperature();

    // Are we exposing?
    bool InExposure;
    bool capturing;
    // Struct to keep timing
    struct timeval ExpStart;

    float ExposureRequest;
    float TemperatureRequest;
    int   timerID;
    
    uint32_t width;
    uint32_t height;
    
    float gain_min;
    float gain_max;
    
    dc1394video_mode_t selected_mode;
    
    bool temperatureCanRead;
    INumberVectorProperty SettingsNP;
    INumber SettingsN[1];
    
    // We declare the CCD temperature property
    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;
    
    dc1394_t *dc1394;
    dc1394camera_t *dcam;

};

#endif // DC1394_PGREY_H
