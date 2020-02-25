#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS
SelfProtectPreCreateCallback(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

void
CreateProcessNotifyCallback(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);


#endif // !_CALLBACKS_H_
