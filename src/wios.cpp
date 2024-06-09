#include "wios.hpp"
#include "node.hpp"
#include "logger.hpp"

#include <span>
#include <cmath>
#include <cstring>
#include <string>
#include <format>

#include <iostream>
#include <fstream>
#include <sstream>

// This file contains all Wire/WiOS wren foreign function bindings

namespace wire {
    KernSourceStorage::KernSourceStorage() noexcept {
        add_kernel_src("libproc");
        add_kernel_src("wios_init");
        
        assert(!storage_instance && "instance already exists");
        storage_instance = this;
    }

    KernSourceStorage::~KernSourceStorage() noexcept {
        storage_instance = nullptr;   
    }

    void KernSourceStorage::add_kernel_src(std::string_view lib_name) noexcept {
        std::ifstream lib(std::format("kern_src/{}.wren", lib_name));
        
        if (!lib.is_open()) {
            log_err(std::format("failed to open kernel lib source file \"{}.wren\"", lib_name));

            std::abort();
        }

        std::stringstream lib_buf;
        lib_buf << lib.rdbuf();

        kernel_sources.emplace(std::hash<std::string_view>{}(lib_name), lib_buf.str());
    }

    static void wios_init_libproc(WrenVM* vm) noexcept {
        WireNode::WiOSProc& proc = *reinterpret_cast<WireNode::WiOSProc*>(wrenGetUserData(vm));
        
        wrenEnsureSlots(vm, 2);
        wrenGetVariable(vm, "libproc", "ProcInfo", 1);
        proc.proc_info_class_handle = wrenGetSlotHandle(vm, 1);
    }

    // used as the data for a WiOS process class `foreign class ProcInfo`
    struct WiOSForeignProcInfo {
        std::string proc_name;

        wire_timestamp_t proc_uptime;
        wire_pid_t proc_id;

        static void name(WrenVM* vm) noexcept {
            WiOSForeignProcInfo* info = reinterpret_cast<WiOSForeignProcInfo*>(wrenGetSlotForeign(vm, 0));
            wrenSetSlotBytes(vm, 0, info->proc_name.data(), info->proc_name.size());
        }

        static void uptime(WrenVM* vm) noexcept {
            WiOSForeignProcInfo* info = reinterpret_cast<WiOSForeignProcInfo*>(wrenGetSlotForeign(vm, 0));
            wrenSetSlotDouble(vm, 0, info->proc_uptime);
        }

        static void pid(WrenVM* vm) noexcept {
            WiOSForeignProcInfo* info = reinterpret_cast<WiOSForeignProcInfo*>(wrenGetSlotForeign(vm, 0));
            wrenSetSlotDouble(vm, 0, info->proc_id);
        }
    };

    static void wios_proc_info_allocate(WrenVM* vm) noexcept {
        WiOSForeignProcInfo* proc_info = reinterpret_cast<WiOSForeignProcInfo*>(wrenSetSlotNewForeign(vm, 1, 3, sizeof(WiOSForeignProcInfo)));
        assert(false);
    }

    static void wios_proc_info_finalize(void* data) noexcept {
        WiOSForeignProcInfo& proc_info = *reinterpret_cast<WiOSForeignProcInfo*>(data);
        proc_info.~WiOSForeignProcInfo();
    }

    // static proc_list()
    static void wios_proc_list(WrenVM* vm) noexcept {
        WireNode::WiOSProc& proc = *reinterpret_cast<WireNode::WiOSProc*>(wrenGetUserData(vm));

        wrenEnsureSlots(vm, 4);

        const std::map<wire_pid_t, WireNode::WiOSProc>& procs = proc.host_node->proc_list();
        wrenSetSlotNewList(vm, 0);

        // load ProcInfo foreign class handle for new instance creation 
        if (!proc.proc_info_class_handle) { wios_init_libproc(vm); }
        
        wrenSetSlotHandle(vm, 3, proc.proc_info_class_handle);

        for (auto& proc_instance : procs) {
            WiOSForeignProcInfo* proc_info = reinterpret_cast<WiOSForeignProcInfo*>(wrenSetSlotNewForeign(vm, 1, 3, sizeof(WiOSForeignProcInfo)));
            
            // init ProcInfo data
            proc_info->proc_name = proc_instance.second.proc_name;

            proc_info->proc_uptime = proc.host_node->timestamp() - proc_info->proc_uptime;
            proc_info->proc_id = proc_instance.first;

            wrenInsertInList(vm, 0, -1, 1);
        }
    }

    static void wios_exec(WrenVM* vm) noexcept {
        WireNode::WiOSProc& proc = *reinterpret_cast<WireNode::WiOSProc*>(wrenGetUserData(vm));

        if (wrenGetSlotType(vm, 1) != WREN_TYPE_STRING || wrenGetSlotType(vm, 2) != WREN_TYPE_LIST /*TODO: might allow Null as none*/) {
            wrenSetSlotString(vm, 0, "WiOS Error: mistyped call arguments, must use String for path and List for args");
            wrenAbortFiber(vm, 0);
            
            return;
        }

        int size;
        const char* data = wrenGetSlotBytes(vm, 1, &size);
        std::string_view src_path(data, size);

        proc.host_node->exec(vm, src_path);
    }

    static void wios_kill(WrenVM* vm) noexcept {
        WireNode::WiOSProc& proc = *reinterpret_cast<WireNode::WiOSProc*>(wrenGetUserData(vm));

        if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) {
            wrenSetSlotString(vm, 0, "WiOS Error: kill() pid argument must be a non-negative integer Num");
            wrenAbortFiber(vm, 0);
            
            return;
        }

        double dpid = wrenGetSlotDouble(vm, 1);
        
        if (std::fmod(dpid, 1) != dpid) {
            wrenSetSlotString(vm, 0, "WiOS Error: kill() pid argument must be a non-negative integer Num");
            wrenAbortFiber(vm, 0);
            
            return;
        }
        
        wire_pid_t pid = static_cast<wire_pid_t>(dpid);

        proc.host_node->kill(vm, pid);
    }

    static WrenLoadModuleResult wios_load_module(WrenVM* vm, const char* name) noexcept {
        WrenLoadModuleResult result{nullptr, nullptr, nullptr};

        // check for node installed libraries and import src if found
        auto lib_src = KernSourceStorage::get_storage().get_kernel_src(name);

        if (lib_src) {
            result.source = lib_src.value().data();
        }

        return result;
    }

    static WrenForeignMethodFn wios_bind_method(WrenVM* vm, const char* module, const char* class_name, bool is_static, const char* signature) noexcept {
        // only kernel libs (Wire supplied wren modules) can bind foreign methods
        if (!WireNode::is_kernel_lib(module)) {
            return nullptr;
        }

        // FIXME: filter for static?
        // note: ignores checking class_name as we assume that kernel libs are correct in foreign usage

        if (strcmp(class_name, "ProcInfo") == 0) {
            if (strcmp(signature, "name") == 0) {
                return WiOSForeignProcInfo::name;
            } else if (strcmp(signature, "uptime") == 0) {
                return WiOSForeignProcInfo::uptime;
            } else if (strcmp(signature, "pid") == 0) {
                return WiOSForeignProcInfo::pid;
            }
        } else if (strcmp(class_name, "proc") == 0) {
            if (strcmp(signature, "proc_list()") == 0) {
                return wios_proc_list;
            } else if (strcmp(signature, "exec(_,_)") == 0) {
                return wios_exec;
            } else if (strcmp(signature, "kill(_)") == 0) {
                return wios_kill;
            }
        }

        assert(false && "failed to bind a kernel lib method");
        return nullptr;
    }

    static WrenForeignClassMethods wios_bind_class(WrenVM* vm, const char* module, const char* class_name) noexcept {
        WrenForeignClassMethods class_impl{nullptr, nullptr};
        
        // only kernel libs (Wire supplied wren modules) can bind foreign classes
        if (!WireNode::is_kernel_lib(module)) {
            return class_impl;
        }

        if (strcmp(class_name, "ProcInfo") == 0) {
            class_impl.allocate = wios_proc_info_allocate;
            class_impl.finalize = wios_proc_info_finalize;
        }

        assert(false && "failed to bind a kernel lib class");
        return class_impl;
    }

    static void wios_write(WrenVM* vm, const char* text) noexcept {
        std::cout << text;
    }

    static void wios_error(WrenVM* vm, WrenErrorType type, const char* mod, int line, const char* message) noexcept {
        if (mod) {
            std::cerr << '[' << mod << ':' << line << "] " << message << '\n';
            return;
        }

        std::cerr << message << '\n';
    }

    void wiosify_vm_config(WrenConfiguration* config) noexcept {
        config->loadModuleFn = wios_load_module;
        config->bindForeignMethodFn = wios_bind_method;

        config->writeFn = wios_write;
        config->errorFn = wios_error;
    }
}