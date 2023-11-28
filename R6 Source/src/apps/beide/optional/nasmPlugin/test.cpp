#include <stdio.h>
#define DEBUG 1
#include <Debug.h>

char* src = "hello world!";

extern "C" void my_copy(char* dest, char* source, long length);

int main()
{
	long length = strlen(src);
	char* dest = new char[length + 1];
	// DEBUGGER("ready to call my_copy");

	my_copy(dest, src, length);
	fprintf(stderr, "The string (which should be a nice greeting) is... %s\n", dest);
	delete [] dest;
	return 0;
}
