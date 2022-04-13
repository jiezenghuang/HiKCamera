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
}

HiKCameraFactory::~HiKCameraFactory() {}

int HiKCameraFactory::find(std::vector<PluginInstanceProfile>& profiles) 
{
    MV_CC_DEVICE_INFO_LIST cameras = { 0 };
	int ec = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &cameras);
	if (ec != MV_OK)
	{
		LOG_E("find camera fail, error {:#x}", static_cast<uint32_t>(ec));
		return EC_FAIL_API_ERROR;
	}
	LOG_D("found {} cameras", cameras.nDeviceNum);

    plugins_.clear(); // 清空插件实例
    profiles.clear();
    for (uint32_t i = 0; i < cameras.nDeviceNum; ++i)
	{
        PluginInstanceProfile profile;
        profile.available = false;

		void* handle = nullptr;
        ec = MV_CC_CreateHandleWithoutLog(&handle, cameras.pDeviceInfo[i]);
        //ec = MV_CC_CreateHandle(&handle, cameras.pDeviceInfo[i]);
        if (ec != MV_OK)
        {
            LOG_W("create camera[{}] handle fail, error {:#x}", i, static_cast<uint32_t>(ec));
            continue;
        }

        HiKCameraPlugin::Ptr camera = std::make_shared<HiKCameraPlugin>(profile_, handle);
        plugins_.emplace_back(camera);   

        if (!MV_CC_IsDeviceAccessible(cameras.pDeviceInfo[i], MV_ACCESS_Exclusive))
        {
            LOG_W("camera[{}] is not accessible", i);  
            profile.available = false;     
        }
        else
            profile.available = true;

        HiKCameraPlugin::ParseProfile(*cameras.pDeviceInfo[i], profile);
        profiles.emplace_back(profile);
	}
    return EC_SUCESS;
}
    
PLUGIN_REGISTER("HiKCameraPlugin", HiKCameraFactory);
