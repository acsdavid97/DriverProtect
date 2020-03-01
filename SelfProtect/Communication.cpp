#include "Communication.h"
#include "SelfProtect.h"
#include "comm_defs.h"
#include "ProcessCollector.h"

extern SELF_PROTECT_DATA gSelfProtectData;

static
NTSTATUS
HandleAddProcess(
    _In_ PSELF_PROTECT_ADD_PROCESS_COMMAND AddProcessCommand
)
{
    PEPROCESS process = nullptr;
    auto status = PsLookupProcessByProcessId((HANDLE)AddProcessCommand->ProcessId, &process);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("PsLookupProcessByProcessId failed with status 0x%X\n", status));
        return status;
    }

    status = ProcessCollectorAdd(process);
    if (!NT_SUCCESS(status))
    {
        ObDereferenceObject(process);
        KdPrint(("ProcessCollectorAdd failed with status 0x%X\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
FLTAPI
ConnectNotifyCallback(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionPortCookie
)
{
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionPortCookie);

    gSelfProtectData.ClientPort = ClientPort;

    return STATUS_SUCCESS;
}

static
VOID
FLTAPI DisconnectNotifyCallback(
    _In_opt_ PVOID ConnectionCookie
)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);

    FltCloseClientPort(gSelfProtectData.Filter, &gSelfProtectData.ClientPort);
    gSelfProtectData.ClientPort = nullptr;
}

static
NTSTATUS
FLTAPI
MessageNotifyCallback(
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
)
{
    UNREFERENCED_PARAMETER(PortCookie);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    *ReturnOutputBufferLength = 0;

    if (!InputBuffer)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (InputBufferLength < sizeof(SELF_PROTECT_MESSAGE))
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    auto message = (PSELF_PROTECT_MESSAGE)InputBuffer;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    __try
    {
        switch (message->Command)
        {
        case SelfProtectAddProcess:
            if (InputBufferLength < sizeof(SELF_PROTECT_ADD_PROCESS_COMMAND))
            {
                status = STATUS_INVALID_PARAMETER_3;
                break;
            }
            status = HandleAddProcess((PSELF_PROTECT_ADD_PROCESS_COMMAND)InputBuffer);
            break;
        case SelfProtectTurnDenyOn:
            _InterlockedExchange(&gSelfProtectData.IsDenyOn, TRUE);
            status = STATUS_SUCCESS;
            break;
        case SelfProtectTurnDenyOff:
            _InterlockedExchange(&gSelfProtectData.IsDenyOn, FALSE);
            status = STATUS_SUCCESS;
            break;
        default:
            status = STATUS_INVALID_PARAMETER_1;
            break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KdPrint(("Exception occoured!\n"));
    }
    
    return status;
}

NTSTATUS
CommunicationInit()
{
    UNICODE_STRING portName = RTL_CONSTANT_STRING(SELF_PROTECT_PORT_NAME);
    PSECURITY_DESCRIPTOR securityDescriptor = nullptr;
    auto status = FltBuildDefaultSecurityDescriptor(&securityDescriptor, FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltBuildDefaultSecurityDescriptor failed with status: 0x%X\n", status));
        goto CleanUp;
    }

    OBJECT_ATTRIBUTES portAttributes;
    InitializeObjectAttributes(&portAttributes, &portName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, securityDescriptor);

    status = FltCreateCommunicationPort(gSelfProtectData.Filter, &gSelfProtectData.ServerPort,
        &portAttributes, nullptr, ConnectNotifyCallback, DisconnectNotifyCallback,
        MessageNotifyCallback, 1);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("FltCreateCommunicationPort failed with status: 0x%X\n", status));
        goto CleanUp;
    }
CleanUp:
    if (!securityDescriptor)
    {
        FltFreeSecurityDescriptor(securityDescriptor);
    }
    return status;
}

void
CommunicationUninit()
{
    FltCloseCommunicationPort(gSelfProtectData.ServerPort);
}