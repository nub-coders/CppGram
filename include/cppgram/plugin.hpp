#pragma once

/**
 * @file plugin.hpp
 * @brief Modular plugin interfaces and registration system for CppGram.
 */

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

namespace cppgram {

class Client;

/**
 * @brief Abstract base class for self-contained CppGram plugins and command modules.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Unique identifier for the plugin.
     */
    virtual std::string name() const = 0;

    /**
     * @brief Semantic version of the plugin.
     */
    virtual std::string version() const { return "1.0.0"; }

    /**
     * @brief Brief description of the plugin's functionality.
     */
    virtual std::string description() const { return ""; }

    /**
     * @brief Lifecycle hook called when the plugin is registered with a Client instance.
     */
    virtual void on_load(Client& client) = 0;

    /**
     * @brief Lifecycle hook called when the plugin is unregistered from a Client instance.
     */
    virtual void on_unload(Client& client) { (void)client; }
};

/**
 * @brief Manages plugin registration, discovery, and lifecycle dispatching.
 */
class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager() = default;

    /**
     * @brief Registers and loads a plugin into the specified Client.
     * @return True if registered successfully; false if a plugin with the same name already exists.
     */
    bool register_plugin(std::shared_ptr<IPlugin> plugin, Client& client);

    /**
     * @brief Unregisters and unloads a plugin by name.
     * @return True if unloaded; false if not found.
     */
    bool unregister_plugin(const std::string& name, Client& client);

    /**
     * @brief Checks if a plugin is currently registered.
     */
    bool has_plugin(const std::string& name) const;

    /**
     * @brief Retrieves a registered plugin by name.
     */
    std::shared_ptr<IPlugin> get_plugin(const std::string& name) const;

    /**
     * @brief Returns a snapshot of all registered plugins.
     */
    std::vector<std::shared_ptr<IPlugin>> list_plugins() const;

    /**
     * @brief Unloads and clears all registered plugins.
     */
    void unload_all(Client& client);

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::shared_ptr<IPlugin>> plugins_;
};

} // namespace cppgram
