#include "onReceipt.h"
#include "Microkernel.h"
#include "SystemCalls.h"

void onReceipt(Message *m)
{
    uint32_t vfs = getVFS();
    int idx = getLinuxPID(m->Source);

    if(m->Code < _signalCount && _signals[m->Code] != 0)
    {
        _sig_func_ptr function = _signals[m->Code];

        //I don't like this, but it's needed for POSIX compatibility
        _signals[m->Code] = 0;
        function(m->Code);
    }

    //This is a decent message handling system. Fairly simple
    if(idx != -1 && _pids[idx] != 0 && _pids[idx]->handler != 0)
        _pids[idx]->handler(m);
}
