#include <fal/common/Logger.h>
#include <fal/common/ErrorCategory.h>
#include <fal/plugin/PluginManager.hpp>
#include <fal/device/AbstractCameraPlugin.h>

#include <iostream>

using namespace fal;

int main(int argc, const char** argv)
{
    Logger::Instance().file();
    if(argc > 1)
        PluginManager::Instance().load(argv[1]);
    else
        PluginManager::Instance().load(".");

    if(PluginManager::Instance().size() == 0)
    {
        LOG_E("no plugin found");
        return EC_FAIL_NOT_FOUND;
    }

    std::vector<std::string> plugins;
    LOG_D("total {} plugins", PluginManager::Instance().size());
    PluginManager::Instance().find(plugins, PLUGIN_TYPE_CAMERA_AREA);
    for (size_t i = 0; i < plugins.size(); ++i)
        LOG_D("camera area plugin: [{}]{}", i, plugins[i]);

    int index = 0, max = plugins.size() - 1;
    do
    {
        std::cout << fmt::format("Please select plugin index(0 ~ {}):", max);
        std::cin >> index;
        std::cin.ignore();
    } while (index > max);

    if(index < 0)
    {
        LOG_D("select nothing, exit");
        return EC_SUCESS;
    }
    
    LOG_D("select plugin[{}]: {}", index, plugins[index]);
    PluginFactory::Ptr factory = PluginManager::Instance().find(plugins[index]);
    if(factory == nullptr)
    {
        LOG_E("camera factory {} is null", plugins[index]);
        return EC_FAIL_NOT_FOUND;
    }

    std::vector<PluginInstanceProfile> profiles;
    factory->find(profiles);
    if(profiles.empty())
    {
        LOG_E("no camera plugin found");
        return EC_FAIL_NOT_FOUND;
    }

    for(auto profile : profiles)
        LOG_D("name:{}, sn:{}, vendor:{}, version:{}, model:{}, workspace:{}, available:{}", profile.name, profile.sn, 
            profile.vendor, profile.version, profile.model, profile.workspace, profile.available);

    AbstractCameraPlugin::Ptr camera = factory->create<AbstractCameraPlugin>(0);       
    camera->onCapture([&](const AbstractCameraPlugin* sendor, const cv::Mat& image){   
        cv::namedWindow(camera->iprofile().model);      
        cv::imshow(camera->iprofile().model, image);
        cv::waitKey();
    });
    camera->open();

    bool trigger = false;
    camera->setTriggerMode(trigger);
    if(trigger)
        camera->setTriggerSource("Software");

    camera->setExposure(0);
    camera->start();
    if(trigger)
        camera->trigger();
    std::cout << "Press any key to close";
    std::cin.get();

    camera->save("camera.json");
    camera->stop();
    camera->close();
    
    return EC_SUCESS;
}