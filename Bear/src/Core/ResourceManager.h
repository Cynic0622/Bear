//
// Created by shw on 2025/8/5.
//

#pragma once
#include <string>
#include <unordered_map>
#include <memory>

namespace Bear
{
    template<typename TResource, typename TKey = std::string>
    class ResourceManager
    {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        template<typename... TArgs>
        std::shared_ptr<TResource> Load(const TKey& key, TArgs&&... args)
        {
            auto it = m_ResourceCache.find(key);
            if (it != m_ResourceCache.end())
            {
                return it->second;
            }

            // 没找到则新建资源
            std::shared_ptr<TResource> resource = std::make_shared<TResource>(std::forward<TArgs>(args)...);
            m_ResourceCache[key] = resource;

            return resource;
        }
    private:
        std::unordered_map<TKey, std::shared_ptr<TResource>> m_ResourceCache;
    };
}