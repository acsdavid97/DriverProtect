#include "ProcessCollector.h"
#include "HeapTags.h"

typedef struct _MONITORED_PROCESS_ENTRY
{
    LIST_ENTRY Entry;
    PEPROCESS Process;
    PUNICODE_STRING ImagePath;
}MONITORED_PROCESS_ENTRY, *PMONITORED_PROCESS_ENTRY;

typedef struct _PROCESS_COLLECTOR_DATA
{
    FAST_MUTEX Mutex;
    _Guarded_by_(Mutex)
    LIST_ENTRY MonitoredProcessesHead;
}PROCESS_COLLECTOR_DATA, *PPROCESS_COLLECTOR_DATA;

static PROCESS_COLLECTOR_DATA gProcessCollectorData;

void
ProcessCollectorInit()
{
    ExInitializeFastMutex(&gProcessCollectorData.Mutex);
    InitializeListHead(&gProcessCollectorData.MonitoredProcessesHead);
}

void
ProcessCollectorUninit()
{
    ExAcquireFastMutex(&gProcessCollectorData.Mutex);

    PLIST_ENTRY current = RemoveHeadList(&gProcessCollectorData.MonitoredProcessesHead);
    while (current != &gProcessCollectorData.MonitoredProcessesHead)
    {
        auto processEntry = CONTAINING_RECORD(current, MONITORED_PROCESS_ENTRY, Entry);
        ObDereferenceObject(processEntry->Process);
        ExFreePoolWithTag(processEntry, PROC_COLLECTOR_TAG);
        current = RemoveHeadList(&gProcessCollectorData.MonitoredProcessesHead);
    }

    ExReleaseFastMutex(&gProcessCollectorData.Mutex);
}

NTSTATUS
ProcessCollectorAdd(
    _In_ PEPROCESS Process
)
{
    auto processEntry = (PMONITORED_PROCESS_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(MONITORED_PROCESS_ENTRY), PROC_COLLECTOR_TAG);
    if (!processEntry)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    processEntry->Process = Process;
    ExAcquireFastMutex(&gProcessCollectorData.Mutex);
    InsertTailList(&gProcessCollectorData.MonitoredProcessesHead, &processEntry->Entry);
    ExReleaseFastMutex(&gProcessCollectorData.Mutex);

    return STATUS_SUCCESS;
}

bool
ProcessCollectorIsMonitored(
    _In_ PEPROCESS Process
)
{
    bool found = false;
    ExAcquireFastMutex(&gProcessCollectorData.Mutex);

    for (PLIST_ENTRY current = gProcessCollectorData.MonitoredProcessesHead.Flink;
        current != &gProcessCollectorData.MonitoredProcessesHead;
        current = current->Flink)
    {
        auto processEntry = CONTAINING_RECORD(current, MONITORED_PROCESS_ENTRY, Entry);
        if (processEntry->Process == Process)
        {
            found = true;
            break;
        }
    }

    ExReleaseFastMutex(&gProcessCollectorData.Mutex);

    return found;
}

NTSTATUS
ProcessCollectorRemove(
    _In_ PEPROCESS Process
)
{
    NTSTATUS status = STATUS_NOT_FOUND;
    ExAcquireFastMutex(&gProcessCollectorData.Mutex);

    for (PLIST_ENTRY current = gProcessCollectorData.MonitoredProcessesHead.Flink;
        current != &gProcessCollectorData.MonitoredProcessesHead;
        current = current->Flink)
    {
        auto processEntry = CONTAINING_RECORD(current, MONITORED_PROCESS_ENTRY, Entry);
        if (processEntry->Process == Process)
        {
            RemoveEntryList(&processEntry->Entry);
            ObDereferenceObject(processEntry->Process);
            ExFreePoolWithTag(processEntry, PROC_COLLECTOR_TAG);
            status = STATUS_SUCCESS;
        }
    }

    ExReleaseFastMutex(&gProcessCollectorData.Mutex);

    return status;
}
