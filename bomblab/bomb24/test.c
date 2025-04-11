#include <stdio.h>

int main()
{   
    for (int i = 0; i < 8; i++)
    {
        int cnt = 0;
        while (1)
        {
            if (phase_3(i, cnt) == 1)
            {
                printf("i = %d, cnt = %d\n", i, cnt);
                break;
            }
            cnt++;
        }
        
    }
    

    return 0;
}

int phase_3(x, y)
{
    int result = 0;
	switch(x)
    {
        case 0: result += 0x208;
		case 1: result -= 0x6e;
		case 2: result += 0x248;
		case 3: result -= 0x26c;
		case 4: result += 0x26c;
		case 5: result -= 0x26c;
		case 6: result += 0x26c;
		case 7: result -= 0x26c;
    }
		

	if (result == y) { return 1;}
	else {return 0;}

}
