#include "cppgram/plugin.hpp"
#include "cppgram/client.hpp"

namespace cppgram {

bool PluginManager::register_plugin(std::shared_ptr<IPlugin> plugin, Client& client) {
    if (!plugin) return false;
    const std::string name = plugin->name();
    if (name.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (plugins_.find(name) != plugins_.end()) {
            return false;
        }
        plugins_[name] = plugin;
    }

    try {
        plugin->on_load(client);
    } catch (...) {
        std::lock_guard<std::mutex> lock(mtx_);
        plugins_.erase(name);
        throw;
    }
    return true;
}

bool PluginManager::unregister_plugin(const std::string& name, Client& client) {
    std::shared_ptr<IPlugin> target;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = plugins_.find(name);
        if (it == plugins_.end()) return false;
        target = it->second;
        plugins_.erase(it);
    }
    if (target) {
        try {
            target->on_unload(client);
        } catch (...) {
            // Suppress exception during unload
        }
    }
    return true;
}

bool PluginManager::has_plugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return plugins_.find(name) != plugins_.end();
}

std::shared_ptr<IPlugin> PluginManager::get_plugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = plugins_.find(name);
    if (it != plugins_.end()) return it->second;
    return nullptr;
}

std::vector<std::shared_ptr<IPlugin>> PluginManager::list_plugins() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::shared_ptr<IPlugin>> res;
    res.reserve(plugins_.size());
    for (const auto& [_, p] : plugins_) {
        res.push_back(p);
    }
    return res;
}

void PluginManager::unload_all(Client& client) {
    std::vector<std::shared_ptr<IPlugin>> all;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto& [_, p] : plugins_) {
            all.push_back(p);
        }
        plugins_.clear();
    }
    for (auto& p : all) {
        if (p) {
            try {
                p->on_unload(client);
            } catch (...) {
            }
        }
    }
}

} // namespace cppgram
