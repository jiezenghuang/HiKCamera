#include "HiKCameraFactory.h"

#include <fal/common/Logger.h>
#include <fal/common/ErrorCategory.h>

#include "MvCameraControl.h"

using namespace fal;


HiKCameraFactory::HiKCameraFactory()
{
    profile_.type = PLUGIN_TYPE_CAMERA_AREA;
    profile_.name = "HiKCameraPlugin";            
    profile_.version = "1.0.0";
    profile_.author = "Wingtech";
    profile_.description = "HiKRobotic Camera Plugin";

    plugins_.emplace_back(std::make_shared<HiKCameraPlugin>(profile_));
}

HiKCameraFactory::~HiKCameraFactory() {}

int HiKCameraFactory::find(std::vector<PluginInstanceProfile>& profiles) 
{
    if(plugins_.empty())
        return EC_FAIL_NOT_FOUND;
    
    for(auto plugin : plugins_)
            profiles.push_back(plugin->iprofile());

    return EC_SUCESS;
}
    
PLUGIN_REGISTER("HiKCameraPlugin", HiKCameraFactory);
