#include <support/SupportDefs.h>
#include <priv_syscalls.h>
#include <driver_settings.h>

#include <stdio.h>

int main(int argc, char **argv)
{
	char c;
	uint32 size = sizeof(c);

	if (_kget_safemode_option_(B_SAFEMODE_SAFE_MODE, &c, &size) == B_OK) {
		printf("yes\n");
		return 1;
	}

	printf("no\n");

	return 0;
}
