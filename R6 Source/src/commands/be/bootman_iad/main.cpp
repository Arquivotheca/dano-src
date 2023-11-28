#include <Application.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Screen.h>

#include <fs_info.h>

#include <getopt.h>
#include <string.h>

#include "window.h"

int main(int argc, char **argv)
{
	BApplication app("application/x-vnd.Be-bootman");
	char default_name[B_FILE_NAME_LENGTH];
	char default_beos_boot_path[B_FILE_NAME_LENGTH];
	char *default_file = default_name;
	char *beos_boot_path = default_beos_boot_path;

	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, dev_for_path("/boot"), true,
			default_name, B_FILE_NAME_LENGTH) == B_OK) {
		strcat(default_name, "/bootman/MBR");
	} else {
		strcpy(default_name, "/boot/home/config/settings/bootman/MBR");
	}

	strcpy(default_beos_boot_path, "/boot");

	struct option longopts[] = {
		{ "defaultfile", required_argument, 0, 'd' },
		{ "bootpath", required_argument, 0, 'b' },
		{ NULL, 0, 0, 0 }
	};

	char *p = strrchr(*argv, '/');
	if (p) *argv = p + 1;

	int i;
	while ((i = getopt_long(argc, argv, "b:d:", longopts, NULL)) != EOF) {
		switch (i) {
			case 'b' :
					beos_boot_path = optarg;
					break;
			case 'd' :
					default_file = optarg;
					break;
			default :
					return 1;
		}
	}
	
	if (optind != argc) {
		while (optind < argc) {
			printf("Extraneous command line argument: %s\n", argv[optind]);
			optind++;
		}
		return 1;
	}

	float w = 300, h = 250, sw, sh;

	font_height fh;
	be_plain_font->GetHeight(&fh);
	h *= (fh.ascent + fh.descent) / 12.;
	w *= (fh.ascent + fh.descent) / 12.;

	BRect f = BScreen().Frame();
	sw = f.Width(); sh = f.Height();
	f.left = (sw - w) / 2; f.right = (sw + w) / 2;
	f.top = (sh - h) / 2; f.bottom = (sh + h) / 2;

	TWindow *win = new TWindow(f, "Be Boot Manager",
			B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE,
			default_file, beos_boot_path);
	win->Show();

	app.Run();

	return 0;
}
