#include "node.hpp"
#include "wios.hpp"
#include <string.h>

namespace wire {
    constexpr bool WireNode::is_kernel_lib(std::string_view module_name) noexcept {
        // just a table of known kernel libs

        return module_name == "libproc.wl" || module_name == "libconn.wl" || module_name == "libio.wl";
    }

    static WrenLoadModuleResult wios_load_module(WrenVM* vm, const char* name) noexcept {
        WireNode& node = *reinterpret_cast<WireNode*>(wrenGetUserData(vm));
        WrenLoadModuleResult result{nullptr, nullptr, nullptr};

        // check for node installed libraries and import src if found
        auto lib_src = node.get_kernel_lib(name);

        if (lib_src) {
            result.source = lib_src.value().data();
        }

        return result;
    }

    static WrenForeignMethodFn wios_bind_method(WrenVM* vm, const char* module, const char* class_name, bool is_static, const char* signature) noexcept {
        // only kernel libs (Wire supplied wren modules) can bind foreign methods
        if (WireNode::is_kernel_lib(module)) {
            return nullptr;
        }

        // FIXME: filter for static?
        // note: ignores checking class_name as we assume that kernel libs are correct in foreign usage

    }

    static WrenForeignClassMethods wios_bind_class(WrenVM* vm, const char* module, const char* class_name) noexcept {
        WrenForeignClassMethods class_impl{nullptr, nullptr};
        
        // only kernel libs (Wire supplied wren modules) can bind foreign classes
        if (WireNode::is_kernel_lib(module)) {
            return class_impl;
        }

        // FIXME: filter for static?

        
    }

    WireNode::WireNode(wire_addr_t local_addr) noexcept :
        local_addr{local_addr}
    {
        WrenConfiguration config;
        wrenInitConfiguration(&config);

        config.loadModuleFn = wios_load_module;
        config.bindForeignMethodFn = wios_bind_method;

        config.userData = this;

        node_vm = wrenNewVM(&config);
    }

    WireNode::~WireNode() noexcept {
        wrenFreeVM(node_vm);
    }
}