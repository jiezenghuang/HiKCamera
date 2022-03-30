#include <fal/device/AbstractCameraPlugin.h>

#include "MvCameraControl.h"

namespace fal
{
    class HiKCameraPlugin : public AbstractCameraPlugin
    {
    public:
        FAL_SHARED_PTR(HiKCameraPlugin);

        HiKCameraPlugin(const PluginProfile& profile, void* handle);
        virtual ~HiKCameraPlugin();

        static void ParseProfile(const MV_CC_DEVICE_INFO& info, PluginInstanceProfile& profile);

        virtual int open() override;

        virtual bool isOpen() override;

        virtual int setExposure(int us) override;

        virtual int getExposure() override;

        virtual int setTriggerMode(bool on) override;
        
        virtual bool getTriggerMode() override;
        
        virtual int setTriggerSource(const std::string& source) override;

        virtual std::string getTriggerSource() override;
        
        virtual int trigger() override;
        
        virtual void close() override;

    protected:
        static void __stdcall ImageCallBack(unsigned char *pImage, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser);

    protected:
        std::string file_, source_;
        bool open_, trigger_;
        int exposure_;
        void* handle_;
        uint8_t* convert_buffer_;
        uint32_t convert_buffer_size_;
    };
}