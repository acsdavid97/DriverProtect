#include "SelfProtect.h"
#include "Callbacks.h"
#include "ProcessCollector.h"
#include "Communication.h"


NTSTATUS
SelfProtectUnloadCallback(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

SELF_PROTECT_DATA gSelfProtectData = { 0 };

const FLT_OPERATION_REGISTRATION gCallbacks[] =
{
    {
        IRP_MJ_CREATE,
        FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO | FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SelfProtectPreCreateCallback,
        nullptr
    },
    { IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION gFltRegistration =
{
    sizeof(FLT_REGISTRATION),                       // Size
    FLT_REGISTRATION_VERSION,                       // Version
    FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP, // Flags
    nullptr,                                        // ContextRegistration
    gCallbacks,                                     // OperationRegistration

    SelfProtectUnloadCallback,                      // FilterUnload
    
    nullptr,                                        // InstanceSetupCallback
    nullptr,                                        // InstanceQueryTeardownCallback
    nullptr,                                        // InstanceTeardownStartCallback
    nullptr,                                        // InstanceTeardownCompleteCallback

    nullptr,                                        // GenerateFileNameCallback
    nullptr,                                        // NormalizeNameComponentCallback
    nullptr,                                        // NormalizeContextCleanupCallback
    nullptr,                                        // TransactionNotificationCallback
    nullptr,                                        // NormalizeNameComponentExCallback
    nullptr,                                        // SectionNotificationCallback
};

NTSTATUS
SelfProtectUnloadCallback(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyCallback, TRUE);
    FltUnregisterFilter(gSelfProtectData.Filter);
    ProcessCollectorUninit();

    return STATUS_SUCCESS;
}

extern "C"
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    bool filterRegistered = false;
    bool processNotificationRegistered = false;

    ProcessCollectorInit();

    gSelfProtectData.DriverObject = DriverObject;
    auto status = FltRegisterFilter(DriverObject, &gFltRegistration, &gSelfProtectData.Filter);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltRegisterFilter failed with status: 0x%X\n", status));
        goto CleanUp;
    }
    filterRegistered = true;

    status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyCallback, FALSE);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("PsSetCreateProcessNotifyRoutineEx failed with status: 0x%X\n", status));
        goto CleanUp;
    }
    processNotificationRegistered = true;

    status = CommunicationInit();
    if (!NT_SUCCESS(status))
    {
        KdPrint(("CommunicationInit failed with status: 0x%X\n", status));
        goto CleanUp;
    }

    status = FltStartFiltering(gSelfProtectData.Filter);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltStartFiltering failed with status: 0x%X\n", status));
        goto CleanUp;
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        if (processNotificationRegistered)
        {
            PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyCallback, TRUE);
        }

        if (filterRegistered)
        {
            FltUnregisterFilter(gSelfProtectData.Filter);
        }

        ProcessCollectorUninit();
    }

    return status;
}