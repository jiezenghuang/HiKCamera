#include <fal/device/CameraArea.h>

namespace fal
{
    class HiKCameraPlugin : public CameraAreaPlugin
    {
    public:
        FAL_SHARED_PTR(HiKCameraPlugin);

        HiKCameraPlugin(const PluginProfile& profile);
        virtual ~HiKCameraPlugin();

        virtual int open() override;

        virtual bool isOpen() override;

        virtual int setExposure(int us) override;

        virtual int getExposure() override;

        virtual int setTriggerMode(bool on) override;
        
        virtual bool getTriggerMode() override;
        
        virtual int setTriggerSource(const std::string& source) override;

        virtual const std::string& getTriggerSource() override;
        
        virtual int trigger() override;
        
        virtual void close() override;

    protected:
        std::string file_, source_;
        bool open_, trigger_;
        int exposure_;
    };
}