#include "node.hpp"

#include <span>

// This file contains all Wire/WiOS wren foreign functions
namespace wire {
    // used as the data for a WiOS process class `foreign class ProcInfo`
    struct WiOSForeignProcInfo {
        WrenHandle* proc_name; // handle to a pre-allocated wren string

        wire_timestamp_t proc_uptime;
        wire_pid_t proc_id;
    };

    // static proc_list()
    static void wios_proc_list(WrenVM* vm) noexcept {
        WireNode& node = *reinterpret_cast<WireNode*>(wrenGetUserData(vm));

        wrenEnsureSlots(vm, 4);

        std::span<WireNode::WireProc> procs = node.proc_list();
        wrenSetSlotNewList(vm, 0);

        for (auto& proc : procs) {
            WiOSForeignProcInfo* proc_info = reinterpret_cast<WiOSForeignProcInfo*>(wrenSetSlotNewForeign(vm, 1, 3, sizeof(WiOSForeignProcInfo)));
            
            // init ProcInfo data
            wrenSetSlotBytes(vm, 2, proc.proc_name.data(), proc.proc_name.size());
            proc_info->proc_name = wrenGetSlotHandle(vm, 2);

            proc_info->proc_uptime = node.timestamp() - proc_info->proc_uptime;
            proc_info->proc_id = proc.proc_id;

            wrenInsertInList(vm, 0, -1, 1);
        }
    }

    static void wios_exec(WrenVM* vm) noexcept {
        WireNode& node = *reinterpret_cast<WireNode*>(wrenGetUserData(vm));

        if (wrenGetSlotType(vm, 1) != WREN_TYPE_STRING || wrenGetSlotType(vm, 2) != WREN_TYPE_LIST /*TODO: might allow Null as none*/) {
            wrenSetSlotString(vm, 0, "WiOS Error: mistyped call arguments, must use String for path and List for args");
            wrenAbortFiber(vm, 0);
            
            return;
        }

        int size;
        const char* data = wrenGetSlotBytes(vm, 1, &size);
        std::string_view src_path(data, size);

        WrenHandle* args = wrenGetSlotHandle(vm, 2);

        wrenSetSlotDouble(vm, 0, node.exec(src_path, args));
    }

    static void wios_kill(WrenVM* vm) noexcept {
        WireNode& node = *reinterpret_cast<WireNode*>(wrenGetUserData(vm));
    }
}