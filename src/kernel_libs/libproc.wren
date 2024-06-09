// WiOS libproc.wl kernel lib implementation

foreign class ProcInfo {
    foreign pid
    foreign name
    
    foreign uptime

    toString { "name:%(name), pid:%(pid)" }
}

class proc {
    // returns a list of ProcInfos currently active on this node
    foreign static proc_list()
    
    // starts a new process in the background, returns the pid of the new proc
    // aborts if no executable found in [path] or [args] is not a List of Strings (or empty)
    foreign static exec(path, args)

    // kills an active process, aborts if no process with [pid] active
    foreign static kill(pid)
}