#include <Application.h>
#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <video_overlay.h>
#include <tv_out.h>

double 
h_rate_from_display_mode(const display_mode *dm)
{
	double dmRate = (double)dm->timing.pixel_clock; // * (double)1000.0;
	dmRate /= (double)dm->timing.h_total;
	if (dm->timing.flags & B_TIMING_INTERLACED) dmRate /= 2;
	return dmRate;
}

double 
v_rate_from_display_mode(const display_mode *dm)
{
	double dmRate = (double)dm->timing.pixel_clock * (double)1000.0;
	dmRate /= (double)dm->timing.h_total * (double)dm->timing.v_total;
	if (dm->timing.flags & B_TIMING_INTERLACED) dmRate /= 2;
	return dmRate;
}

const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32_LITTLE);
		s2s(B_RGBA32_LITTLE);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16_LITTLE);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15_LITTLE);
		s2s(B_RGBA15_LITTLE);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

void dump_mode(const display_mode *dm) {
	const display_timing *t = &(dm->timing);
	printf("  pixel_clock: %ldKHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags: 0x%08lx\n", t->flags);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh (vert): %4.2f\n", v_rate_from_display_mode(dm));
	printf(" refresh (hori): %4.2f\n", h_rate_from_display_mode(dm));
	printf("  color space: (0x%08lx) %s\n", dm->space, spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags: %.08lx\n\t", dm->flags);
#define DUMPMASKFLAG(mask, value) if ((dm->flags & (uint32)(mask)) == (uint32)(value)) printf(" "#value);
#define DUMPFLAG(value) DUMPMASKFLAG(value, value)
	DUMPFLAG(B_SCROLL);
	DUMPFLAG(B_8_BIT_DAC);
	DUMPFLAG(B_HARDWARE_CURSOR);
	DUMPFLAG(B_PARALLEL_ACCESS);
	DUMPFLAG(B_SUPPORTS_OVERLAYS);
#define TVOUTFLAG(x) DUMPMASKFLAG(B_TV_OUT_MASK, x)
	TVOUTFLAG(B_TV_OUT_NONE);
	TVOUTFLAG(B_TV_OUT_NTSC);
	TVOUTFLAG(B_TV_OUT_NTSC_J);
	TVOUTFLAG(B_TV_OUT_PAL);
	TVOUTFLAG(B_TV_OUT_PAL_M);
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

class myapp : public BApplication {
	public:
		myapp(int argc, char *argv[])
			: BApplication("application/x-vnd.Be.SetScreen")
			, argc(argc)
			, argv(argv)
			{}
		virtual void ReadyToRun(void);
	protected:
		int argc;
		char **argv;
};

int getcmd(void)
{
	char buf[1024];
	char *s = fgets(buf, sizeof(buf), stdin);
	if (s) return *s;
	return 0;
}

void myapp::ReadyToRun(void)
{
	status_t err;
	BScreen screen;
	if(!screen.IsValid()) {
		printf("screen not valid\n");
		Quit();
	}
	display_mode mode, orig;
	err = screen.GetMode(&mode);
	if(err != B_OK) {
		printf("could not get mode\n");
		Quit();
	}
	dump_mode(&mode);
	orig = mode;
	int key;
	bool dump = false;
	char peg_vertical = 'v';
	float refresh = (float)mode.timing.pixel_clock * 1000.0;
	if (peg_vertical == 'v') refresh /= (float)mode.timing.h_total * mode.timing.v_total;
	else refresh /= (float)mode.timing.h_total;

	fprintf(stdout, "Choose: g udlr tswn hvxp:");
	fflush(stdout);
	
	while ((key = getcmd()) != 'q') {
		bool changed = true;
		switch (key) {
			case 'g': // get current mode
				err = screen.GetMode(&mode);
				orig = mode;
				changed = false;
				dump_mode(&mode);
				break;
			case 'u': // up
				mode.timing.v_sync_start += 1;
				mode.timing.v_sync_end += 1;
				break;
			case 'd': // down
				mode.timing.v_sync_start -= 1;
				mode.timing.v_sync_end -= 1;
				break;
			case 'l': // left
				mode.timing.h_sync_start += 8;
				mode.timing.h_sync_end += 8;
				break;
			case 'r': // right
				mode.timing.h_sync_start -= 8;
				mode.timing.h_sync_end -= 8;
				break;
			case ',': // short h sync pulse
				mode.timing.h_sync_end -=8;
				break;
			case '.': // longer h sync pulse
				mode.timing.h_sync_end +=8;
				break;
			case 'w': // wider
				mode.timing.h_total -= 8;
				break;
			case 'n': // narrower
				mode.timing.h_total += 8;
				break;
			case 't': // taller
				mode.timing.v_total -= 1;
				break;
			case 's': // shorter
				mode.timing.v_total += 1;
				break;
			case ';': // set refresh rate
				{
				float new_refresh;
				fprintf(stdout, "New rate: "); fflush(stdout);
				fscanf(stdin, "%f", &new_refresh);
				// burn the new line
				fgetc(stdin);
				if (new_refresh) {
					if (peg_vertical != 'c') refresh = new_refresh;
					else mode.timing.pixel_clock = (uint32)new_refresh;
				} else changed = false;
				}
				break;
			case 'x': // revert to original mode
				mode = orig;
				// fall through to recalc refresh rate
			case 'c': // peg pixel clock
			case 'h': // peg horizontal refresh rate
			case 'v': // peg vertical refresh rate (default)
				if (key != 'x') {
					peg_vertical = key;// == 'v';
					changed = false;
				}
				refresh = (float)mode.timing.pixel_clock * 1000.0;
				if (peg_vertical == 'v') refresh /= (float)mode.timing.h_total * mode.timing.v_total;
				else if (peg_vertical == 'h') refresh /= (float)mode.timing.h_total;
				fprintf(stdout, "\nUsing %s refresh rate of %.2f\n", peg_vertical == 'v' ? "vertical" : (peg_vertical == 'h' ? "horizontal" : "clock"), refresh);
				break;
			case 'p': // toggle mode dumping after each mode set
				dump = !dump;
				fprintf(stdout, "Mode dumping %s\n", dump ? "enabled" : "disabled");
				// fall through
			default:
				// barf
				changed = false;
				break;
		}
		if (changed) {
			// recalc pixel clock, pegging the appropriate refresh rate
			if (peg_vertical != 'c')
				mode.timing.pixel_clock = (uint32)(peg_vertical == 'v' ?
					(refresh * mode.timing.h_total * mode.timing.v_total) / 1000.0 :
					(refresh * mode.timing.h_total) / 1000.0);
			printf("Trying to set this mode:\n");
			dump_mode(&mode);
			// set new mode
			err = screen.SetMode(&mode);
			if (err != B_OK) {
				printf("\nCould not set mode.  Getting current mode.\n");
				err = screen.GetMode(&mode);
			}
			// dump mode info
			if (dump) dump_mode(&mode);
		}
		fprintf(stdout, "\nChoose: g udlr tswn hvxp:");
		fflush(stdout); 
	}
	Quit();
}

int
main(int argc, char *argv[])
{
	myapp app(argc, argv);
	app.Run();
}

