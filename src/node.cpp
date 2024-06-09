#include "node.hpp"
#include "wios.hpp"

#include <string.h>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

namespace wire {
    bool WireNode::is_kernel_lib(std::string_view module_name) noexcept {
        // just a table of known kernel libs

        return module_name == "libproc" || module_name == "libconn" || module_name == "libio";
    }

    WireNode::WireNode(wire_addr_t local_addr) noexcept :
        local_addr{local_addr}
    {
        exec_init();
    }

    WireNode::~WireNode() noexcept {

    }

    void WireNode::tick() noexcept {

    }

    /* std::optional<std::string_view> WireNode::get_kernel_src(std::string_view lib_name) noexcept {
        if (lib_name == "libproc.wl") {
            return std::nullopt;
        } else if (lib_name == "libio.wl") {
            return std::nullopt;
        }

        return std::nullopt;
    } */

    std::optional<std::string_view> WireNode::get_source(std::string_view path) const noexcept {
        auto iter = node_installed_sources.find(std::hash<std::string_view>{}(path));

        if (iter == node_installed_sources.end()) return std::nullopt;
        return iter->second;
    }

    void WireNode::exec(WrenVM* vm, std::string_view src_path) noexcept {
        std::optional<std::string_view> src = get_source(src_path);
        if (!src) {
            wrenSetSlotString(vm, 0, "WiOS error: executable file not found");
            wrenAbortFiber(vm, 0);

            return;
        }

        wire_pid_t pid = next_pid++;
        const auto [iter, inserted] = node_active_procs.emplace(std::piecewise_construct, std::forward_as_tuple(pid), std::forward_as_tuple(this, pid, src_path));
        
        assert(inserted && "pid already in use");
        WiOSProc& proc = iter->second;

        // trasfter args
        wrenEnsureSlots(proc.proc_vm, 3);
        wrenSetSlotNewList(proc.proc_vm, 1);

        int arg_count = wrenGetListCount(vm, 2);
        for (uint32_t i = 0; i < arg_count; i++) {
            wrenGetListElement(vm, 2, i, 1);

            if (wrenGetSlotType(vm, 1) != WREN_TYPE_STRING) {
                wrenSetSlotString(vm, 0, "WiOS error: all args must be a String type");
                wrenAbortFiber(vm, 0);

                return;
            }

            // read from callee vm
            int arg_size;
            const char* arg_data = wrenGetSlotBytes(vm, 1, &arg_size);

            // write to new vm
            wrenSetSlotBytes(proc.proc_vm, 2, arg_data, arg_size);
            wrenInsertInList(proc.proc_vm, 1, -1, 1);
        }

        proc.proc_args_buf = wrenGetSlotHandle(proc.proc_vm, 1);

        // TODO: metered wren execution
        WrenInterpretResult result = wrenInterpret(proc.proc_vm, "user_proc", src.value().data());

        if (result != WREN_RESULT_SUCCESS) {
            cleanup_proc(pid);
        }
    }

    void WireNode::exec_init() noexcept {
        wire_pid_t pid = next_pid++;
        const auto [iter, inserted] = node_active_procs.emplace(std::piecewise_construct, std::forward_as_tuple(pid), std::forward_as_tuple(this, pid, "wios_init"));
        
        assert(inserted && "pid already in use");
        WiOSProc& proc = iter->second;

        // init args
        wrenEnsureSlots(proc.proc_vm, 2);
        wrenSetSlotNewList(proc.proc_vm, 1);

        proc.proc_args_buf = wrenGetSlotHandle(proc.proc_vm, 1);

        auto init_src = KernSourceStorage::get_storage().get_kernel_src("wios_init");
        assert(init_src && "failed to load wios_init source");

        // TODO: metered wren execution
        WrenInterpretResult result = wrenInterpret(proc.proc_vm, "user_proc", init_src.value().data());

        assert(result == WREN_RESULT_SUCCESS && "failed to interpret init proc");
    }

    void WireNode::kill(WrenVM* vm, wire_pid_t pid) noexcept {
        auto iter = node_active_procs.find(pid);

        if (iter == node_active_procs.end()) {
            wrenSetSlotString(vm, 0, "WiOS error: no process with pid active");
            wrenAbortFiber(vm, 0);

            return;
        }

        if (pid == reinterpret_cast<WiOSProc*>(wrenGetUserData(vm))->proc_id) {
            wrenSetSlotString(vm, 0, "WiOS error: a process cannot kill itself");
            wrenAbortFiber(vm, 0);

            return;
        }

        cleanup_proc(pid);
    }

    void WireNode::cleanup_proc(wire_pid_t pid) noexcept {
        auto iter = node_active_procs.find(pid);
        assert(iter != node_active_procs.end() && "can't find pid");

        // TODO: logging

        node_active_procs.erase(iter);
    }

    WireNode::WiOSProc::WiOSProc(WireNode* host_node, wire_pid_t pid, std::string_view name) noexcept:
        host_node{host_node},
        proc_name{name},
        start_time{host_node->timestamp()},
        proc_id{pid}
    {
        WrenConfiguration config;
        wrenInitConfiguration(&config);
        wiosify_vm_config(&config);

        config.userData = this;

        proc_vm = wrenNewVM(&config);
    }

    WireNode::WiOSProc::~WiOSProc() noexcept {
        if (proc_info_class_handle) wrenReleaseHandle(proc_vm, proc_info_class_handle);
        wrenReleaseHandle(proc_vm, proc_args_buf);

        wrenFreeVM(proc_vm);
    }
}