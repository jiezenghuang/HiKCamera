#include "HiKCamera.h"

#include <fal/plugin/PluginManager.hpp>

namespace fal
{
    class HiKCameraFactory : public PluginFactory
    {
    public:
        FAL_SHARED_PTR(HiKCameraFactory);

        HiKCameraFactory();

        virtual ~HiKCameraFactory();

        virtual int find(std::vector<PluginInstanceProfile>& profiles) override;
    };
}