#pragma once

#include <wren.hpp>
#include <cassert>

#include <map>
#include <string>
#include <optional>

namespace wire {
    // a tiny singleton class that loads all kernel lib sources on load time
    class KernSourceStorage {
    public:
        KernSourceStorage() noexcept;
        ~KernSourceStorage() noexcept;

        std::optional<std::string_view> get_kernel_src(std::string_view lib_name) const noexcept {
            auto iter = kernel_sources.find(std::hash<std::string_view>{}(lib_name));

            if (iter == kernel_sources.end()) {
                return std::nullopt;
            }

            return iter->second;
        }

        static KernSourceStorage& get_storage() noexcept { assert(storage_instance && "no storage initialized!"); return *storage_instance; }
    private:
        inline static KernSourceStorage* storage_instance = nullptr;

        void add_kernel_src(std::string_view lib_name) noexcept;

        std::map<size_t, std::string> kernel_sources; 
    };

    void wiosify_vm_config(WrenConfiguration* config) noexcept;
}