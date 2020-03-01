#ifndef _SELF_PROTECT_H_
#define _SELF_PROTECT_H_

#include <fltKernel.h>

typedef struct _SELF_PROTECT_DATA
{
    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER Filter;
    PFLT_PORT ServerPort;
    PFLT_PORT ClientPort;
    volatile LONG IsDenyOn;
}SELF_PROTECT_DATA, * PSELF_PROTECT_DATA;

#endif // !_SELF_PROTECT_H_
