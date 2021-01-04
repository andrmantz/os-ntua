//zing2.c
#include <stdio.h>
#include <unistd.h>

void zing(void)
{
	printf("What's up, %s?\n", getlogin());
}

