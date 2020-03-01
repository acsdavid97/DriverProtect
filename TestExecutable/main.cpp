#include <stdio.h>
#include <windows.h>
#include <string>

bool CreateNewFile(char* Directory)
{
    std::string fileToCreate = std::string(Directory) + "\\hello.txt";

    HANDLE file = CreateFileA(fileToCreate.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("create file failed: %d\n", GetLastError());
        return true;
    }

    CloseHandle(file);
    return false;
}

bool DeleteExistingFile(char* Directory)
{
    std::string fileToDelete = std::string(Directory) + "\\preexisting.txt";
    if (!DeleteFileA(fileToDelete.c_str()))
    {
        printf("delete file failed: %d\n", GetLastError());
        return true;
    }
    return false;
}

void RunAll(char* Directory)
{
    if (!CreateNewFile(Directory))
    {
        printf("CreateNewFile failed!\n");
    }

    if (!DeleteExistingFile(Directory))
    {
        printf("DeleteExistingFile failed!\n");
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("tester.exe <directory>");
        return 0;
    }


    while (true)
    {
        RunAll(argv[1]);
        Sleep(1000);
    }

    return 0;
}