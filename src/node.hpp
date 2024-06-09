#pragma once

#include "utils.hpp"
#include <wren.hpp>

#include <map>
#include <unordered_map>
#include <string>
#include <optional>
#include <span>

namespace wire {
    // Class that contains the entire state of a single active node
    // This class contains the Wren VMs dedicated to the node and cannot be used concurrently 

    class WireNode {
    public:
        WireNode(wire_addr_t local_addr /*, NodeConfig node_config*/) noexcept;
        ~WireNode() noexcept;

        void tick() noexcept;

        wire_timestamp_t timestamp() const noexcept { return 0; } // TODO

        // WiOS interface

        // TODO: cross-vm args
        struct WiOSProc {
            WiOSProc(WireNode* host_node, wire_pid_t pid, std::string_view name) noexcept;
            ~WiOSProc() noexcept;

            WiOSProc(const WiOSProc&) = delete;
            WiOSProc& operator=(const WiOSProc&) = delete;

            WrenVM* proc_vm;
            WireNode* host_node;

            WrenHandle* proc_args_buf;

            // note: WrenHandles are inited after loading the relevant kernel lib
            WrenHandle* proc_info_class_handle = nullptr;

            std::string proc_name;
            
            wire_timestamp_t start_time;
            wire_pid_t proc_id;
        };
        
        // gets the source for a wren script installed on this node
        std::optional<std::string_view> get_source(std::string_view path) const noexcept;

        // libproc.wl
        std::map<wire_pid_t, WiOSProc>& proc_list() noexcept { return node_active_procs; }
        
        void exec(WrenVM* vm, std::string_view src_path) noexcept;
        void kill(WrenVM* vm, uint32_t pid) noexcept;

        void cleanup_proc(wire_pid_t pid) noexcept;

        // libconn.wl
        // TODO: finish WireConn

        // libio.wl
        // TODO:

        // checks if [module_name] is a valid kernel lib module (aka can import foreign methods)
        static bool is_kernel_lib(std::string_view module_name) noexcept;
    private:
        void exec_init() noexcept;

        // node/WiOS state 
        // std::vector<WireConn> node_conns;
        std::map<wire_pid_t, WiOSProc> node_active_procs;
        std::map<size_t /*hashed path string*/, std::string> node_installed_sources;
        
        wire_pid_t next_pid = 1;

        // node config
        wire_addr_t local_addr;
    };
}