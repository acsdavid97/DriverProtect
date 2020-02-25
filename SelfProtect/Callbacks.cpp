#include "Callbacks.h"
#include "ProcessCollector.h"

#include <ntifs.h>

const UNICODE_STRING gMonitoredPath = RTL_CONSTANT_STRING(L"very_specific_monitored_path_in_lowercase");

constexpr
static
USHORT
GetCharCountForUnicode(
    _In_ PCUNICODE_STRING String
)
{
    return String->Length / sizeof(WCHAR);
}

static
bool
IsMonitoredPath(
    _In_ PCUNICODE_STRING Path
)
{
    USHORT lengthInChar = GetCharCountForUnicode(Path);
    for (USHORT i = 0; i < lengthInChar; i++)
    {
        USHORT remaining = lengthInChar - i;
        if (remaining < GetCharCountForUnicode(&gMonitoredPath))
        {
            // path too small to be substring of monitored string.
            return false;
        }

        UNICODE_STRING tempString;
        tempString.Buffer = &Path->Buffer[i];
        tempString.Length = gMonitoredPath.Length;
        tempString.MaximumLength = gMonitoredPath.Length;

        LONG difference = RtlCompareUnicodeString(&gMonitoredPath, &tempString, true);
        if (difference == 0)
        {
            return true;
        }
    }
    
    return false;
}

FLT_PREOP_CALLBACK_STATUS
SelfProtectPreCreateCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (Data->RequestorMode != UserMode)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (!Data->Thread)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PFLT_FILE_NAME_INFORMATION fileNameInformation = nullptr;
    auto status = FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fileNameInformation);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltGetFileNameInformation failed with status 0x%X\n", status));
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    FLT_PREOP_CALLBACK_STATUS callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    if (IsMonitoredPath(&fileNameInformation->Name))
    {
        KdPrint(("Create/Open at %wZ!\n", fileNameInformation->Name));
        PEPROCESS process = IoThreadToProcess(Data->Thread);
        if (!ProcessCollectorIsMonitored(process))
        {
            KdPrint(("Denied access for process 0x%x at path %wZ!\n", PsGetProcessId(process), fileNameInformation->Name));
            // process not in collector, deny access
            auto create = Data->Iopb->Parameters.Create;
            create.SecurityContext->DesiredAccess &= ~DELETE;
            create.SecurityContext->DesiredAccess &= ~WRITE_DAC;
            create.SecurityContext->DesiredAccess &= ~WRITE_OWNER;
            create.SecurityContext->DesiredAccess &= ~WRITE_OWNER;
            create.SecurityContext->DesiredAccess &= ~GENERIC_WRITE;
            create.SecurityContext->DesiredAccess &= ~GENERIC_ALL;
            create.SecurityContext->DesiredAccess &= ~FILE_WRITE_DATA;
            create.SecurityContext->DesiredAccess &= ~FILE_WRITE_ATTRIBUTES;
            create.SecurityContext->DesiredAccess &= ~FILE_WRITE_EA;
            create.SecurityContext->DesiredAccess &= ~FILE_APPEND_DATA;
            create.SecurityContext->AccessState->RemainingDesiredAccess = create.SecurityContext->DesiredAccess;
            create.SecurityContext->AccessState->OriginalDesiredAccess = create.SecurityContext->DesiredAccess;
            callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    }

    FltReleaseFileNameInformation(fileNameInformation);

    return callbackStatus;
}

void
CreateProcessNotifyCallback(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    if (!CreateInfo)
    {
        ProcessCollectorRemove(Process);
        return;
    }

    if (IsMonitoredPath(CreateInfo->ImageFileName))
    {
        KdPrint(("Process with PID: 0x%x and Path: %wZ and CommandLine: %wZ is created\n", ProcessId, CreateInfo->ImageFileName, CreateInfo->CommandLine));
        auto status = ProcessCollectorAdd(Process);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("ProcessCollectorAdd failed with 0x%X\n", status));
        }
    }
}
