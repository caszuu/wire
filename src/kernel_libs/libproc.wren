// WiOS libproc.wl kernel lib implementation

foreign class ProcInfo {
    foreign pid
    foreign name
    
    foreign uptime
};

class proc {
    proc_list() {
        return [1, 2]
    }

    foreign kern_proc_list()
};