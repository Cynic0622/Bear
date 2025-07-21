#pragma once

#include <optional>

namespace Bear {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        // std::optional<uint32_t> transferFamily; // 未来可以添加专门的传输队列

        bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
}