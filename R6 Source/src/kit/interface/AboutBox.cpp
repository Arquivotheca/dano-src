// (c) 1997 Be Incorporated
//
#include <Application.h>
#include <AppFileInfo.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <StatusBar.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>

#include <stdlib.h>
#include <string.h>

#include "AboutBox.h"

#if !SUPPORTS_BEOS_ABOUT_BOX

void run_be_about()
{
}

#else

#include "BeOSCredits.h"

// The AboutBox will be centered on the screen
const BRect kAboutBoxRect(0, 0, 500, 300);
const BRect kHotSpotRect(65, 26, 70, 31);
// Some color constants needed///
const rgb_color kLightGray = { 216, 216, 216, 0};
const rgb_color kDarkGray = { 184, 184, 184, 0};
const rgb_color kDarkerGray = { 100, 100, 100, 0};
const rgb_color kWhite = { 255, 255, 255, 0};
const rgb_color kBlack = { 0, 0, 0, 0};
const rgb_color kTextColor = {0xf7, 0xd3, 0x69, 0xff};
const rgb_color kAlternateTextColor = {0xeb, 0x63, 0x63, 0xff};
const rgb_color kBarColor = {50, 150, 255, 255};
// Divider between info and legal views...
const float kDivider = 200.0;
// Path to libbe, used for finding the current version number
static const char *kLibbePath = "/system/lib/libbe.so";
// These are used for each views name, which is
// needed for TogglePoint() so that it known's
// which views to show/hide
static const char *kSysInfoItemName = "info";
static const char *kCreditsItemName = "credits";

void run_be_about()
{
	AboutWindow::RunAboutWindow();
}

// We only keep one copy of the window open...
AboutWindow *AboutWindow::oneCopyOnly = 0;

// ----------------------------
// 			AboutWindow		
// ----------------------------
// #pragma mark -

AboutWindow::AboutWindow()
	: BWindow(kAboutBoxRect, "About BeOS", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AddChild(new AboutView(Bounds(), "AboutView"));
	oneCopyOnly = this;
	// Center the window
	BScreen screen(this);
	MoveTo((screen.Frame().Width() - Frame().Width()) / 2.0, (screen.Frame().Height() - Frame().Height()) / 2.0);
}

AboutWindow::~AboutWindow()
{
	oneCopyOnly = NULL;
}

void AboutWindow::RunAboutWindow()
{
	if (oneCopyOnly) {
		oneCopyOnly->Activate();
		return;
	}

	(new AboutWindow())->Show();
}

static void GetPlatformType(BString &result, system_info *sysinfo)
{
	switch(sysinfo->platform_type) {
		case B_BEBOX_PLATFORM:
			result = "BeBox";
			break;
		case B_MAC_PLATFORM:
			result = "Macintosh";
			break;
		case B_AT_CLONE_PLATFORM:
			result = "IBM PC/AT or clone";
			break;
		default:
			result = "(unknown)";
			break;
	}
}

static int32 CalcCPUSpeed(const system_info *sysinfo)
{
	int32 target = sysinfo->cpu_clock_speed / 1000000;
	int32 frac = target % 100;
	int32 delta = -frac;
	int32 at = 0;
	int32 freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };

	for (uint32 index = 0; index < sizeof(freqs)/sizeof(freqs[0]); index++) {
		int32 ndelta = freqs[index]-frac;
		if (abs(ndelta) < abs(delta)) {
			at = freqs[index];
			delta = ndelta;
		}
	}
	return target+delta;
}


static void GetCPUType(BString &result, system_info *sysinfo)
{
	const char *name;
	
	switch(sysinfo->cpu_type) {
		case B_CPU_PPC_601:
			name = "PowerPC 601";
			break;
		case B_CPU_PPC_603:
			name = "PowerPC 603";
			break;
		case B_CPU_PPC_603e:
			name = "PowerPC 603e";
			break;
		case B_CPU_PPC_750:
			name = "PowerPC 750";
			break;
		case B_CPU_PPC_604:
			name = "PowerPC 604";
			break;
		case B_CPU_PPC_604e:
			name = "PowerPC 604e";
			break;
		case B_CPU_X86:
			name = "Intel x86";
			break;
		case B_CPU_INTEL_PENTIUM:
		case B_CPU_INTEL_PENTIUM75:
			name = "Intel Pentium";
			break;
		case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
		case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
			name = "Intel Pentium 486 Overdrive";
			break;
		case B_CPU_INTEL_PENTIUM_MMX:
		case B_CPU_INTEL_PENTIUM_MMX_MODEL_8:
			name = "Intel Pentium MMX";
			break;
		case B_CPU_INTEL_PENTIUM_PRO:
			name = "Intel Pentium Pro";
			break;
		case B_CPU_INTEL_PENTIUM_II_MODEL_3:
		case B_CPU_INTEL_PENTIUM_II_MODEL_5:
			name = "Intel Pentium II";
			break;
		case B_CPU_INTEL_CELERON:
			name = "Intel Celeron";
			break;			
		case B_CPU_INTEL_PENTIUM_III:
		case B_CPU_INTEL_PENTIUM_III_MODEL_8:
			name = "Intel Pentium III";
			break;
		case B_CPU_AMD_K5_MODEL0:
		case B_CPU_AMD_K5_MODEL1:
		case B_CPU_AMD_K5_MODEL2:
		case B_CPU_AMD_K5_MODEL3:
			name = "AMD-K5";
			break;
		case B_CPU_AMD_K6_MODEL6:
		case B_CPU_AMD_K6_MODEL7:
			name = "AMD-K6";
			break;
		case B_CPU_AMD_K6_2:
			name = "AMD-K6-2";
			break;
		case B_CPU_AMD_K6_III:
			name = "AMD-K6-III";
			break;
		case B_CPU_AMD_ATHLON_MODEL1:
		case B_CPU_AMD_ATHLON_THUNDERBIRD:
			name = "AMD Athlon";
			break;
		case B_CPU_CYRIX_GXm:
			name = "Cyrix GXm";
			break;
		case B_CPU_CYRIX_6x86MX:
			name = "Cyrix 6x86MX";
			break;
		case B_CPU_IDT_WINCHIP_C6:
			name = "IDT WinChip C6";
			break;
		case B_CPU_IDT_WINCHIP_2:
			name = "IDT WinChip 2";
			break;
		case B_CPU_RISE_mP6:
			name = "Rise mP6";
			break;
		case B_CPU_ALPHA:
			name = "Alpha AXP";
			break;
		default:
			name = "(unknown)";
			break;
	}

	int32 count = sysinfo->cpu_count; 	
 	result = "";
 	if (count > 1)
 		result << count << ' ';

 	result << name << (count > 1 ? "'s" : "") <<  " running at " << CalcCPUSpeed(sysinfo) << "MHz";
}

// ----------------------------
// 			AboutView		
// ----------------------------
// #pragma mark -

AboutView::AboutView(BRect frame, const char *name)
	:	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
		fUptime(0),
		fLastUptimeInSeconds(0),
		fThermometer(0),
		fSysInfoMode(true),
		fBigIconRect(0,0,200,50)
{
	// Since we aren't even close to being font sensitive in here
	// we'll just hard code the fonts so that in the event the user
	// decides that they want a default font size of 24.0pt,
	// at least the AboutBox will still be legible. (Or maybe not
	// if the user can't read 10pt font size...) <shrug>
	const font_family plainFontFamily = "Swis721 BT";
	const font_family boldFontFamily = "Swis721 BT";
	const font_style plainFontStyle = "Roman";
	const font_style boldFontStyle = "Bold";
	const float plainFontSize = 10.0;
	const float boldFontSize = 12.0;
	
	BFont plainFont;
	plainFont.SetFamilyAndStyle(plainFontFamily, plainFontStyle);
	plainFont.SetSize(plainFontSize);
	
	BFont boldFont;
	boldFont.SetFamilyAndStyle(boldFontFamily, boldFontStyle);
	boldFont.SetSize(boldFontSize);

	bool showThermometer = ((modifiers() & (B_CONTROL_KEY | B_COMMAND_KEY)) == (B_CONTROL_KEY | B_COMMAND_KEY));

	SetViewColor(kLightGray);
		
	const float kMargin = 5.0;
	const float kIndentation = 0.0;
	const float kBigLogoHeight = 60.0;
	const float kItemSpacing = 14.0;
	const float kItemPadding = showThermometer ? 8.0 : 10.0;
	
	BRect rect(Bounds());
	rect.top = kBigLogoHeight;
	rect.right = kDivider - 2;
	rect.InsetBy(10, 10);
	
	// Auto scrolling credits, hidden by default
	fCreditsView = new CreditsAutoScrollView(rect, kCreditsItemName, &plainFont);
	fCreditsView->Hide();
	AddChild(fCreditsView);

	rect.bottom = rect.top + kItemSpacing;
	rect.left = kMargin;
	rect.right = kDivider - 3 - kIndentation;
	
	// Fetch some important system information...
	BString string;
	system_info sysInfo;
	get_system_info(&sysInfo);
	GetPlatformType(string, &sysInfo);
		
	BStringView *stringView = new BStringView(rect, kSysInfoItemName, "Platform:", 0);
	stringView->SetFont(&boldFont);
	AddChild(stringView);
	rect.OffsetBy(kIndentation, kItemSpacing);
	stringView = new BStringView(rect, kSysInfoItemName, string.String(), 0);
	stringView->SetFont(&plainFont);
	AddChild(stringView);

	// Show the CPU type
	GetCPUType(string, &sysInfo);
	rect.OffsetBy(-kIndentation, kItemSpacing + kItemPadding);
	stringView = new BStringView(rect, kSysInfoItemName, "CPU:", 0);
	stringView->SetFont(&boldFont);
	AddChild(stringView);
	rect.OffsetBy(kIndentation, kItemSpacing);
	stringView = new BStringView(rect, kSysInfoItemName, string.String(), 0);
	stringView->SetFont(&plainFont);
	AddChild(stringView);

	// Show the Kernel version
	string = sysInfo.kernel_build_date;
	string << ' ' << sysInfo.kernel_build_time;
	rect.OffsetBy(-kIndentation, kItemSpacing + kItemPadding);
	stringView = new BStringView(rect, kSysInfoItemName, "Kernel:", 0);
	stringView->SetFont(&boldFont);
	AddChild(stringView);
	rect.OffsetBy(kIndentation, kItemSpacing);
	stringView = new BStringView(rect, kSysInfoItemName, string.String(), 0);
	stringView->SetFont(&plainFont);
	AddChild(stringView);

	// Show the libbe version (closest thing we have to a system wide
	// version number)
	BFile libbe(kLibbePath, B_READ_ONLY);
	if (libbe.InitCheck() == B_OK) {
		BAppFileInfo info(&libbe);
		if (info.InitCheck() == B_OK) {
			version_info version;
			if (info.GetVersionInfo(&version, B_APP_VERSION_KIND) == B_OK) {
				rect.OffsetBy(-kIndentation, kItemSpacing + kItemPadding);
				stringView = new BStringView(rect, kSysInfoItemName, "System Version:", 0);
				stringView->SetFont(&boldFont);
				AddChild(stringView);
				rect.OffsetBy(kIndentation, kItemSpacing);
				stringView = new BStringView(rect, kSysInfoItemName, version.short_info, 0);
				stringView->SetFont(&plainFont);
				AddChild(stringView);
			}
		}
	}

	// show uptime
	rect.OffsetBy(-kIndentation, kItemSpacing + kItemPadding);
	stringView = new BStringView(rect, kSysInfoItemName, "Running:", 0);
	stringView->SetFont(&boldFont);
	AddChild(stringView);
	GetUptimeString(string);
	rect.OffsetBy(kIndentation, kItemSpacing);
	fUptime = new FlickerFreeStringView(rect, kSysInfoItemName, string.String());
	fUptime->SetFont(&plainFont);
	AddChild(fUptime);

	// show the memory size
	rect.OffsetBy(-kIndentation, kItemSpacing + kItemPadding);
	stringView = new BStringView(rect, kSysInfoItemName, "Memory:", 0);
	stringView->SetFont(&boldFont);
	AddChild(stringView);
	string = "";
	string << (sysInfo.max_pages * B_PAGE_SIZE >> 10) << " KB total";
	rect.OffsetBy(kIndentation, kItemSpacing);
	rect.right = rect.Width() / 2;
	stringView = new BStringView(rect, kSysInfoItemName, string.String(), 0);
	stringView->SetFont(&plainFont);
	AddChild(stringView);

	if (showThermometer) {
		// optionally show the physical memory use bar
		BRect trailerRect = rect;
		trailerRect.left = trailerRect.right;
		trailerRect.right = kDivider - 3 - kIndentation;
		string = "";
		string << ((sysInfo.max_pages - sysInfo.used_pages) * B_PAGE_SIZE >> 10) << " KB free";
		fMemoryFree = new FlickerFreeStringView(trailerRect, kSysInfoItemName, string.String());
		fMemoryFree->SetFont(&plainFont);
		AddChild(fMemoryFree);
	
		rect.OffsetBy(-kIndentation, kItemSpacing + (kItemPadding / 2));
		rect.left = kMargin * 3;
		rect.right = kDivider - 2 - (kMargin * 3);
		rect.bottom = rect.top + kItemSpacing;
	
		fThermometer = new Thermometer(rect, kSysInfoItemName, sysInfo.max_pages * B_PAGE_SIZE, sysInfo.used_pages * B_PAGE_SIZE);
		AddChild(fThermometer);
	}
	
	// Make sure we're not off the view....	
	BRect legalRect(Bounds());
	legalRect.left = kDivider;
	fLegalView = new LicenseInfoView(legalRect, &plainFont);
	AddChild(fLegalView);
	
	GetLibbeResources()->GetBitmapResource('BBMP', 100, &fBigLogo);
}

AboutView::~AboutView()
{
	delete fBigLogo;
}

void AboutView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
}

void AboutView::Draw(BRect /* rect */)
{
	BRect aRect(Bounds());
	aRect.right = kDivider - 2;
	
	SetHighColor(kLightGray);
	FillRect(aRect);
	
	BeginLineArray(2);
		AddLine(BPoint(kDivider - 2, 0), BPoint(kDivider - 2, Bounds().Height()), kDarkGray);
		AddLine(BPoint(kDivider - 1, 0), BPoint(kDivider - 1, Bounds().Height()), kDarkerGray);
	EndLineArray();
	
	aRect.left = kDivider;
	aRect.right = Bounds().Width();
	
	SetHighColor(kWhite);
	FillRect(aRect);
	
	if(fBigLogo)
		DrawBitmap(fBigLogo);

}

void AboutView::UpdateInfo()
{
	if (fThermometer) {
		system_info sysinfo;
		get_system_info(&sysinfo);
	
		// some fields do not need to be updated
		if (fOldSysInfo.used_pages != sysinfo.used_pages) {

			fOldSysInfo = sysinfo;
			BString buffer;
			buffer << ((fOldSysInfo.max_pages - fOldSysInfo.used_pages) * B_PAGE_SIZE >> 10) << " KB free";
			fMemoryFree->SetText(buffer.String());
			fThermometer->SetValue(fOldSysInfo.used_pages * B_PAGE_SIZE);
		}
	}

	// update uptime
	BString string;
	if (GetUptimeString(string)) 
		fUptime->SetText(string.String());
}

bool AboutView::GetUptimeString(BString &string)
{
	bigtime_t now = system_time();

	now /= 1000000;
	if (fLastUptimeInSeconds == now)
		return false;
	
	fLastUptimeInSeconds = now;
	string = "";
	int32 days = 0;
	if (now >= 24 * 60 * 60) {
		days = now / (24 * 60 * 60);
		string << days << (days != 1 ? " days, " : " day, ");
	}
	now %= (24 * 60 * 60);
	if (now > 60 * 60) {
		int32 hours = now / (60 * 60);
		string << hours << (hours != 1 ? " hours, " : " hour, ");
	}

	now %= (60 * 60);
	int32 minutes = now / (60);
	string << minutes << (minutes != 1 ? " minutes" : " minute");

	if (!days) {
		now %= 60;
		string << ", " << now << (now != 1 ? " seconds" : " second");
	}
	return true;
}

void AboutView::Pulse()
{
	UpdateInfo();
}

void AboutView::MouseDown(BPoint point)
{
	if(fBigIconRect.Contains(point))
		ToggleMode(point);
}

void AboutView::ToggleMode(BPoint point)
{
	fSysInfoMode = !fSysInfoMode;
	for (int32 index = CountChildren() -  1; index >= 0; index--) {
		BView *view = ChildAt(index);
		if (strcmp(view->Name(), kSysInfoItemName) == 0) {
			if (fSysInfoMode)
				view->Show();
			else
				view->Hide();
		} else if (strcmp(view->Name(), kCreditsItemName) == 0) {
			if (fSysInfoMode)
				view->Hide();
			else {
				dynamic_cast<CreditsAutoScrollView *>(view)->
					SetShowSecretText(kHotSpotRect.Contains(point));
				view->Show();
			}
		}
	}
	Window()->SetPulseRate(fSysInfoMode ? 1000000 : 100000);
}

// ----------------------------
//	CreditsAutoScrollView		
// ----------------------------
// #pragma mark -

CreditsAutoScrollView::CreditsAutoScrollView(BRect frame, const char *name, const BFont *font)
	:	BScrollView(name,
			fTextView = new BTextView(frame, "", BRect(0, 0, frame.Width(), 1000),
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE),
			B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED, false, false),
		fShowSecretText(false)
{
	fTextView->SetStylable(true);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	fTextView->MakeEditable(false);
	fTextView->SetViewColor(kBlack);
	fTextView->SetAlignment(B_ALIGN_CENTER);
	fTextView->SetFontAndColor(font);
}

void CreditsAutoScrollView::Show()
{
	bool altCredits = fShowSecretText
		&& ((modifiers() & (B_SHIFT_KEY | B_OPTION_KEY | B_CONTROL_KEY))
			== (B_SHIFT_KEY | B_OPTION_KEY | B_CONTROL_KEY));

	fTextView->SetText(altCredits ? kCreditsString : kSanitizedCreditsString);
	fTextView->ScrollTo(0, 0);
	fTextView->SetFontAndColor(0, fTextView->TextLength(), 0, 0, altCredits ? &kAlternateTextColor : &kTextColor);

	_inherited::Show();
}

void CreditsAutoScrollView::Pulse()
{
	if (IsHidden())
		return;

	// keep loopin the show
	fTextView->ScrollBy(0, 1);
	
	// are we past last character ??
	BPoint lastPoint = fTextView->PointAt(fTextView->TextLength() + 1);
	if (fTextView->LeftTop().y > lastPoint.y)
		fTextView->ScrollTo(0, 0);
}

void CreditsAutoScrollView::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		const rgb_color *color;
		ssize_t size;
		if (message->FindData("RGBColor", 'RGBC', (const void **)&color, &size) == B_OK) {
			if (modifiers() & B_OPTION_KEY) 
				fTextView->SetFontAndColor(0, fTextView->TextLength(), 0, 0, color);
			else
				fTextView->SetViewColor(*color);

			fTextView->Invalidate();
			return;
		}
	}
	_inherited::MessageReceived(message);
}

// ----------------------------
// 		LicenseInfoView		
// ----------------------------
// #pragma mark -

LicenseInfoView::LicenseInfoView(BRect frame, const BFont *font)
	:	BView(frame, "LicensenInfoView", B_FOLLOW_ALL, B_WILL_DRAW),
		fFont(*font)
{
	SetViewColor(kWhite);
}

void LicenseInfoView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// Add all the view
	float top = 0.0;
	float spacing = 5.0;
	
	// Haha, this has got to be the most hard-coded UI code
	// I've ever written, however, I've been "informed" that
	// the about box needs to be done \yesterday\, so here
	// it is. It also doesn't help when no-one can seem to agree
	// on a UI layout... <anonymous coward>
	LicenseItem *item;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),55), "corporate", 101, kCorporateLicense, &fFont));
	top = item->Frame().bottom + spacing;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),top + 28), "logo", 102, kLogoLicense, &fFont));
	top = item->Frame().bottom + spacing;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),top + 28), "real", 103, kRealLicense, &fFont));
	top = item->Frame().bottom + spacing;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),top + 43), "fraunhofer", 104, kFraunhoferLicense, &fFont));
	top = item->Frame().bottom + spacing;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),top + 28), "rsa", 105, kRSALicense, &fFont));
	top = item->Frame().bottom + spacing;
	AddChild(item = new LicenseItem(BRect(0,top,Bounds().Width(),top + 40), "indeo", -1, kIndeoLicense, &fFont));
}

// ----------------------------
// 		   LicenseItem		
// ----------------------------
// #pragma mark -

LicenseItem::LicenseItem(BRect frame, const char *name, int32 iconId, const char *text, const BFont *font)
	:	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW),
		fText(text),
		fLogo(NULL)
{
	SetViewColor(kWhite);
	GetLibbeResources()->GetBitmapResource('BBMP', iconId, &fLogo);
		
	BRect rect(frame);
	rect.OffsetTo(0,0);
	rect.left = 60.0;
	fTextView = new BTextView(rect, "licenseTextView", BRect(2, 2, rect.Width() - 2, rect.Height() - 2), B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED); 
	fTextView->SetStylable(true);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	fTextView->MakeEditable(false);
	rgb_color color(make_color(0, 0, 0, 255));
	fTextView->SetFontAndColor(font, B_FONT_ALL, &color);
	fTextView->SetText(text);
	fTextView->SetViewColor(make_color(255, 255, 255, 255));
	fTextView->SetLowColor(make_color(255, 255, 255, 255));
	AddChild(fTextView);
}

LicenseItem::~LicenseItem()
{
	if(fLogo)
		delete fLogo;
}

void LicenseItem::Draw(BRect /* rect */)
{
	if(fLogo)
		DrawBitmap(fLogo, BPoint(5, (Bounds().Height() - fLogo->Bounds().Height()) / 2));
}

// ----------------------------
// 		   Thermometer		
// ----------------------------
// #pragma mark -

Thermometer::Thermometer(BRect frame, const char *name, float maxValue, float initialValue)
	:	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW),
		fMaxValue(maxValue),	
		fValue(initialValue)
{
	// don't erase us, we are drawn offscreen
	SetViewColor(B_TRANSPARENT_32_BIT);
}

void Thermometer::SetValue(float newValue)
{
	if (fValue == newValue)
		return;
	
	fValue = newValue;
	Invalidate();
}

void Thermometer::Draw(BRect)
{
	BRect bounds(Bounds());
	OffscreenBitmap bitmap;
	bitmap.BeginUsing(bounds);
	
	rgb_color base = LowColor();

	if (base.red == 255 && base.green == 255 && base.blue == 255)
		base.red = base.green = base.blue = 216;

	bitmap.View()->BeginLineArray(8);
	bitmap.View()->AddLine(bounds.LeftBottom(), bounds.RightBottom(), tint_color(base, 0.3));
	bitmap.View()->AddLine(bounds.RightBottom(), bounds.RightTop(), tint_color(base, 0.3));
	bitmap.View()->AddLine(bounds.RightTop(), bounds.LeftTop(), tint_color(base, 1.2));
	bitmap.View()->AddLine(bounds.LeftTop(), bounds.LeftBottom(), tint_color(base, 1.2));
	bounds.InsetBy(1,1);
	bitmap.View()->AddLine(bounds.LeftBottom(), bounds.RightBottom(), tint_color(base, 1.4));
	bitmap.View()->AddLine(bounds.RightBottom(), bounds.RightTop(), tint_color(base, 1.4));
	bitmap.View()->AddLine(bounds.RightTop(), bounds.LeftTop(), tint_color(base, 1.6));
	bitmap.View()->AddLine(bounds.LeftTop(), bounds.LeftBottom(), tint_color(base, 1.6));
	bitmap.View()->EndLineArray();

	bounds.InsetBy(1, 1);
	
	BRect emptyRect(bounds);
	bounds.right = bounds.left + ((fValue / fMaxValue) * bounds.Width());
	emptyRect.left = bounds.right + 1;
	
	if (bounds.right > bounds.left + 1) {
		bitmap.View()->BeginLineArray(4);
		bitmap.View()->AddLine(bounds.LeftBottom() + BPoint(0,-1), bounds.LeftTop(), tint_color(kBarColor, 0.6));
		bitmap.View()->AddLine(bounds.LeftTop(), bounds.RightTop() + BPoint(-1, 0), tint_color(kBarColor, 0.6));

		bitmap.View()->AddLine(bounds.RightTop(), bounds.RightBottom(), tint_color(kBarColor, 1.4));
		bitmap.View()->AddLine(bounds.RightBottom(), bounds.LeftBottom(), tint_color(kBarColor, 1.4));
		bitmap.View()->EndLineArray();
		bounds.InsetBy(1,1);

		bitmap.View()->SetHighColor(kBarColor);
		bitmap.View()->FillRect(bounds);
	}

	bitmap.View()->SetHighColor(255, 255, 255, 0);
	bitmap.View()->FillRect(emptyRect);
	bitmap.View()->Sync();

	SetDrawingMode(B_OP_COPY);
	DrawBitmap(bitmap.Bitmap());
	bitmap.DoneUsing();
}

// ----------------------------
// 	  FlickerFreeStringView		
// ----------------------------
// #pragma mark -

FlickerFreeStringView::FlickerFreeStringView(BRect bounds, const char *name, const char *text)
	:	BStringView(bounds, name, text, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		fBitmap(NULL),
		fOrigBitmap(NULL)
{

}

FlickerFreeStringView::~FlickerFreeStringView()
{
	delete fBitmap;
}

void FlickerFreeStringView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	if (Parent()) {
		fViewColor = Parent()->ViewColor();
		fLowColor = Parent()->ViewColor();
	}
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(B_TRANSPARENT_32_BIT);
}

void FlickerFreeStringView::Draw(BRect)
{
	BRect bounds(Bounds());
	if (!fBitmap)
		fBitmap = new OffscreenBitmap(Bounds());
	
	BView *offscreen = fBitmap->BeginUsing(bounds);

	if (Parent()) {
		fViewColor = Parent()->ViewColor();
		fLowColor = Parent()->ViewColor();
	}

	offscreen->SetViewColor(fViewColor);
	offscreen->SetHighColor(HighColor());
	offscreen->SetLowColor(fLowColor);

	BFont font;
    GetFont(&font);
	offscreen->SetFont(&font);

	offscreen->Sync();
	if (fOrigBitmap)
		offscreen->DrawBitmap(fOrigBitmap, Frame(), bounds);
	else
		offscreen->FillRect(bounds, B_SOLID_LOW);

	if (Text()) {
		BPoint loc;

		font_height	height;
		GetFontHeight(&height);

		edge_info eInfo;
		switch (Alignment()) {
			case B_ALIGN_LEFT: {
				// If the first char has a negative left edge give it
				// some more room by shifting that much more to the right.
				font.GetEdges(Text(), 1, &eInfo);
				loc.x = bounds.left + (2 - eInfo.left);
				break;
			}
			case B_ALIGN_CENTER: {
				float width = StringWidth(Text());
				float center = (bounds.right - bounds.left) / 2;
				loc.x = center - (width/2);
				break;
			}
			case B_ALIGN_RIGHT: {
				float width = StringWidth(Text());
				loc.x = bounds.right - width - 2;
				break;
			}
		}
		loc.y = bounds.bottom - (1 + height.descent);
		offscreen->MovePenTo(loc);
		offscreen->DrawString(Text());
	}
	offscreen->Sync();
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap->Bitmap());
	fBitmap->DoneUsing();
}

void FlickerFreeStringView::SetViewColor(rgb_color color)
{
	if (fViewColor != color) {
		fViewColor = color;
		Invalidate();
	}
	_inherited::SetViewColor(B_TRANSPARENT_32_BIT);
}

void FlickerFreeStringView::SetLowColor(rgb_color color)
{
	if (fLowColor != color) {
		fLowColor = color;
		Invalidate();
	}
	_inherited::SetLowColor(B_TRANSPARENT_32_BIT);
}

// ----------------------------
// 	  OffscreenBitmap		
// ----------------------------
// #pragma mark -

OffscreenBitmap::OffscreenBitmap(BRect frame)
	:	fBitmap(NULL)
{
	NewBitmap(frame);
}

OffscreenBitmap::OffscreenBitmap()
	:	fBitmap(NULL)
{
}

OffscreenBitmap::~OffscreenBitmap()
{
	delete fBitmap;
}

void OffscreenBitmap::NewBitmap(BRect bounds)
{
	delete fBitmap;
	fBitmap = new BBitmap(bounds, B_COLOR_8_BIT, true);
	if (fBitmap->Lock()) {
		BView *view = new BView(fBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
		fBitmap->AddChild(view);

		BRect clipRect = view->Bounds();
		BRegion newClip;
		newClip.Set(clipRect);
		view->ConstrainClippingRegion(&newClip);

		fBitmap->Unlock();
	} else {
		delete fBitmap;
		fBitmap = NULL;
	}
}

BView *OffscreenBitmap::BeginUsing(BRect frame)
{
	if (!fBitmap || fBitmap->Bounds() != frame)
		NewBitmap(frame);
	fBitmap->Lock();
	return View();
}

void OffscreenBitmap::DoneUsing()
{
	fBitmap->Unlock();
}

BBitmap *OffscreenBitmap::Bitmap() const
{
	return fBitmap;
}

BView * OffscreenBitmap::View() const
{
	return fBitmap->ChildAt(0);
}

#endif

// ----------------------------
// 	  BImageResources		
// ----------------------------
// #pragma mark -

BImageResources::BImageResources(void *memAddr)
{
	image_id image = find_image(memAddr);
	image_info info;
	if (get_image_info(image, &info) == B_OK) {
#if _SUPPORTS_RESOURCES
		BFile file(&info.name[0], B_READ_ONLY);
#else
		BString name(&info.name[0]);
		name += ".rsrc";
		BFile file(name.String(), B_READ_ONLY);
#endif
		if (file.InitCheck() == B_OK) 
			fResources.SetTo(&file);
	}
}

BImageResources::~BImageResources()
{
}

const BResources *
BImageResources::ViewResources() const
{
	if (fLock.Lock() != B_OK)
		return NULL;
	
	return &fResources;
}

BResources *
BImageResources::ViewResources()
{
	if (fLock.Lock() != B_OK)
		return NULL;
	
	return &fResources;
}

status_t
BImageResources::FinishResources(BResources *res) const
{
	ASSERT(res == &fResources);
	if (res != &fResources)
		return B_BAD_VALUE;

	fLock.Unlock();
	return B_OK;
}
	
const void *
BImageResources::LoadResource(type_code type, int32 id, size_t *out_size) const
{	
	// Serialize execution.
	// Looks like BResources is not really thread safe. We should
	// clean that up in the future and remove the locking from here.
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return 0;
	
	// Return the resource.  Because we never change the BResources
	// object, the returned data will not change until TTracker is
	// destroyed.
	return const_cast<BResources *>(&fResources)->LoadResource(type, id, out_size);
}

const void *
BImageResources::LoadResource(type_code type, const char *name, size_t *out_size) const
{
	// Serialize execution.
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return NULL;
	
	// Return the resource.  Because we never change the BResources
	// object, the returned data will not change until TTracker is
	// destroyed.
	return const_cast<BResources *>(&fResources)->LoadResource(type, name, out_size);
}


status_t BImageResources::GetBitmapResource(type_code type, int32 id, BBitmap** out) const
{
	*out = 0;
	
	size_t len = 0;
	const void *data = LoadResource(type, id, &len);

	if (data == 0) {
		TRESPASS();
		return B_ERROR;
	}
	
	BMemoryIO stream(data, len);
	
	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err;
	if( (err=archive.Unflatten(&stream)) == B_OK ) {
		*out = new BBitmap(&archive);
		if( !(*out) ) err = B_ERROR;
		else if( (err=(*out)->InitCheck()) != B_OK ) {
			delete *out;
			*out = 0;
		}
	}
	
	return err;
}
	
image_id
BImageResources::find_image(void *memAddr) const
{
	image_info info; 
	int32 cookie = 0; 
	while (get_next_image_info(0, &cookie, &info) == B_OK) 
		if ((info.text <= memAddr && (((uint8 *)info.text)+info.text_size) > memAddr)
			||(info.data <= memAddr && (((uint8 *)info.data)+info.data_size) > memAddr)) 
			// Found the image.
			return info.id;
	
	return -1;
}

static BLocker resLock;
static BImageResources *resources = NULL;

// This class is used as a static instance to delete the resources
// global object when the image is getting unloaded.
class _TTrackerCleanupResources
{
public:
	_TTrackerCleanupResources() { }
	~_TTrackerCleanupResources()
	{
		delete resources;
		resources = NULL;
	}
};

namespace BTrackerTheft {

static _TTrackerCleanupResources CleanupResources;


BImageResources *GetLibbeResources()
{
	if (!resources) {
		BAutolock lock(&resLock);
		resources = new BImageResources(&resources);
	}
	return resources;
}

}

// End of AboutBox.cpp
