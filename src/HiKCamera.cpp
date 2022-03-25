#include "HiKCamera.h"

#include <fal/common/Logger.h>
#include <fal/common/ErrorCategory.h>
#include <fal/plugin/PluginManager.hpp>

using namespace fal;

HiKCameraPlugin::HiKCameraPlugin(const PluginProfile& profile)
    : CameraAreaPlugin(profile), open_(false), exposure_(1000), trigger_(true)
{
    iprofile_.name = "VirtualCameraAreaPlugin";
    iprofile_.vendor = "Wingtech";
    iprofile_.model = "C1";
    iprofile_.sn = "V123456789d";
    iprofile_.version = "1.21";
}

HiKCameraPlugin::~HiKCameraPlugin() {}

int HiKCameraPlugin::open() 
{            
    if(!open_)
        open_ = true;
        
    LOG_D("{} plugin {} open sucess", profile_.name, iprofile_.name);
    return EC_SUCESS;
}

bool HiKCameraPlugin::isOpen() 
{
    return open_;
}

int HiKCameraPlugin::setExposure(int us) 
{
    exposure_ = us;
    LOG_D("{} plugin {} exposure set to {}", profile_.name, iprofile_.name, exposure_);
    return EC_SUCESS;
}

int HiKCameraPlugin::getExposure() 
{
    return exposure_;
}

int HiKCameraPlugin::setTriggerMode(bool on) 
{
    trigger_ = on;
    LOG_D("{} plugin {} trigger mode set to {}", profile_.name, iprofile_.name, trigger_);
    return EC_SUCESS;
}

bool HiKCameraPlugin::getTriggerMode() 
{
    return trigger_;
}

int HiKCameraPlugin::setTriggerSource(const std::string& source) 
{
    source_ = source;
    LOG_D("{} plugin {} trigger source set to {}", profile_.name, iprofile_.name, source_);
    return EC_SUCESS;
}

const std::string& HiKCameraPlugin::getTriggerSource() 
{
    return source_;
}

int HiKCameraPlugin::trigger() 
{
    if(file_.empty())
    {
        LOG_E("{} plugin {} image file is empty", profile_.name, iprofile_.name);
        return EC_FAIL_NOT_FOUND;
    }

    cv::Mat image = cv::imread(file_, cv::IMREAD_UNCHANGED);
    if(image.empty())
    {
        LOG_E("{} plugin {} read image file is empty", profile_.name, iprofile_.name);
        return EC_FAIL_NOT_FOUND;
    }

    if(capture_callback_ != nullptr)
        capture_callback_(image, shared_from_this());
    
    LOG_D("{} plugin {} trigger sucess", profile_.name, iprofile_.name);
    return EC_SUCESS;
}

void HiKCameraPlugin::close() 
{
    open_ = false;
    LOG_D("{} plugin {} close", profile_.name, iprofile_.name);
}