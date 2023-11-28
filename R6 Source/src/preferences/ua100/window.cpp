#include <Application.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <unistd.h>

#include "channel.h"
#include "knob.h"
#include "midi.h"
#include "ua100.h"
#include "settings.h"

#define WIN_SIZE BPoint(490, CHANNEL_Y + CHANNEL_I)
#define BUTTON_SIZE BPoint(24, 20)
#define PAN_Y 55

MixWindow::MixWindow(BPoint where, char* name, int fd)
  : BWindow(BRect(where, where + WIN_SIZE), name, B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	operating_mode(0),
	input_mode(0)
{
  Lock();
  top = new UTopView(Bounds(), name, fd);
  AddChild(top);
  Unlock();
  Show();
  reader = new Reader(fd);
  reader->Run();
  // start initialization sequence
  //  RQ1(fd, 0x400000);
  use_mic1(fd);
}

MixWindow::~MixWindow()
{
  bool last_window = true;

  for (int i = 0; i < MAX_DEVS; i++)
	if (my_app->mix_wins[i])
	  if (my_app->mix_wins[i] == this)
		my_app->mix_wins[i] = NULL;
	  else
		last_window = false;

  close(reader->fd);
  delete(reader);

  if (last_window)
	be_app->PostMessage(B_QUIT_REQUESTED);
}

void
MixWindow::EnableEffect(int32 n)
{
  if (Lock()) {
	for (int i = 1; i < 7; i++)
	  if (i == n || operating_mode == 4 || (n < 5 && i < 5)) {
		leds[i]->SetValue(i == n);
		leds[i]->Invalidate();
	  }
	Unlock();
  }
}

void
MixWindow::UpdateOutputMenu(BMenu* menu)
{
  static char* name[] = {"Mic1/Gtr", "Line", "Mic1+Mic2"};
  bool vt = (operating_mode == 1 || operating_mode == 5);
  menu->ItemAt(0)->SetLabel(name[input_mode]);
  menu->ItemAt(1)->SetEnabled(input_mode == 0);
  menu->ItemAt(4)->SetEnabled(!vt);
  menu->ItemAt(5)->SetEnabled(!vt && input_mode == 0);
  menu->ItemAt(6)->SetEnabled(!vt);
  menu->ItemAt(7)->SetEnabled(!vt);
  menu->ItemAt(8)->SetLabel(vt ? "VT Out" : "Sub Mix");
  menu->FindMarked()->SetMarked(true);
}

void
MixWindow::SetOperatingMode(int32 v)
{
  //fprintf(stderr, "SetOperatingMode(%d)\n", v);
  if (Lock()) {
	operating_mode = v;
	UpdateOutputMenu(top->master_menu->Menu());
	UpdateOutputMenu(top->wave_menu->Menu());
	BMenu* menu = top->insertion_menu->Menu();
	for (int i = 0; i < 5; i++)
	  menu->ItemAt(i)->SetEnabled(v == 4);
	for (int i = 14; i < 70; i++)
	  menu->ItemAt(i)->SetEnabled(v != 4);
	for (int i = 0; i < 2; i++)
	  if (v == 4) {
		if (!top->sys_menu[i]->IsHidden())
		  top->sys_menu[i]->Hide();
	  }
	  else {
		if (top->sys_menu[i]->IsHidden())
		  top->sys_menu[i]->Show();
	  }
	top->Invalidate(BRect(40 * 8, CHANNEL_I - 28,
						  40 * 11, CHANNEL_I + 28));
	Unlock();
  }
  my_app->SendAllControls(leds[1]->fd);
}

void
MixWindow::SetInputMode(int32 v)
{
  if (input_mode != v && Lock()) {
	input_mode = v;
	UpdateOutputMenu(top->master_menu->Menu());
	UpdateOutputMenu(top->wave_menu->Menu());
	if (input_mode == 0) {
	  top->channels[0]->ResizeTo(CHANNEL_X, CHANNEL_Y);
	  if (top->channels[1]->IsHidden())
		top->channels[1]->Show();
	}
	else {
	  top->channels[0]->ResizeTo(2 * CHANNEL_X, CHANNEL_Y);
	  if (!top->channels[1]->IsHidden())
		top->channels[1]->Hide();
	}

	for (int i = 0; i < 2; i++)
	  if (input_mode == 1) {
		if (!top->pans[i]->IsHidden())
		  top->pans[i]->Hide();
	  }
	  else if (top->pans[i]->IsHidden())
		top->pans[i]->Show();
	top->Invalidate(BRect(40, 0, 120, PAN_Y));
	Unlock();
  }
}

UTopView::UTopView(BRect frame, char* name, int fd)
  : UView(frame, name, fd)
{
}

void
UTopView::AttachedToWindow()
{
  static char* cnames[] = {
	" Mic1", "  Mic2", "Wave1", "Wave2"
  };
  BPopUpMenu* pum;
  int32 a;

  if (fd < 0)
	return;

  SetViewColor(GRAY);
  SetLowColor(GRAY);

  pans[0] = AddKnob(BPoint(42, PAN_Y), "pan1", 0x401001, channel_color(1));
  pans[1] = AddKnob(BPoint(82, PAN_Y), "pan2", 0x401002, channel_color(2));

  for (int i = 0; i < 4; i++) {
	channels[i] = new UChannel(BPoint(40 * (i + 1), CHANNEL_I),
							   cnames[i], fd, i + 1);
	AddChild(channels[i]);
  }

  AddButton(BPoint(66, 3), "", 0x401000);

  AddKnob(BPoint(40 * 6, CHANNEL_I + SEND1_Y), "ret1", 0x404007, WHITE);
  AddKnob(BPoint(40 * 7, CHANNEL_I + SEND1_Y), "subret1", 0x404008, WHITE);
  AddKnob(BPoint(40 * 6, CHANNEL_I + SEND2_Y), "ret2", 0x404009, WHITE);
  AddKnob(BPoint(40 * 7, CHANNEL_I + SEND2_Y), "subret2", 0x40400a, WHITE);
  AddLED(BPoint(40 * 5 + 24, CHANNEL_I + SEND1_Y + 10), "snd1", 0x404005);
  AddLED(BPoint(40 * 5 + 24, CHANNEL_I + SEND2_Y + 10), "snd2", 0x404006);

  pum = new BPopUpMenu("Master Select");
  a = 0x405001;
  pum->AddItem(new BMenuItem("Mic1/Gtr", NewMenuMsg(a, 0)));
  pum->AddItem(new BMenuItem("Mic2", NewMenuMsg(a, 1)));
  pum->AddItem(new BMenuItem("Wave1", NewMenuMsg(a, 2)));
  pum->AddItem(new BMenuItem("Wave2", NewMenuMsg(a, 3)));
  pum->AddItem(new BMenuItem("Channel 1", NewMenuMsg(a, 4)));
  pum->AddItem(new BMenuItem("Channel 2", NewMenuMsg(a, 5)));
  pum->AddItem(new BMenuItem("Channel 3", NewMenuMsg(a, 6)));
  pum->AddItem(new BMenuItem("Channel 4", NewMenuMsg(a, 7)));
  pum->AddItem(new BMenuItem("Sub Mix", NewMenuMsg(a, 8)));
  pum->AddItem(new BMenuItem("Main Mix", NewMenuMsg(a, 9)));
  pum->AddItem(new BMenuItem("Wave(REC)", NewMenuMsg(a, 10)));
  pum->FindItem("Main Mix")->SetMarked(true);
  master_menu = AddMenu(BRect(40 * 8.2, CHANNEL_I + SOLO_Y,
							  40 * 10.2, CHANNEL_I + SOLO_Y + 20),
						"Master Select", a, pum);
  AddSlider(BRect(40 * 8, CHANNEL_I + MAIN_Y,
				  40 * 10, CHANNEL_I + MAIN_Y + MAIN_H),
			"master", 0x405003, "Master");

  pum = new BPopUpMenu("Wave Select");
  a = 0x405000;
  pum->AddItem(new BMenuItem("Mic1/Gtr", NewMenuMsg(a, 0)));
  pum->AddItem(new BMenuItem("Mic2", NewMenuMsg(a, 1)));
  pum->AddItem(new BMenuItem("Wave1", NewMenuMsg(a, 2)));
  pum->AddItem(new BMenuItem("Wave2", NewMenuMsg(a, 3)));
  pum->AddItem(new BMenuItem("Channel 1", NewMenuMsg(a, 4)));
  pum->AddItem(new BMenuItem("Channel 2", NewMenuMsg(a, 5)));
  pum->AddItem(new BMenuItem("Channel 3", NewMenuMsg(a, 6)));
  pum->AddItem(new BMenuItem("Channel 4", NewMenuMsg(a, 7)));
  pum->AddItem(new BMenuItem("Sub Mix", NewMenuMsg(a, 8)));
  pum->AddItem(new BMenuItem("Main Mix", NewMenuMsg(a, 9)));
  pum->FindItem("Mic1/Gtr")->SetMarked(true);
  wave_menu = AddMenu(BRect(40 * 10.2, CHANNEL_I + SOLO_Y,
							40 * 12.2, CHANNEL_I + SOLO_Y + 20),
					  "Wave Select", a, pum);
  AddSlider(BRect(40 * 10, CHANNEL_I + MAIN_Y,
				  40 * 12, CHANNEL_I + MAIN_Y + MAIN_H),
			"wave", 0x405002, "Wave(REC)");

  pum = new BPopUpMenu("Effects Mode");
  a = 0x404000;
  pum->AddItem(new BMenuItem("Full Effects Mode", NewMenuMsg(a, 4)));
  pum->AddItem(new BMenuItem("Compact Effects Mode", NewMenuMsg(a, 3)));
  pum->FindItem("Compact Effects Mode")->SetMarked(true);
  AddMenu(BRect(40 * 8.2, 40, 40 * 12.2, 60),
		  "Effects Mode", a, pum);

  pum = new BPopUpMenu("Effect");
  a = 0x400100;
  pum->AddItem(new BMenuItem("High Quality Reverb", NewMenuMsg(a, 0x011)));
  pum->AddItem(new BMenuItem("Mic Simulator", NewMenuMsg(a, 0x012)));
  pum->AddItem(new BMenuItem("Vocoder", NewMenuMsg(a, 0x013)));
  pum->AddItem(new BMenuItem("Vocal Multi", NewMenuMsg(a, 0x014)));
  pum->AddItem(new BMenuItem("Game", NewMenuMsg(a, 0x016)));
  pum->AddItem(new BMenuItem("Rotary Multi", NewMenuMsg(a, 0x300)));
  pum->AddItem(new BMenuItem("Guitar Multi 1", NewMenuMsg(a, 0x400)));
  pum->AddItem(new BMenuItem("Guitar Multi 2", NewMenuMsg(a, 0x401)));
  pum->AddItem(new BMenuItem("Guitar Multi 3", NewMenuMsg(a, 0x402)));
  pum->AddItem(new BMenuItem("Clean Guitar Multi 1", NewMenuMsg(a, 0x403)));
  pum->AddItem(new BMenuItem("Clean Guitar Multi 2", NewMenuMsg(a, 0x404)));
  pum->AddItem(new BMenuItem("Bass Multi", NewMenuMsg(a, 0x405)));
  pum->AddItem(new BMenuItem("Electric Piano Multi", NewMenuMsg(a, 0x406)));
  pum->AddItem(new BMenuItem("Keyboard Multi", NewMenuMsg(a, 0x500)));

  pum->AddItem(new BMenuItem("Noise Supressor", NewMenuMsg(a, 0x000)));
  pum->AddItem(new BMenuItem("Stereo EQ", NewMenuMsg(a, 0x100)));
  pum->AddItem(new BMenuItem("Spectrum", NewMenuMsg(a, 0x101)));
  pum->AddItem(new BMenuItem("Enhancer", NewMenuMsg(a, 0x102)));
  pum->AddItem(new BMenuItem("Humanizer", NewMenuMsg(a, 0x103)));
  pum->AddItem(new BMenuItem("Overdrive", NewMenuMsg(a, 0x110)));
  pum->AddItem(new BMenuItem("Distortion", NewMenuMsg(a, 0x111)));
  pum->AddItem(new BMenuItem("Phaser", NewMenuMsg(a, 0x120)));
  pum->AddItem(new BMenuItem("AutoWah", NewMenuMsg(a, 0x121)));
  pum->AddItem(new BMenuItem("Rotary", NewMenuMsg(a, 0x122)));
  pum->AddItem(new BMenuItem("Stereo Flanger", NewMenuMsg(a, 0x123)));
  pum->AddItem(new BMenuItem("Step Flanger", NewMenuMsg(a, 0x124)));
  pum->AddItem(new BMenuItem("Tremolo", NewMenuMsg(a, 0x125)));
  pum->AddItem(new BMenuItem("Auto Pan", NewMenuMsg(a, 0x126)));
  pum->AddItem(new BMenuItem("Compressor", NewMenuMsg(a, 0x130)));
  pum->AddItem(new BMenuItem("Limiter", NewMenuMsg(a, 0x131)));
  pum->AddItem(new BMenuItem("Hexa Chorus", NewMenuMsg(a, 0x140)));
  pum->AddItem(new BMenuItem("Tremolo Chorus", NewMenuMsg(a, 0x141)));
  pum->AddItem(new BMenuItem("Stereo Chorus", NewMenuMsg(a, 0x142)));
  pum->AddItem(new BMenuItem("Space D", NewMenuMsg(a, 0x143)));
  pum->AddItem(new BMenuItem("3D Chorus", NewMenuMsg(a, 0x144)));
  pum->AddItem(new BMenuItem("Stereo Delay", NewMenuMsg(a, 0x150)));
  pum->AddItem(new BMenuItem("Modulation Delay", NewMenuMsg(a, 0x151)));
  pum->AddItem(new BMenuItem("3 Tap Delay", NewMenuMsg(a, 0x152)));
  pum->AddItem(new BMenuItem("4 Tap Delay", NewMenuMsg(a, 0x153)));
  pum->AddItem(new BMenuItem("Time Control Delay", NewMenuMsg(a, 0x154)));
  pum->AddItem(new BMenuItem("Reverb", NewMenuMsg(a, 0x155)));
  pum->AddItem(new BMenuItem("Gated Reverb", NewMenuMsg(a, 0x156)));
  pum->AddItem(new BMenuItem("3D Delay", NewMenuMsg(a, 0x157)));
  pum->AddItem(new BMenuItem("2 Voice Pitch Shifter", NewMenuMsg(a, 0x160)));
  pum->AddItem(new BMenuItem("Feedback Pitch Shifter", NewMenuMsg(a, 0x161)));
  pum->AddItem(new BMenuItem("3D Auto", NewMenuMsg(a, 0x170)));
  pum->AddItem(new BMenuItem("3D Manual", NewMenuMsg(a, 0x171)));
  pum->AddItem(new BMenuItem("Lo-Fi 1", NewMenuMsg(a, 0x172)));
  pum->AddItem(new BMenuItem("Lo-Fi 2", NewMenuMsg(a, 0x173)));
  pum->AddItem(new BMenuItem("ODrive => Chorus", NewMenuMsg(a, 0x200)));
  pum->AddItem(new BMenuItem("ODrive => Flanger", NewMenuMsg(a, 0x201)));
  pum->AddItem(new BMenuItem("ODrive => Delay", NewMenuMsg(a, 0x202)));
  pum->AddItem(new BMenuItem("Dist => Chorus", NewMenuMsg(a, 0x203)));
  pum->AddItem(new BMenuItem("Dist => Flanger", NewMenuMsg(a, 0x204)));
  pum->AddItem(new BMenuItem("Dist => Delay", NewMenuMsg(a, 0x205)));
  pum->AddItem(new BMenuItem("Enhancer => Chorus", NewMenuMsg(a, 0x206)));
  pum->AddItem(new BMenuItem("Enhancer => Flanger", NewMenuMsg(a, 0x207)));
  pum->AddItem(new BMenuItem("Enhancer => Delay", NewMenuMsg(a, 0x208)));
  pum->AddItem(new BMenuItem("Chorus => Delay", NewMenuMsg(a, 0x209)));
  pum->AddItem(new BMenuItem("Flanger => Delay", NewMenuMsg(a, 0x20a)));
  pum->AddItem(new BMenuItem("Chorus => Flanger", NewMenuMsg(a, 0x20b)));

  pum->AddItem(new BMenuItem("Chorus / Delay", NewMenuMsg(a, 0x1100)));
  pum->AddItem(new BMenuItem("Flanger / Delay", NewMenuMsg(a, 0x1101)));
  pum->AddItem(new BMenuItem("Chorus / Flanger", NewMenuMsg(a, 0x1102)));
  pum->AddItem(new BMenuItem("ODrive1 / ODrive2", NewMenuMsg(a, 0x1103)));
  pum->AddItem(new BMenuItem("ODrive / Rotary", NewMenuMsg(a, 0x1104)));
  pum->AddItem(new BMenuItem("ODrive / Phaser", NewMenuMsg(a, 0x1105)));
  pum->AddItem(new BMenuItem("ODrive / AutoWah", NewMenuMsg(a, 0x1106)));
  pum->AddItem(new BMenuItem("Phaser / Rotary", NewMenuMsg(a, 0x1107)));
  pum->AddItem(new BMenuItem("Phaser / AutoWah", NewMenuMsg(a, 0x1108)));
  BMenuField* field = new BMenuField(BRect(40 * 8.2, CHANNEL_I - 10,
										   40 * 13, CHANNEL_I + 10),
									 "Effect", "", pum);
  pum->SetTargetForItems(be_app);
  field->SetDivider(0);
  AddChild(field);
  insertion_menu = field;

  pum = new BPopUpMenu("System Effect 1");
  a = 0x400500;
  pum->AddItem(new BMenuItem("Delay", NewMenuMsg(a, 0x021)));
  pum->AddItem(new BMenuItem("Chorus", NewMenuMsg(a, 0x022)));
  field = new BMenuField(BRect(40 * 8.2, CHANNEL_I + 29,
							   40 * 13, CHANNEL_I + 49),
						 "System Effect 1", "", pum);
  pum->SetTargetForItems(be_app);
  field->SetDivider(0);
  AddChild(field);
  sys_menu[0] = field;

  pum = new BPopUpMenu("System Effect 2");
  a = 0x400600;
  pum->AddItem(new BMenuItem("Delay", NewMenuMsg(a, 0x031)));
  pum->AddItem(new BMenuItem("Reverb", NewMenuMsg(a, 0x032)));
  field = new BMenuField(BRect(40 * 8.2, CHANNEL_I + 52,
							   40 * 13, CHANNEL_I + 72),
						 "System Effect 2", "", pum);
  pum->SetTargetForItems(be_app);
  field->SetDivider(0);
  AddChild(field);
  sys_menu[1] = field;

  AddCheckBox(BRect(40 * 8.2, CHANNEL_I + SUB_Y + 77,
					40 * 13, CHANNEL_I + SUB_Y + 97),
			  "Copyright digital output", 0x400001);

  BButton* b = new BButton(BRect(40 * 5.7, CHANNEL_I + SUB_Y + 80,
								 40 * 7.3, CHANNEL_I + SUB_Y + 100),
						   "Listen", "Listen", NewMsg(0, 0, U_LISTEN));
  b->SetTarget(be_app);
  AddChild(b);
  b = new BButton(BRect(40 * 5.7, CHANNEL_I + SUB_Y + 120,
						40 * 7.3, CHANNEL_I + SUB_Y + 140),
				  "Use Mic1", "Use Mic1", NewMsg(0, 0, U_USE_MIC1));
  b->SetTarget(be_app);
  AddChild(b);
  b = new BButton(BRect(40 * 5.7, CHANNEL_I + SUB_Y + 160,
						40 * 7.3, CHANNEL_I + SUB_Y + 180),
				  "Use Line", "Use Line", NewMsg(0, 0, U_USE_LINE));
  b->SetTarget(be_app);
  AddChild(b);
  b = new BButton(BRect(40 * 5.7, CHANNEL_I + SUB_Y + 200,
						40 * 7.3, CHANNEL_I + SUB_Y + 220),
				  "Eff Loop", "Eff Loop", NewMsg(0, 0, U_EFF_LOOP));
  b->SetTarget(be_app);
  AddChild(b);
  b = new BButton(BRect(40 * 5.7, CHANNEL_I + SUB_Y + 240,
						40 * 7.3, CHANNEL_I + SUB_Y + 260),
				  "Karaoke", "Karaoke", NewMsg(0, 0, U_KARAOKE));
  b->SetTarget(be_app);
  AddChild(b);
}

void
UTopView::Draw(BRect)
{
  MixWindow* win = (MixWindow*) Window();
  if (fd < 0) {
	DrawString("Could not connect to " APP_NAME, BPoint(10, 20));
	return;
  }

  SetHighColor(WHITE);
  switch (win->input_mode) {
  case 0:
	DrawString("Mic1/", BPoint(48, PAN_Y - 16));
	DrawString("Guitar", BPoint(48, PAN_Y - 6));
	DrawString("Mic2", BPoint(88, PAN_Y - 16));
	break;
  case 1:
	DrawString("Line", BPoint(68, PAN_Y - 16));
	break;
  case 2:
	DrawString("Mic1", BPoint(48, PAN_Y - 16));
	DrawString("+", BPoint(74, PAN_Y - 16));
	DrawString("Mic2", BPoint(88, PAN_Y - 16));
	break;
  }
  DrawString(" Wave1", BPoint(120, PAN_Y - 16));
  DrawString(" Wave2", BPoint(160, PAN_Y - 16));

  SetHighColor(BLACK);
  DrawString("Pan", BPoint(8, PAN_Y + 18));
  DrawString("Ins", BPoint(8, CHANNEL_I + INS_Y + 3));
  DrawString("Effect", BPoint(8, CHANNEL_I + INS_Y + 13));
  DrawString("Send1", BPoint(8, CHANNEL_I + SEND1_Y + 18));
  DrawString("Send2", BPoint(8, CHANNEL_I + SEND2_Y + 18));
  DrawString("Sub", BPoint(8, CHANNEL_I + SUB_Y + 70));
  DrawString("Mute", BPoint(8, CHANNEL_I + MUTE_Y + 13));
  DrawString("Solo", BPoint(8, CHANNEL_I + SOLO_Y + 13));
  DrawString("Main", BPoint(8, CHANNEL_I + MAIN_Y + 100));

  DrawString("Return", BPoint(40 * 6.5, CHANNEL_I + INS_Y - 8));
  DrawString("Master", BPoint(40 * 6, CHANNEL_I + INS_Y + 8));
  DrawString("Sub", BPoint(40 * 7.2, CHANNEL_I + INS_Y + 8));
  DrawString("1", BPoint(40 * 5 + 10, CHANNEL_I + SEND1_Y + 18));
  DrawString("2", BPoint(40 * 5 + 10, CHANNEL_I + SEND2_Y + 18));

  if (win->operating_mode != 4) {
	DrawString("insertion effect", BPoint(340, CHANNEL_I - 12));
	DrawString("system effects", BPoint(340, CHANNEL_I + 27));
  }

  DrawString("Source Select", BPoint(326, CHANNEL_I + MUTE_Y + 12));
  DrawString("Source Select", BPoint(406, CHANNEL_I + MUTE_Y + 12));

  SetHighColor(BLUE);
  DrawString("INPUT", BPoint(102, 20));
  DrawString("SYSTEM EFFECT", BPoint(40 * 5.6, 20));
  DrawString("EFFECT", BPoint(40 * 9.4, 20));
  DrawString("OUTPUT", BPoint(40 * 9.4, CHANNEL_I + SUB_Y + 60));
  DrawString("EASY SETTING", BPoint(40 * 5.65, CHANNEL_I + SUB_Y + 60));
}
