#include <stdio.h>
#include <Input.h>
#include <List.h>


void
list_devices()
{
	printf("         name                  type         state \n");
	printf("--------------------------------------------------\n");

	BList devices;
	get_input_devices(&devices);

	int32 numDevices = devices.CountItems();

	if (numDevices < 1)
		printf("...no input devices found...\n");

	for (int32 i = 0; i < numDevices; i++) {
		BInputDevice *device = (BInputDevice *)devices.ItemAt(i);

		const char *typeStr = NULL;
		switch (device->Type()) {
			case B_POINTING_DEVICE:
				typeStr = "B_POINTING_DEVICE";
				break;

			case B_KEYBOARD_DEVICE:
				typeStr = "B_KEYBOARD_DEVICE";
				break;

			default:
				typeStr = "B_UNDEFINED_DEVICE";
				break;
		}

		printf("%23s %18s %7s\n", device->Name(), typeStr, device->IsRunning() ? "running" : "stopped");
	}
}


void
print_help(
	const char	*command_name)
{
	printf("USAGE: %s [+|-]input_device_name\n", command_name);	
}


void
start_device(
	const char	*name)
{
	BInputDevice *device = find_input_device(name);
	if (device == NULL) 
		printf("Error finding device \"%s\"\n", name);
	else {
		status_t err = device->Start();
		if (err == B_NO_ERROR)
			printf("Started device \"%s\"\n", name);
		else
			printf("Error starting device \"%s\" (%ld)\n", name, err);
		delete (device);
		
	}
}


void
stop_device(
	const char	*name)
{
	BInputDevice *device = find_input_device(name);
	if (device == NULL) 
		printf("Error finding device \"%s\"\n", name);
	else {
		status_t err = device->Stop();
		if (err == B_NO_ERROR)
			printf("Stopped device \"%s\"\n", name);
		else
			printf("Error stopping device \"%s\" (%ld)\n", name, err);
		delete (device);
		
	}
}


int
main(
	int		argc,
	char	*argv[])
{
	if (argc < 2)
		list_devices();
	else {
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '+')
				start_device(argv[i] + 1);
			else if (argv[i][0] == '-')
				stop_device(argv[i] + 1);
			else
				print_help(argv[0]);
		}
	}

	return (0);
}
