
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main()
{
	//for(int i =0;i<10;i++)
	//{
	//    int *a = (int*)malloc(1000+i);
	//	*a = 1;
	//}
	unsigned short a = 1,b=65535;
	printf("a-b=%d\n", (int)(unsigned short)(a - b));
	system("pause");
	return 0;
}