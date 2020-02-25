#ifndef _PROCESS_COLLECTOR_H_
#define _PROCESS_COLLECTOR_H_

#include <fltKernel.h>

void
ProcessCollectorInit();

void
ProcessCollectorUninit();

NTSTATUS
ProcessCollectorAdd(
    _In_ PEPROCESS Process
);

bool
ProcessCollectorIsMonitored(
    _In_ PEPROCESS Process
);

NTSTATUS
ProcessCollectorRemove(
    _In_ PEPROCESS Process
);

#endif // !_PROCESS_COLLECTOR_H_

