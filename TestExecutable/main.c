#include <stdio.h>
#include <windows.h>

int main()
{
    HANDLE file = CreateFileA("hello.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("create file failed: %d\n", GetLastError());
        return 1;
    }

    printf("all good!\n");

    CloseHandle(file);

    return 0;
}