#include <stdio.h>
int func4(int, int, int);

int main()
{
    int i = 0;
    while (1)
    {
        if (func4(i, 0, 0xe) == 0x15)
        {
            printf("%d\n", i);
            break;
        }
        i++;
    }
    
    return 0;
}

int func4(int edi, int esi, int edx) // target, left, right
{
    int ebx;
    int rax; //result

    ebx = (edx - esi) / 2 + esi; // 사실상 (left + right) / 2
    if (ebx > edi)
    {
        return ebx + func4(edi, esi, ebx - 1); // target이 작음
    }
    rax = ebx;
    if (ebx < edi)
    {
        return ebx + func4(edi, ebx + 1, edx); // target이 큼
    }
    return rax;
}