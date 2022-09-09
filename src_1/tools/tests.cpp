/*
#include <cstdio>

#include "structofarrays.hpp"
extern bool soarealloc_test();
extern bool structofarrays_test();

#include "hashtable.hpp"
extern bool hashtable_test();

#include <Windows.h>

int main(int, char**) {
	int successes = 0;

	if (structofarrays_test())	++successes;
	if (hashtable_test())		++successes;

	if (successes != 2) printf("Some tests failed.\n");
	else printf("All tests passed successfully.\n");

	HANDLE hconsole = GetStdHandle(STD_OUTPUT_HANDLE); //stdout
	DWORD mode = 0;
	GetConsoleMode(hconsole, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hconsole, mode);

	printf("\33[92m green? \x1b[0m\n");

	return 0;
}
*/