#pragma once

#include "utils.hpp"
#include <wren.hpp>

#include <vector>
#include <string>
#include <optional>
#include <span>

namespace wire {
    // Class that contains the entire state of a single active node
    // This class contains the Wren VM dedicated to the node and cannot be used concurrently 

    class WireNode {
    public:
        WireNode(wire_addr_t local_addr /*, NodeConfig node_config*/) noexcept;
        ~WireNode() noexcept;

        void tick() noexcept;

        wire_timestamp_t timestamp() const noexcept { return 0; } // TODO

        // WiOS interface

        struct WireProc {
            std::string proc_name;
            
            wire_timestamp_t start_time;
            wire_pid_t proc_id;
        };

        // gets the wren src for the [lib_name] kernel lib
        std::optional<std::string_view> get_kernel_lib(std::string_view lib_name) noexcept;

        // libproc.wl
        std::span<WireProc> proc_list() noexcept;
        
        uint8_t exec(std::string_view src_path, WrenHandle* exec_args) noexcept;
        uint8_t kill(uint32_t pid) noexcept;

        // libconn.wl
        // TODO: finish WireConn

        // libio.wl
        // TODO:

        // checks if [module_name] is a valid kernel lib module (aka can import foreign methods)
        constexpr static bool is_kernel_lib(std::string_view module_name) noexcept;



    private:

        // Wren vm
        WrenVM* node_vm;

        // node/WiOS state 
        // std::vector<WireConn> node_conns;
        std::vector<WrenHandle*> node_active_procs;
        
        // node config
        wire_addr_t local_addr;
    };
}