#include "yandex/contest/invoker/process_group/DefaultSettings.hpp"

#include "yandex/contest/invoker/ProcessGroup.hpp"

namespace yandex{namespace contest{namespace invoker{namespace process_group
{
    void DefaultSettings::setUpProcessGroup(const ProcessGroupPointer &processGroup) const
    {
        processGroup->setResourceLimits(resourceLimits);
        processGroup->setProcessDefaultSettings(processDefaultSettings);
    }
}}}}
