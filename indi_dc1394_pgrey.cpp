/**
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

#include <sys/time.h>
#include <memory>
#include <stdint.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

#include <indiapi.h>
#include <iostream>
#include "indi_dc1394_pgrey.h"
#include <dc1394/dc1394.h>

//const int POLLMS = 250;
//moved into initProperties

const float GAIN_DEFAULT = 1;

std::unique_ptr<DC1394_PGREY> dc1394_pgrey(new DC1394_PGREY());

void ISInit()
{
    static int isInit =0;
    if (isInit == 1)
        return;

    isInit = 1;
    if(dc1394_pgrey.get() == 0) dc1394_pgrey.reset(new DC1394_PGREY());
}

void ISGetProperties(const char * dev)
{
    ISInit();
    dc1394_pgrey->ISGetProperties(dev);
}

void ISNewSwitch(const char * dev, const char * name, ISState * states, char * names[], int num)
{
    ISInit();
    dc1394_pgrey->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char * dev, const char * name, char * texts[], char * names[], int num)
{
    ISInit();
    dc1394_pgrey->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char * dev, const char * name, double values[], char * names[], int num)
{
    ISInit();
    dc1394_pgrey->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char * dev, const char * name, int sizes[], int blobsizes[], char * blobs[], char * formats[], char * names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle * root)
{
    ISInit();
    dc1394_pgrey->ISSnoopDevice(root);
}


DC1394_PGREY::DC1394_PGREY()
{
    InExposure = false;
    capturing = false;
}


bool DC1394_PGREY::Connect()
{
    dc1394camera_list_t * list;
    dc1394error_t err;
    bool supported;
    bool settings_valid;
    uint32_t val;
    dc1394format7mode_t fm7;
    dc1394feature_info_t feature;
    float min, max, temp;
    dc1394video_modes_t modes;
    dc1394framerates_t framerates;

    dc1394 = dc1394_new();
    if (!dc1394)
    {
        return false;
    }

    err = dc1394_camera_enumerate(dc1394, &list);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not find DC1394 cameras!");
        return false;
    }
    if (!list->num)
    {
        IDMessage(getDeviceName(), "No DC1394 cameras found!");
        return false;
    }
    dcam = dc1394_camera_new(dc1394, list->ids[0].guid);
    if (!dcam)
    {
        IDMessage(getDeviceName(), "Unable to connect to camera!");
        return false;
    }

    /* Reset camera */
    err = dc1394_camera_reset(dcam);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to reset camera!");
        return false;
    }

    err = dc1394_video_get_supported_modes(dcam, &modes);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to get list of supported modes");
        return false;
    }
    IDMessage(getDeviceName(), "Number of Supported modes: %d",modes.num);

    // selected_mode = modes.modes[modes.num-1];

    //selected_mode = DC1394_VIDEO_MODE_1280x960_MONO16; 	// DC1394_VIDEO_MODE_640x480_MONO16 ;
    
    selected_mode = DC1394_VIDEO_MODE_FORMAT7_1;	//resolution no greater than 640x480, pixel format MONO16 according to technical specifications by manufacturer

    IDMessage(getDeviceName(), "Current mode: %d",selected_mode);

    err = dc1394_format7_set_image_position(dcam,selected_mode, 0, 0);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set image upper left corner position");
        return false;
    }


    err = dc1394_format7_set_image_size(dcam,selected_mode,640,480);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set format7 image size");
        return false;
    }
 
    err = dc1394_format7_set_color_coding(dcam,selected_mode,DC1394_COLOR_CODING_MONO8);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set format7 color coding");
        return false;
    }
    //Apparently, framerates make sense only with non-scalable video formats. Timestamp: 20230409
    /*
    err = dc1394_video_get_supported_framerates(dcam,selected_mode,&framerates);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not get frame rates");
        return false;
    }


    err = dc1394_video_get_supported_framerates(dcam,selected_mode,&framerates);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not get frame rates");
        return false;
    }

    IDMessage(getDeviceName(), "Frame Rates");
    float f_rate;
    for( int j = 0; j < framerates.num; j++ )
    {
        dc1394framerate_t rate = framerates.framerates[j];
        dc1394_framerate_as_float(rate,&f_rate);
        IDMessage(getDeviceName(), "  [%d] rate = %f\n",j,f_rate);
    }
    */

    err = dc1394_video_set_mode(dcam, selected_mode);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to connect to set videomode!");
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION,  "Connected in format7");

    err = dc1394_get_image_size_from_video_mode(dcam, selected_mode, &width,&height);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to get mode size!");
        return false;
    }

    IDMessage(getDeviceName(), "Current Mode frame width=%d, height=%d",width,height);

    /* Disable Auto exposure control */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_EXPOSURE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable auto exposure control");
        return false;
    }

    /* Set frame rate to the lowest possible */
    //err = dc1394_video_set_framerate(dcam, DC1394_FRAMERATE_7_5);
    //Again, framerate here does not make sense. Timestamp: 20230409
    /*
    err = dc1394_video_set_framerate(dcam, DC1394_FRAMERATE_1_875);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to connect to set framerate!");
        return false;
    }
    */
    /* Turn frame rate control off to enable extended exposure */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_FRAME_RATE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable framerate!");
        return false;
    }

    /* Get the longest possible exposure length */
    err = dc1394_feature_set_mode(dcam, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_MANUAL);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable manual shutter control.");
    }
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_SHUTTER, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable absolute shutter control.");
    }
    err = dc1394_feature_get_absolute_boundaries(dcam, DC1394_FEATURE_SHUTTER, &min, &max);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not get max shutter length");
    }
    else
    {
        IDMessage(getDeviceName(), "Min exposure = %f, Max = %f",min,max);
    }


    /* Set absolute gain control */
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_GAIN, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable absolute gain control.");
    }

    /* get and save min/max gain values */
    err = dc1394_feature_get_absolute_boundaries(dcam, DC1394_FEATURE_GAIN, &gain_min, &gain_max);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not get max gain value");
    }
    else
    {
        IDMessage(getDeviceName(), "Min gain = %f, Max = %f",gain_min,gain_max);
    }

    /* Set brightness */
    err = dc1394_feature_set_mode(dcam, DC1394_FEATURE_BRIGHTNESS, DC1394_FEATURE_MODE_MANUAL);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable manual brightness control.");
    }
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_BRIGHTNESS, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable absolute brightness control.");
    }
    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_BRIGHTNESS, 1);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set max brightness value");
    }

    /* Turn gamma control off */
    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_GAMMA, 1);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set gamma value");
    }
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_GAMMA, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable gamma!");
        return false;
    }

    /* Turn off white balance */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_WHITE_BALANCE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable white balance!");
        return false;
    }

    /* try to read temperature sensor and store flag if it's possible */
    if((temp = GetTemperature()) >= 0)
    {
        IDMessage(getDeviceName(), "Device Temperature : %.2f (C)", temp  );
        temperatureCanRead = true;
    }
    else
    {
        temperatureCanRead = false;
    }

    err = dc1394_capture_setup(dcam,10, DC1394_CAPTURE_FLAGS_DEFAULT);

    return true;
}



float DC1394_PGREY::GetTemperature()
{

    dc1394error_t err;
    uint32_t val;

    err = dc1394_get_control_register(dcam, 0x82c, &val);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to access Temperature register");
        return -1;
    }

    if(val & 0x80000000)
    {
        return (float)(val << 20 >> 20)/10 - 273.15;
    }

    IDMessage(getDeviceName(), "Could not read Temperature (register value = %x)",val );
    return -1;

}

bool DC1394_PGREY::Disconnect()
{
    if (dcam)
    {
        dc1394_capture_stop(dcam);
        dc1394_camera_free(dcam);
        temperatureCanRead = false;
    }

    IDMessage(getDeviceName(), "Point Grey Chameleon disconnected successfully!");
    return true;
}

const char * DC1394_PGREY::getDefaultName()
{
    return "Point Grey Chameleon";
}

bool DC1394_PGREY::initProperties()
{

    // Must init parent properties first!
    INDI::CCD::initProperties();

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();

    // Add Gain control
    IUFillNumberVector(&SettingsNP, SettingsN, 1, getDeviceName(), "GAIN", "Gain settings", MAIN_CONTROL_TAB, IP_RW, 1, IPS_IDLE);

    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Camera Temp. (C)", "%.2f", -50, 70, 0.1, 0);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "Temperature", "Temp.", MAIN_CONTROL_TAB, IP_RO, 1, IPS_IDLE);

    setDefaultPollingPeriod(250);

    return true;
}

void DC1394_PGREY::ISGetProperties(const char * dev)
{
    INDI::CCD::ISGetProperties(dev);

}

bool DC1394_PGREY::updateProperties()
{
    // Call parent update properties first
    INDI::CCD::updateProperties();

    if (isConnected())
    {

        setupParams();

        // Start the timer
        SetTimer(POLLMS);
        // Set gain GUI control to real min/max gain values
        IUFillNumber(&SettingsN[0], "GAIN_VALUE", "Camera Gain (dB)", "%.2f", gain_min, gain_max, (gain_max-gain_min)/99, GAIN_DEFAULT);

        defineNumber(&SettingsNP);
        defineNumber(&TemperatureNP);
    }
    else
    {
        deleteProperty(SettingsNP.name);
        deleteProperty(TemperatureNP.name);
    }

    return true;
}

bool DC1394_PGREY::UpdateCCDBin(int binx, int biny)
{

    if (binx != 1 || biny != 1)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Camera currently does not support binning.");
        return false;
    }
    return true;
}

void DC1394_PGREY::setupParams()
{

    float temp;

    // The Pointgrey Chameleon has Sony ICX445 CCD sensor
    SetCCDParams(width, height, 8, 7.5, 7.5);

    // How much memory we need for the frame buffer
    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP()/8;
    //DEBUG(INDI::Logger::DBG_SESSION, "xres:%d, yres:%d", PrimaryCCD.getXres, PrimaryCCD.getYres);
    nbuf += 512; //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

}


bool DC1394_PGREY::AbortExposure()
{
    InExposure = false;
    return true;
}

float DC1394_PGREY::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now,NULL);

    timesince=(double)(now.tv_sec * 1000.0 + now.tv_usec/1000) - (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec/1000);
    timesince=timesince/1000;

    timeleft=ExposureRequest-timesince;
    return timeleft;
}

bool DC1394_PGREY::ISNewNumber(const char * dev, const char * name, double values[], char * names[], int n)
{

    INumber * np;
    float c_gain, temp;
    dc1394error_t err;

    if (!strcmp(dev, getDeviceName()))
    {
        if (!strcmp(name, SettingsNP.name))
        {
            err = dc1394_feature_get_absolute_value(dcam, DC1394_FEATURE_GAIN, &c_gain);
            if (err != DC1394_SUCCESS)
            {
                IDMessage(getDeviceName(), "Could not get gain ");
            }
            SettingsN[0].value = c_gain;
            if(IUUpdateNumber(&SettingsNP, values, names,n ) < 0)
            {
                IDMessage(getDeviceName(), "Cannot update Gain settings");
                return false;
            }
            SettingsNP.s=IPS_OK;
            c_gain = SettingsN[0].value;
            err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_GAIN, c_gain);
            if (err != DC1394_SUCCESS)
            {
                IDMessage(getDeviceName(), "Could not Set gain ");
            }
            else
            {
                IDMessage(getDeviceName(), "Gain updated, value = %f", SettingsN[0].value);
            }
            IDSetNumber(&SettingsNP, NULL);

            return true;
        }
        else if(!strcmp(name,TemperatureNP.name))
        {
            if((temp = GetTemperature()) >= 0)
            {
                TemperatureN[0].value = temp;
                IDMessage(getDeviceName(), "New temp set ");
                IDSetNumber(&TemperatureNP, NULL);
            }
            return true;
        }
    }

    // If we didn't process anything above, let the parent handle it.
    return INDI::CCD::ISNewNumber(dev,name,values,names,n);
}



bool DC1394_PGREY::ISNewSwitch(const char * dev, const char * name, ISState * states, char * names[], int n)
{

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}


/*void DC1394_PGREY::addFITSKeywords(fitsfile * fptr, CCDChip * targetChip)
{
    // Let's first add parent keywords
    INDI::CCD::addFITSKeywords(fptr, targetChip);
}*/


/////////////////////////////////////////////////////////
/// Add applicable FITS keywords to header (copied from atikccd driver)
/////////////////////////////////////////////////////////
void DC1394_PGREY::addFITSKeywords(INDI::CCDChip *targetChip)
{
    INDI::CCD::addFITSKeywords(targetChip);

    /*if (m_isHorizon)
    {
        fitsKeywords.push_back({"GAIN", ControlN[CONTROL_GAIN].value, 3, "Gain"});
        fitsKeywords.push_back({"OFFSET", ControlN[CONTROL_OFFSET].value, 3, "Offset"});
    }*/
}


void DC1394_PGREY::TimerHit()
{
    long timeleft;
    int timerID = -1;
    float temp;

    if(isConnected() == false)
    {
        return;  //  No need to reset timer if we are not connected anymore
    }

    if (InExposure)
    {
        timeleft = CalcTimeLeft();
        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more - just set a tighter timer
                timerID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    timerID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    if (timeleft > 0)
                    {
                        int slv;
                        slv = 100000 * timeleft;
                        usleep(slv);
                    }

                    /* We're done exposing */
                    DEBUG(INDI::Logger::DBG_SESSION,  "Exposure done, downloading image...");

                    PrimaryCCD.setExposureLeft(0);
                    InExposure = false;
                    /* grab and save image */
                    grabImage();
                }
            }
        }
        else
        {
            if (isDebug())
            {
                IDLog("With time left %ld\n", timeleft);
                IDLog("image not yet ready....\n");
            }
            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    // read temperature sensor (if enabled)
    if(temperatureCanRead && (temp = GetTemperature()) >= 0)
    {
        TemperatureN[0].value = temp;
        IDSetNumber(&TemperatureNP, NULL);
    }


    if (timerID == -1)
        SetTimer(POLLMS);

    return;
}

/*
void DC1394_PGREY::grabImage()
{
    unsigned char * myimage;
    dc1394error_t err;
    dc1394video_frame_t * frame;
    uint32_t uheight, uwidth;
    uint16_t val;
    struct timeval start, end;

    DEBUG(INDI::Logger::DBG_SESSION,  "Grabber called");
    // Let's get a pointer to the frame buffer
    unsigned char * image = PrimaryCCD.getFrameBuffer();

    // Get width and height
    int width = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

	DEBUG(INDI::Logger::DBG_SESSION,  "Got width and heigth");

    memset(image, 0, PrimaryCCD.getFrameBufferSize());

    DEBUG(INDI::Logger::DBG_SESSION,  "memset");

    gettimeofday(&start, NULL);

    DEBUG(INDI::Logger::DBG_SESSION,  "Time");

    err=dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_WAIT, &frame);
    DEBUG(INDI::Logger::DBG_SESSION,  "Set err");
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not capture frame");
    }
    DEBUG(INDI::Logger::DBG_SESSION,  "Frame captured");
    //dc1394_get_image_size_from_video_mode(dcam,DC1394_VIDEO_MODE_1280x960_MONO16, &uwidth, &uheight);
    dc1394_get_image_size_from_video_mode(dcam,selected_mode, &uwidth, &uheight);
    if (DC1394_TRUE == dc1394_capture_is_frame_corrupt(dcam, frame))
    {
        IDMessage(getDeviceName(), "Corrupt frame!");
        return ;
    }

    memcpy(image,frame->image,height*width*2);
    //memcpy(image,frame->image,height*width); 20230410

    // release buffer
    dc1394_capture_enqueue(dcam, frame);

    dc1394_video_set_transmission(dcam,DC1394_OFF);

    IDMessage(getDeviceName(), "Download complete.");
    gettimeofday(&end, NULL);
    IDMessage(getDeviceName(), "Download took %.2f s", (float)((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))/ 1000000);

    // Let INDI::CCD know we're done filling the image buffer
    ExposureComplete(&PrimaryCCD);
}


bool DC1394_PGREY::StartExposure(float duration)
{

    dc1394error_t err;
    float fval, temp;
    dc1394video_frame_t * frame;

    ExposureRequest = duration;

    // Since we have only have one CCD with one chip, we set the exposure duration of the primary CCD
    PrimaryCCD.setBPP(16);
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart,NULL);

    InExposure = true;

    IDMessage(getDeviceName(), "Triggering a %f second exposure ",duration);

    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_SHUTTER, duration);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to set shutter value.");
    }
    err = dc1394_feature_get_absolute_value(dcam, DC1394_FEATURE_SHUTTER, &fval);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to get shutter value.");
    }
    IDMessage(getDeviceName(), "Set shutter value to %f.", fval);


     //Flush the DMA buffer 
    while (1)
    {
        err=dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_POLL, &frame);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Flushing DMA buffer failed!");
            break;
        }
        if (!frame)
        {
            break;
        }
        dc1394_capture_enqueue(dcam, frame);
    }


    IDMessage(getDeviceName(), "start transmission");
    err = dc1394_video_set_transmission(dcam, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to start transmission");
        return false;
    }
    //
    //    if(temperatureCanRead && (temp = GetTemperature()) >= 0) {
    //		TemperatureN[0].value = temp;
    //		IDSetNumber(&TemperatureNP, NULL);
    //	}
    //
    // actual grabbing to do in grabImage
    return true;
}

IPState DC1394_PGREY::GuideNorth(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideSouth(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideEast(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideWest(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}*/

void DC1394_PGREY::grabImage()
{
    unsigned char * myimage;
    dc1394error_t err;
    dc1394video_frame_t * frame;
    uint32_t uheight, uwidth;
    uint16_t val;
    struct timeval start, end;

    // Let's get a pointer to the frame buffer
    unsigned char * image = PrimaryCCD.getFrameBuffer();

    // Get width and height
    int width = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();
    IDMessage(getDeviceName(), "Size: (%d,%d)", width, height);

    memset(image, 0, PrimaryCCD.getFrameBufferSize());

    gettimeofday(&start, NULL);

    err=dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not capture frame");
    }
    //dc1394_get_image_size_from_video_mode(dcam,DC1394_VIDEO_MODE_1280x960_MONO16, &uwidth, &uheight);
    dc1394_get_image_size_from_video_mode(dcam,selected_mode, &uwidth, &uheight);
    if (DC1394_TRUE == dc1394_capture_is_frame_corrupt(dcam, frame))
    {
        IDMessage(getDeviceName(), "Corrupt frame!");
        return ;
    }

    memcpy(image,frame->image,height*width);

    // release buffer
    dc1394_capture_enqueue(dcam, frame);

    dc1394_video_set_transmission(dcam,DC1394_OFF);

    IDMessage(getDeviceName(), "Download complete.");
    gettimeofday(&end, NULL);
    IDMessage(getDeviceName(), "Download took %.2f s", (float)((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))/ 1000000);

    // Let INDI::CCD know we're done filling the image buffer
    ExposureComplete(&PrimaryCCD);
}

bool DC1394_PGREY::StartExposure(float duration)
{

    dc1394error_t err;
    float fval, temp;
    dc1394video_frame_t * frame;

    ExposureRequest = duration;

    // Since we have only have one CCD with one chip, we set the exposure duration of the primary CCD
    //PrimaryCCD.setBPP(16);
    PrimaryCCD.setBPP(8);
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart,NULL);

    InExposure = true;

    IDMessage(getDeviceName(), "Triggering a %f second exposure ",duration);

    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_SHUTTER, duration);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to set shutter value.");
    }
    err = dc1394_feature_get_absolute_value(dcam, DC1394_FEATURE_SHUTTER, &fval);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to get shutter value.");
    }
    IDMessage(getDeviceName(), "Set shutter value to %f.", fval);


    /* Flush the DMA buffer */
    while (1)
    {
        err=dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_POLL, &frame);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Flushing DMA buffer failed!");
            break;
        }
        if (!frame)
        {
            break;
        }
        dc1394_capture_enqueue(dcam, frame);
    }


    IDMessage(getDeviceName(), "start transmission");
    err = dc1394_video_set_transmission(dcam, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to start transmission");
        return false;
    }
    /*
        if(temperatureCanRead && (temp = GetTemperature()) >= 0) {
    		TemperatureN[0].value = temp;
    		IDSetNumber(&TemperatureNP, NULL);
    	}
    */
    // actual grabbing to do in grabImage
    return true;
}

IPState DC1394_PGREY::GuideNorth(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideSouth(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideEast(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}

IPState DC1394_PGREY::GuideWest(float ms)
{
    INDI_UNUSED(ms);
    return IPS_OK;
}
