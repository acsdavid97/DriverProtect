#ifndef _COMM_DEFS_H_
#define _COMM_DEFS_H_

#pragma pack(push)
#pragma pack(1)

typedef enum _SELF_PROTECT_COMMAND
{
    SelfProtectAddProcess,
    SelfProtectTurnDenyOn,
    SelfProtectTurnDenyOff
}SELF_PROTECT_COMMAND, *PSELF_PROTECT_COMMAND;

typedef struct _SELF_PROTECT_MESSAGE
{
    SELF_PROTECT_COMMAND Command;
}SELF_PROTECT_MESSAGE, *PSELF_PROTECT_MESSAGE;

typedef struct _SELF_PROTECT_ADD_PROCESS_COMMAND
{
    SELF_PROTECT_MESSAGE Header;
    ULONG ProcessId;
}SELF_PROTECT_ADD_PROCESS_COMMAND, *PSELF_PROTECT_ADD_PROCESS_COMMAND;

#pragma pack(pop)

#define SELF_PROTECT_PORT_NAME L"\\SelfProtectPort"

#endif // !_COMM_DEFS_H_
