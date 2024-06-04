#pragma once

#include "utils.hpp"

namespace wire {
    // Class that owns that state of a single connection between two nodes

    class WireConn {
    public:
        WireConn() noexcept;
        ~WireConn() noexcept;

    private:
        

        wire_addr_t local_addr;
        wire_addr_t remote_addr;

        wire_port_t local_port;
        wire_port_t remote_port;
    };
}