#include <Windows.h>
#include <stdio.h>
#include <fltuser.h>
#include <TlHelp32.h>
#include "comm_defs.h"

void PrintUsage()
{
    printf("usage:\n  Client.exe -e <process_name_to_allow>\n  OR\n  Client.exe -on\n  OR\n  Client.exe -off");
}

void
SendHeaderCommand(
    HANDLE FilterPort,
    SELF_PROTECT_COMMAND Command
)
{
    DWORD bytesReturned = 0;
    SELF_PROTECT_MESSAGE header;
    header.Command = Command;
    HRESULT status = FilterSendMessage(FilterPort, &header, sizeof(header), nullptr, 0, &bytesReturned);
    if (status != S_OK)
    {
        printf("FilterSendMessage failed with 0x%x\n", status);
        return;
    }
}

void
TurnDenyOn(
    HANDLE FilterPort
)
{
    SendHeaderCommand(FilterPort, SelfProtectTurnDenyOn);
}

void
TurnDenyOff(
    HANDLE FilterPort
)
{
    SendHeaderCommand(FilterPort, SelfProtectTurnDenyOff);
}

void ExceptProcessByPid(
    HANDLE FilterPort,
    ULONG ProcessId
)
{
    DWORD bytesReturned = 0;
    SELF_PROTECT_ADD_PROCESS_COMMAND command;
    command.Header.Command = SelfProtectAddProcess;
    command.ProcessId = ProcessId;
    HRESULT status = FilterSendMessage(FilterPort, &command, sizeof(command), nullptr, 0, &bytesReturned);
    if (status != S_OK)
    {
        printf("FilterSendMessage failed with 0x%x for PID: 0x%x\n", status, ProcessId);
        return;
    }
}

void
ExceptProcessByName(
    HANDLE FilterPort,
    char* ProcessName
)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    bool foundOne = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_stricmp(entry.szExeFile, ProcessName) == 0)
            {
                ExceptProcessByPid(FilterPort, entry.th32ProcessID);
                foundOne = true;
            }
        }
    }

    CloseHandle(snapshot);
    if (!foundOne)
    {
        printf("Could not find process with name %s\n", ProcessName);
        return;
    }

    printf("All good!\n");
}

int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3)
    {
        PrintUsage();
        return -1;
    }

    HANDLE filterPort = nullptr;
    auto status = FilterConnectCommunicationPort(SELF_PROTECT_PORT_NAME, FLT_PORT_FLAG_SYNC_HANDLE,
        nullptr, 0, nullptr, &filterPort);
    if (status != S_OK)
    {
        printf("FilterConnectCommunicationPort failed with 0x%x\n", status);
        return 0;
    }


    if (_stricmp(argv[1], "-e") == 0)
    {
        if (argc != 3)
        {
            PrintUsage();
            return -1;
        }
        ExceptProcessByName(filterPort, argv[2]);
    }

    if (_stricmp(argv[1], "-on") == 0)
    {
        TurnDenyOn(filterPort);
    }

    if (_stricmp(argv[1], "-off") == 0)
    {
        TurnDenyOff(filterPort);
    }


    CloseHandle(filterPort);
    return 0;
}