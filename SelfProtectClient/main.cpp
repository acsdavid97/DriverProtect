#include <Windows.h>
#include <stdio.h>
#include <fltuser.h>
#include <TlHelp32.h>
#include "comm_defs.h"


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("usage: Client.exe <process_name_to_allow>\n");
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

    HANDLE process = nullptr;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_stricmp(entry.szExeFile, argv[1]) == 0)
            {
                process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            }
        }
    }

    CloseHandle(snapshot);

    if (!process)
    {
        printf("no process found with name %s\n", argv[1]);
        return -1;
    }

    DWORD bytesReturned = 0;
    SELF_PROTECT_ADD_PROCESS_COMMAND command;
    command.Header.Command = SelfProtectAddProcess;
    command.ProcessHandle = process;
    status = FilterSendMessage(filterPort, &command, sizeof(command), nullptr, 0, &bytesReturned);
    if (status != S_OK)
    {
        printf("FilterSendMessage failed with 0x%x\n", status);
        return 0;
    }

    printf("All good!\n");

    CloseHandle(filterPort);
    return 0;
}