#include <fal/common/Logger.h>
#include <fal/common/ErrorCategory.h>
#include <fal/plugin/PluginManager.hpp>
#include <fal/device/CameraArea.h>

#include <iostream>

using namespace fal;

int main(int argc, const char** argv)
{
    PluginManager::Instance().load(".");
    if(PluginManager::Instance().size() == 0)
    {
        LOG_E("no plugin found");
        return EC_FAIL_NOT_FOUND;
    }

    std::vector<std::string> plugins;
    LOG_D("load {} plugins", PluginManager::Instance().size());
    PluginManager::Instance().find(plugins);
    for (auto plugin : plugins)
        LOG_D("plugin id: {}", plugin);

    plugins.clear();
    PluginManager::Instance().find(plugins, PLUGIN_TYPE_CAMERA_AREA);

    if(plugins.size() == 1)
    {        
        PluginFactory::Ptr factory = PluginManager::Instance().find(plugins.front());
        std::vector<PluginInstanceProfile> profiles;
        factory->find(profiles);
        CameraAreaPlugin::Ptr camera = factory->create<CameraAreaPlugin>(0);
        camera->open();
        camera->onCapture([](const cv::Mat& image, const AbstractPlugin::Ptr& camera){
            cv::namedWindow(camera->iprofile().model);
            cv::imshow(camera->iprofile().model, image);
        });
        std::cin.get();
        camera->close();
    }
    else
    {
        LOG_E("no camera plugin found");
        return EC_FAIL_NOT_FOUND;
    }
    
    return EC_SUCESS;
}