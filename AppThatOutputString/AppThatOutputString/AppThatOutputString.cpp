#include <iostream>
#include <Windows.h>

int main()
{
    CHAR str[20] = "I hate programming!";
    for (int count = 0; ; count++) {
        printf("%d:%s\n", count, str);
        Sleep(1000);
    }

}


