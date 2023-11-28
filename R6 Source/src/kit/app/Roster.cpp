/*****************************************************************************

	File : Roster.cpp

	Written by: Peter Potrebic

	Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <message_strings.h>
#include <private/storage/mime_private.h>
#include <messages.h>
#include <message_util.h>
#include <session.h>

#include <OS.h>
#include <roster_private.h>
#include <input_server_private.h>
#include <AppDefsPrivate.h>
#include <Roster.h>
#include <Messenger.h>
#include <OS.h>
#include <ObjectList.h>
#include <image.h>
#include <Application.h>
#include <Resources.h>
#include <Mime.h>
#include <AppFileInfo.h>
#include <Path.h>
#include <File.h>
#include <Autolock.h>
#include <Query.h>
#include <VolumeRoster.h>
#include <FindDirectory.h>
#include <Alias.h>
#include <private/storage/walker.h>

#define CHUNK_SIZE 3

extern "C" char** environ;

const BRoster	*be_roster = NULL;

const char *ROSTER_PORT_NAME	= "_roster_port_";
const char *ROSTER_THREAD_NAME	= "_roster_thread_";
const char *APP_SIG_PREFIX		= "application/";
const char *BE_APP_SIG_PREFIX	= "application/x-vnd.Be-";
const char *ROSTER_MIME_SIG		= "application/x-vnd.Be-ROST";
const char *TASK_BAR_MIME_SIG	= "application/x-vnd.Be-TSKB";
const char *KERNEL_MIME_SIG		= "application/x-vnd.Be-KERN";
const char *APP_SERVER_MIME_SIG	= "application/x-vnd.Be-APPS";

/*---------------------------------------------------------------*/

static bool ok_to_use(BEntry *entry, BMimeType *, const char *, bool *in_trash);
static status_t resolve_ref(const entry_ref *ref, entry_ref* result);

/*---------------------------------------------------------------*/

extern "C" int	_init_roster_();
int		_init_roster_()
{
	be_roster = new BRoster;
	return B_OK;
}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/

app_info::app_info()
{
	thread = -1;
	team = -1;
	port = -1;
	signature[0] = '\0';
	flags = DEFAULT_APP_FLAGS;
}

/*---------------------------------------------------------------*/

app_info::~app_info()
{
}

char *to_new_sig(uint32 osig, char *buffer)
{
#if BYTE_ORDER == __LITTLE_ENDIAN
	uint32 new_osig;
	new_osig = (osig >> 24) | ((osig >> 8) & 0xff00) |
		((osig << 8) & 0xff0000) | (osig << 24);
	sprintf(buffer, "%s%.4s", BE_APP_SIG_PREFIX, (char *) &new_osig);
#else
	sprintf(buffer, "%s%.4s", BE_APP_SIG_PREFIX, (char *) &osig);
#endif
	return buffer;
}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/

BRoster::BRoster()
	: fMess(ROSTER_MIME_SIG, -1, NULL), fMimeMess()
{
//+	PRINT(("thread=%d, roster is_valid=%d\n", find_thread(NULL), fMess.IsValid()));
}

/*---------------------------------------------------------------*/

BRoster::~BRoster()
{
}

/*---------------------------------------------------------------*/

void BRoster::InitMessengers()
{
	status_t	err;
	BMessage	msg(CMD_GET_MIME_HANDLER);
	BMessage	reply;
	if ((err = fMess.SendMessage(&msg, &reply)) == B_OK) {
		reply.FindMessenger("messenger", &fMimeMess);
//+		PRINT(("thread=%d, mime is_valid=%d\n",
//+			find_thread(NULL), fMimeMess.IsValid()));
	}
}

/*---------------------------------------------------------------*/

status_t BRoster::FindApp(entry_ref *file, entry_ref *app) const
{
	uint32		appFlags;
	status_t	err;
	char		mime_sig[B_MIME_TYPE_LENGTH];
	entry_ref	the_ref;
	bool		arg_is_doc = false;
		
	if (!file)
		return B_BAD_VALUE;

	entry_ref real_file;
	err = resolve_ref(file, &real_file);
	if (err != B_OK)
		return err;

	err = resolve_app(NULL, &real_file, &the_ref, mime_sig, &appFlags, &arg_is_doc);
	*app = the_ref;

//+	PRINT(("determine_app, err=%x\n", err));
	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::FindApp(const char *in_sig, entry_ref *app) const
{
	uint32		appFlags;
	status_t	err;
	char		mime_sig[B_MIME_TYPE_LENGTH];
	entry_ref	the_ref;
	bool		arg_is_doc = false;
		
	if (!in_sig)
		return B_BAD_VALUE;

	err = resolve_app(in_sig, NULL, &the_ref, mime_sig, &appFlags, &arg_is_doc);
	*app = the_ref;

//+	PRINT(("determine_app, err=%x\n", err));
	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::Launch(const char *mime_sig, BMessage *initial_msg,
	team_id *app_team) const
{
	BList *list = NULL;
	if (initial_msg) {
		list = new BList();
		list->AddItem(initial_msg);
	}

	return xLaunchAppPrivate(mime_sig, NULL, list, 0, NULL, app_team);
}

/*---------------------------------------------------------------*/

status_t BRoster::Launch(const char *mime_sig, BList *message_list,
	team_id *app_team) const
{
	return xLaunchAppPrivate(mime_sig, NULL, message_list, 0, NULL,
		app_team);
}

/*---------------------------------------------------------------*/

status_t BRoster::Launch(const char *mime_sig, int argc, char **args,
	team_id *app_team) const
{
	return xLaunchAppPrivate(mime_sig, NULL, NULL, argc, args, app_team);
}

/*---------------------------------------------------------------*/

status_t 
BRoster::Launch(const entry_ref *ref, const BMessage *initialMessage,
	team_id *appTeam) const
{
	BObjectList<BMessage> *tmp = 0;
	if (initialMessage) {
		// This could be made more optimal by modifying xLaunchAppPrivate
		// to lazily create new BMessage items if it needs to
		// replace the ref; probably isn't a practical issue though
		tmp = new BObjectList<BMessage>(true, 1);
		tmp->AddItem(new BMessage(*initialMessage));
	}

	status_t result = xLaunchAppPrivate(NULL, ref,
		tmp ? tmp->AsBList() : NULL, 0, NULL, appTeam);

	delete tmp;
	return result;
}

/*---------------------------------------------------------------*/

status_t 
BRoster::Launch(const entry_ref *ref, const BList *messageList,
	team_id *appTeam) const
{
	BObjectList<BMessage> *tmp = 0;
	if (messageList) {
		int32 count = messageList->CountItems();
		tmp = new BObjectList<BMessage>(true, messageList->CountItems());
		for (int32 index = 0; index < count; index++)
			tmp->AddItem(new BMessage(*(const BMessage *)(messageList->ItemAt(index))));
	}
	status_t result = xLaunchAppPrivate(NULL, ref,
		tmp ? tmp->AsBList() : NULL, 0, NULL, appTeam);

	delete tmp;
	return result;
}

/*---------------------------------------------------------------*/

status_t 
BRoster::Launch(const entry_ref *ref, int argc, const char *const *args,
	team_id *appTeam) const
{
	// note: xLaunchAppPrivate doesn't really modify args in any way
	// The right thing to do would be to change the prototype of
	// xLaunchAppPrivate but that would be an incompatible change
	return xLaunchAppPrivate(NULL, ref, NULL,
		argc, const_cast<char **>(args), appTeam);
}


/*---------------------------------------------------------------*/

status_t BRoster::xLaunchAppPrivate(const char *in_sig, const entry_ref *in_ref,
	BList* msg_list, int cargs, char **args, team_id *app_team) const
{
	int			argc;
	char		**argv = NULL;
	port_id		app_port = -1;
	status_t	err;
	thread_id 	tid = -1;

	if (cargs == 0)
		args = NULL;

	if (app_team)
		*app_team = -1;	// set to invalid team_id

	// must pass either a valid sig OR a entry_ref to this method
	if (!in_sig && !in_ref) 
		return B_BAD_VALUE;

	entry_ref file_ref;
	if (in_ref) {
		err = resolve_ref(in_ref, &file_ref);
		if (err != B_OK)
			return err;
		in_ref = &file_ref;
	}

	if (msg_list) {
		int	i = 0;
		BMessage	*m;
		while ((m = (BMessage *) msg_list->ItemAt(i++)) != 0) {
			// only know how to translate REF_RECEIVED messages
			if (m->what != B_REFS_RECEIVED)
				continue;
			entry_ref	ref;
			int ir = 0;
			while (m->FindRef("refs", ir, &ref) == B_OK) {
				err = resolve_ref(&ref, &ref);
				// I can't really handle errors here. Just leave
				// existing entry_ref intact.
				if (!err)
					m->ReplaceRef("refs", ir, &ref);
				ir++;
			}
		}
	}

	char resultingAppSig[B_MIME_TYPE_LENGTH];
	entry_ref resultingAppRef;
	bool arg_is_doc = false;
	uint32 appFlags;

	err = resolve_app(in_sig, in_ref, &resultingAppRef, resultingAppSig, &appFlags,
		&arg_is_doc);
	if (err != B_OK)
		return err;

	ASSERT(!arg_is_doc || in_ref);

	bool launching_some_executable = false;
	if (arg_is_doc && *in_ref == resultingAppRef) 
		launching_some_executable = true;
	else if (arg_is_doc) {
		err = resolve_ref(&resultingAppRef, &resultingAppRef);
		if (err != B_OK) {
			syslog(LOG_ERR, "Roster::LaunchApp: Resolving the reference failed (0x%x).\n", err);
			return err;
		}
	}

	bool launching_roster = (strcasecmp(resultingAppSig, ROSTER_MIME_SIG) == 0);

	// can't launch the kernel or the app_server
	if ((strcasecmp(resultingAppSig, KERNEL_MIME_SIG) == 0) ||
		(strcasecmp(resultingAppSig, APP_SERVER_MIME_SIG) == 0)) {
		syslog(LOG_ERR, "Roster::LaunchApp: Attempt to launch the kernel or app_server.\n");
		return B_BAD_VALUE;
	}

	if ((appFlags & B_ARGV_ONLY) && msg_list) {
		syslog(LOG_WARNING, "Roster::LaunchApp: An argv_only app was launched with some initial BMessages.\n");

		cargs = 0;
		int i = 0;
		BMessage *m;
		while ((m = (BMessage *) msg_list->ItemAt(i++)) != 0) {
			// only know how to translate REF_RECEIVED messages
			if (m->what != B_REFS_RECEIVED)
				continue;
			uint32 type;
			int32 crefs;
			if (m->GetInfo("refs", &type, &crefs) == B_OK)
				cargs += crefs;
		}
	}

	if (!(appFlags & B_ARGV_ONLY)) {
		// create the port here
		if (launching_roster)
			app_port = create_port(100, ROSTER_PORT_NAME);
		else
			app_port = create_port(100, "rAppLooperPort");

		if (app_port < 0) {
			syslog(LOG_ERR, "Roster::LaunchApp: %s\n", strerror(err));
			return (status_t)app_port;
		}

		// ??? why are we doing this here. Should wait until after load_image.
		// Stuff the port with any initial msgs.
		int count = msg_list ? msg_list->CountItems() : 0;
		BMessenger dest;
		dest.fPort = app_port;
		for (int i = 0; i < count; i++) {
			BMessage *message = static_cast<BMessage *>(msg_list->ItemAt(i));
			
			// add all entry_refs in all the launch messages as recent documents
			for (int32 index = 0; ;index++) {
				entry_ref ref;
				if (message->FindRef("refs", index, &ref) != B_OK)
					break;
				// add as recent document
				AddToRecentDocuments(&ref, resultingAppSig);
			}
			dest.SendMessage(message);
		}

		if (!args && arg_is_doc) {
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", in_ref);

			// add as recent document
			AddToRecentDocuments(in_ref, resultingAppSig);

			dest.SendMessage(&msg);
		}

		// send the READY_TO_RUN message from here was well to ensure
		// that it is delivered before any other messages.
		BMessage rtr(B_READY_TO_RUN);
		dest.SendMessage(&rtr);
	}

	// the launchimg_some_executable is for handling the launching of a
	// shell script. Don't want to preregister such a beast.

	bool isAlreadyRunning = false;
	uint32 entry_token = 0;
	if (!launching_some_executable) {
		BMessage msg(CMD_LAUNCH_INFO);
		msg.AddString("mime_sig", resultingAppSig);
		msg.AddInt32("flags", appFlags);
		msg.AddInt32("port", app_port);
		msg.AddRef("ref", &resultingAppRef);
		BMessage reply;
		if (fMess.SendMessage(&msg, &reply) == B_OK) {
			reply.FindBool("running", &isAlreadyRunning);
			reply.FindInt32("token", (int32 *) &entry_token);
		}
	}

	if (isAlreadyRunning) {
		err = B_ALREADY_RUNNING;
		if (appFlags & B_ARGV_ONLY)
			goto done;

		team_id team = -1;
		// This will busy wait if necessary to get the team_id.
		if ((appFlags & B_LAUNCH_MASK) == B_EXCLUSIVE_LAUNCH) 
			team = TeamFor(resultingAppSig);
		else 
			team = TeamFor(&resultingAppRef);

		if (app_team) 
			*app_team = team;

		if (team == _find_cur_team_id_()) 
			// the app calling launch should be handling the args
			// don't send it a message... to avoid a recursion
			err = EINVAL;
		else 
			// of course the app might not be running anymore, ???
			send_to_running(team, &resultingAppRef, cargs, args, msg_list,
				arg_is_doc ? in_ref : NULL);

		goto done;
	}

	// app is either a multi-launch or isn't running yet
	argc = cargs;
	argv = build_arg_vector(args, &argc, &resultingAppRef, arg_is_doc ? in_ref : NULL);


	tid = load_image(argc, (const char**) argv, (const char**) environ);

	// load_image might somehow have failed so we need to clean up
	if (tid <= B_OK) {
		if (entry_token > 0)
			RemovePreRegApp(entry_token);

		if (launching_some_executable) {
			err = B_LAUNCH_FAILED_EXECUTABLE;
			syslog(LOG_ERR, "Roster::LaunchApp: Executable file (%s) failed to launch\n", argv[0]);
		} else {
			err = tid;
			syslog(LOG_ERR, "Roster::LaunchApp: loading image failed (0x%x). %s\n", err, strerror(err));
		}
		goto done;
	} else {
		/*
		 We've just loaded an app. We need to tell the roster what thread
		 goes with that port. This must be done here, can't be done by the
		 app that just launched, because there might be multiple instances
		 of that app that are in the same pre_registered state.

		 We assume at this point that we've received a valid entry_token for
		 a new app being launched from AddApplication.
		*/

		thread_info	tinfo;
		get_thread_info(tid, &tinfo);

		if (app_team)
			*app_team = tinfo.team;

		// transfer ownership of the app_port to the application
		if (app_port >= 0)
			set_port_owner(app_port, tinfo.team);

		if (launching_roster)
			rename_thread(tid, ROSTER_THREAD_NAME);

		if (entry_token > 0)
			SetThreadAndTeam(entry_token, tid, tinfo.team);

		resume_thread(tid);		/* start the app */
	}
	err = B_OK;
	// fall through to the clean up code.

done:
	
	if (err != B_OK) {
		if (app_port >= 0)
			delete_port(app_port);
	}

	if (argv) {
		for (int i = 0; argv[i]; i++)
			free(argv[i]);
		free(argv);
	}

	return err;
}

/*---------------------------------------------------------------*/

char **BRoster::build_arg_vector(char **args, int *pargs,
	const entry_ref *app_ref, const entry_ref *doc_ref) const
{
	char	**argv;
	int		argc;
	int		cargs = *pargs;
	int		i;
	BPath	full_path;
	BEntry	entry;

	entry.SetTo(app_ref);
	if (entry.GetPath(&full_path))
		return NULL;

//+	PRINT(("path=%s\n", full_path));

	if (args) {
		argc = cargs + 1;
		if (doc_ref)
			argc++;

		argv = (char **) malloc((argc + 1) * (sizeof(char *)));
		if (!argv)
			return NULL;
		argv[0] = strdup(full_path.Path());
		if (!argv[0])
			return NULL;

		// copy over the args that were passed in
		for (i = 0; i < cargs; i++) {
			argv[i+1] = strdup(args[i]);
			if (!argv[i+1])
				return NULL;
		}

		if (doc_ref) {
			// get the full path of the entry_ref
			entry.SetTo(doc_ref);
			if (!entry.GetPath(&full_path)) {
				argv[i+1] = strdup(full_path.Path());
				if (!argv[i+1])
					return NULL;
				i++;
			} else {
				// apparently the file no longer exists
				argc--;
			}
		}

		argv[i+1] = 0;
	} else {
		argv = (char **) malloc(2 * sizeof(char *));
		argc = 1;
		if (!argv)
			return NULL;
		argv[0] = strdup(full_path.Path());
		if (!argv[0])
			return NULL;
		argv[1] = 0;
	}

	*pargs = argc;
	return argv;
}

/*---------------------------------------------------------------*/

status_t BRoster::send_to_running(team_id team, const entry_ref *app_ref,
	int cargs, char **args, const BList *msg_list, const entry_ref *doc_ref) const
{
	status_t	err;
	BPath		full_path;
	BEntry		entry;
	bool		send_relaunch = true;

//+	PRINT(("send_to_running(c=%d, %x, doc_ref=%x)\n", cargs, args, doc_ref));

	BMessenger	mess((char *) NULL, team, &err);
	if (err != B_OK)
		return err;

	if (args) {
		// Build a B_ARGV_RECEIVED message and send it!
		BMessage	msg(B_ARGV_RECEIVED);
		entry.SetTo(app_ref);
		err = entry.GetPath(&full_path);
		if (err != B_OK)
			return err;
		msg.AddString("argv", full_path.Path());
		for (int i = 0; i < cargs; i++) {
			msg.AddString("argv", args[i]);
		}
		cargs++;
		if (doc_ref) {
			cargs++;
			entry.SetTo(doc_ref);
			if (!entry.GetPath(&full_path))
				msg.AddString("argv", full_path.Path());
		}
		msg.AddInt32("argc", cargs);

		char cwd[PATH_MAX + 1];
		if (getcwd(cwd, PATH_MAX) != NULL)
			msg.AddString("cwd", cwd);

		err = mess.SendMessage(&msg);
		send_relaunch = false;
	} else {

		app_info appInfo;
		err = GetRunningAppInfo(team, &appInfo);
		if (err != B_OK)
			return err;

		if (msg_list) {
			BMessage *message;
			int i = 0;
			while ((message = (BMessage *) msg_list->ItemAt(i++)) != 0) {

				// add all entry_refs in all the launch messages as recent documents
				for (int32 index = 0; ;index++) {
					entry_ref ref;
					if (message->FindRef("refs", index, &ref) != B_OK)
						break;
					// add as recent document
					AddToRecentDocuments(&ref, appInfo.signature);
				}

				send_relaunch = false;
				if ((err = mess.SendMessage(message)) != B_OK)
					break;
			}
		}
		if (!err && doc_ref) {
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", doc_ref);

			// add as recent document
			AddToRecentDocuments(doc_ref, appInfo.signature);

			err = mess.SendMessage(&msg);
			send_relaunch = false;
		}
	}
	if (send_relaunch) {
//+		PRINT(("Sending Relaunched msg (BRsoter::Launch)\n"));
		BMessage	msg(B_SILENT_RELAUNCH);
		err = mess.SendMessage(&msg);
	}

	return err;
}

/*---------------------------------------------------------------*/

void BRoster::SetThread(team_id team, thread_id thread) const
{
	BMessage	msg(CMD_SET_INFO);
	BMessage	reply;
	msg.AddInt32("team", team);
	msg.AddInt32("thread", thread);
	fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("REMOTE: SetThread\n"));
}

/*---------------------------------------------------------------*/

void BRoster::SetThreadAndTeam(uint32 entry_token, thread_id thread,
	team_id team) const
{
	ASSERT(entry_token != 0);
	BMessage	msg(CMD_SET_INFO);
	BMessage	reply;
	msg.AddInt32("token", entry_token);
	msg.AddInt32("team", team);
	msg.AddInt32("thread", thread);
	fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("REMOTE: SetThreadAndTeam\n"));
}

/*---------------------------------------------------------------*/

bool BRoster::IsAppPreRegistered(entry_ref *ref, team_id team,
	app_info *info) const
{
	bool	rval = false;
	status_t	err;
	
	BMessage	msg(CMD_IS_APP_PRE);
	BMessage	reply;
	msg.AddRef("ref", ref);
	msg.AddInt32("team", team);
	err = fMess.SendMessage(&msg, &reply);
	if (err == B_OK) {
		rval = reply.HasString("mime_sig");
		if (rval) {
//+			reply.FindInt32("sig", (long*) &info->signature);
			const char	*str;
			reply.FindString("mime_sig", &str);
			if (str)
				strcpy(info->signature, str);
			reply.FindInt32("thread", &info->thread);
			reply.FindInt32("team", &info->team);
			reply.FindInt32("port", &info->port);
			reply.FindInt32("flags", (int32 *) &info->flags);
			reply.FindRef("ref", &info->ref);
		}
	}
//+	SERIAL_PRINT(("LOCAL: IsAppPreRegistered(team=%d), %d, err=%x\n",
//+		team, rval, err));

	return rval;
}

/*---------------------------------------------------------------*/

void BRoster::CompleteRegistration(team_id team, thread_id thread,
	port_id port) const
{
//+	SERIAL_PRINT(("CompleteRegistration: team=%d, port=%d\n", team, port));

	BMessage	msg(CMD_COMPLETE_REG);
	BMessage	reply;
	msg.AddInt32("team", team);
	msg.AddInt32("thread", thread);
	msg.AddInt32("port", port);
	fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("REMOTE: CompleteReg\n"));
}

/*---------------------------------------------------------------*/

void BRoster::DumpRoster() const
{
	BMessage	msg(CMD_DUMP_ROSTER);
	BMessage	reply;
	fMess.SendMessage(&msg, &reply);
}

/*---------------------------------------------------------------*/

void BRoster::GetAppList(BList* team_list) const
{
	BMessage	msg(CMD_GET_APP_LIST);
	BMessage	reply;
	status_t		error;
	error = fMess.SendMessage(&msg, &reply);
	if (!error && reply.HasData("items", B_RAW_TYPE)) {
		int32	size;
		const team_id	*v;
		reply.FindData("items", B_RAW_TYPE, (const void **)&v, &size);
		int32 c = size / sizeof(team_id *);

		for (int32 index = 0; index < c; index++) {
//+			SERIAL_PRINT(("\tteam[%d] = %d\n", index, *v));
			team_list->AddItem((void *) *v);
			v++;
		}
	}
		
//+	SERIAL_PRINT(("REMOTE: GetAppList (count=%d)\n", team_list->CountItems()));
	return;
}

/*---------------------------------------------------------------*/

void BRoster::GetAppList(const char *mime_sig, BList* team_list) const
{
	BMessage	msg(CMD_GET_APP_LIST);
	BMessage	reply;
	status_t		error;

	msg.AddString("mime_sig", mime_sig);
	error = fMess.SendMessage(&msg, &reply);
	if (!error && reply.HasData("items", B_RAW_TYPE)) {
		int32	size;
		const team_id	*v;
		reply.FindData("items", B_RAW_TYPE, (const void **)&v, &size);
		int32 c = size / sizeof(team_id *);
		for (int32 index = 0; index < c; index++) {
			team_list->AddItem((void *) *v);
			v++;
		}
	}
		
//+	SERIAL_PRINT(("REMOTE: GetAppList (count=%d)\n", team_list->CountItems()));
	return;
}

/*---------------------------------------------------------------*/

status_t BRoster::GetAppInfo(entry_ref *ref, app_info *info) const
{
	status_t		err = B_OK;
	long		result;

	if (!ref)
		return B_BAD_VALUE;

	BMessage	msg(CMD_GET_APP_INFO);
	BMessage	reply;

	msg.AddRef("ref", ref);
	err = fMess.SendMessage(&msg, &reply);
	if (!err) {
		err = reply.FindInt32("error", &result);
		if (!err && (result == B_OK)) {
			const char	*str;
			reply.FindString("mime_sig", &str);
			if (str)
				strcpy(info->signature, str);
//+			reply.FindInt32("sig", (long*) &info->signature);
			reply.FindInt32("thread", &info->thread);
			reply.FindInt32("team", &info->team);
			reply.FindInt32("port", &info->port);
			reply.FindInt32("flags", (long*) &info->flags);
			reply.FindRef("ref", &info->ref);
		} else if (!err) {
			err = result;
		}
	}
//+	SERIAL_PRINT(("REMOTE: GetAppInfo(ref), team=%d, err=%x\n", t, err));

	if (err != B_OK) {
		// for lazy people who don't check errors but might check struct
		// ### this will be unnecessary when we add a constructor for
		// app_info. 
//+		info->signature = 0;
		info->thread = -1;
		info->team = -1;
		info->port = -1;

		// ### the entry ref fields are already set by the entry_ref
		// constructor

		info->flags = 0;
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::GetAppInfo(const char *mime_sig, app_info *info) const
{
	status_t	err = B_OK;
	long		result;

	BMessage	msg(CMD_GET_APP_INFO);
	BMessage	reply;

	if (!mime_sig)
		return B_BAD_VALUE;

	msg.AddString("mime_sig", mime_sig);
	err = fMess.SendMessage(&msg, &reply);
	if (!err) {
		err = reply.FindInt32("error", &result);
		if (!err && (result == B_OK)) {
//+			reply.FindInt32("sig", (long*) &info->signature);
			const char	*str;
			reply.FindString("mime_sig", &str);
			if (str)
				strcpy(info->signature, str);
			reply.FindInt32("thread", &info->thread);
			reply.FindInt32("team", &info->team);
			reply.FindInt32("port", &info->port);
			reply.FindInt32("flags", (long*) &info->flags);
			reply.FindRef("ref", &info->ref);
		} else if (!err) {
			err = result;
		}
	}
//+	SERIAL_PRINT(("REMOTE: GetAppInfo(sig), team=%d, err=%x\n", team, err));
	if (err != B_OK) {
		// for lazy people who don't check errors but might check struct
		// ### get rid of that when we have a constructor for app_info
//+		info->signature = 0;
		info->signature[0] = 0;
		info->thread = -1;
		info->team = -1;
		info->port = -1;

		// ### ref already initialized by its contructor

		info->flags = 0;
	}
	return err;
}

/*---------------------------------------------------------------*/
status_t BRoster::GetRunningAppInfo(team_id team, app_info *info) const
{
	status_t	err = B_OK;
	long	result;

	BMessage	msg(CMD_GET_APP_INFO);
	BMessage	reply;

	msg.AddInt32("team", team);
	err = fMess.SendMessage(&msg, &reply);
	if (!err) {
		err = reply.FindInt32("error", &result);
		if (!err && (result == B_OK)) {
//+			reply.FindInt32("sig", (long*) &info->signature);
			const char	*str;
			reply.FindString("mime_sig", &str);
			if (str)
				strcpy(info->signature, str);
			reply.FindInt32("thread", &info->thread);
			reply.FindInt32("team", &info->team);
			reply.FindInt32("port", &info->port);
			reply.FindInt32("flags", (long*) &info->flags);
			reply.FindRef("ref", &info->ref);
		} else if (!err) {
			err = result;
		}
	}
	if (err != B_OK) {
		// for lazy people who don't check errors but might check struct
		// ### idem
//+		info->signature = 0;
		info->thread = -1;
		info->team = -1;
		info->port = -1;
		// ### idem
		info->flags = 0;
	}
//+	SERIAL_PRINT(("REMOTE: GetRunningAppInfo, team=%d, err=%x\n", team, err));

	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::_GetActiveAppInfo(app_info *info, BMessenger* out_window) const
{
	status_t	err;
	long	result;

	BMessage	msg(CMD_GET_APP_INFO);
	BMessage	reply;

	msg.AddBool("active", true);
	err = fMess.SendMessage(&msg, &reply);
	if (!err) {
		err = reply.FindInt32("error", &result);
		if (!err && (result == B_OK)) {
			if (info) {
				const char	*str;
				reply.FindString("mime_sig", &str);
				if (str)
					strcpy(info->signature, str);
				reply.FindInt32("thread", &info->thread);
				reply.FindInt32("team", &info->team);
				reply.FindInt32("port", &info->port);
				reply.FindInt32("flags", (long*) &info->flags);
				reply.FindRef("ref", &info->ref);
			}
			if (out_window) {
				reply.FindMessenger("window", out_window);
			}
		} else if (!err) {
			err = result;
		}
	}
	if (err != B_OK) {
		// for lazy people who don't check errors but might check struct
		// ### idem
		if (info) {
//+			info->signature = 0;
			info->thread = -1;
			info->team = -1;
			info->port = -1;
			info->flags = 0;
		}
		if (out_window) {
			*out_window = BMessenger();
		}
	}
//+	SERIAL_PRINT(("REMOTE: GetRunningAppInfo, team=%d, err=%x\n", team, err));

	return err;
}

status_t BRoster::GetActiveAppInfo(app_info *info) const
{
	return _GetActiveAppInfo(info, NULL);
}

BMessenger	BRoster::ActiveApp() const
{
	app_info ai;
	if (_GetActiveAppInfo(&ai, NULL) != B_OK) {
		return BMessenger();
	}
	return BMessenger(ai.team, ai.port, NO_TOKEN, false);
}

BMessenger	BRoster::ActiveWindow(bool preferred_target) const
{
	BMessenger window;
	if (_GetActiveAppInfo(NULL, &window) != B_OK) {
		return BMessenger();
	}
	window.fPreferredTarget = preferred_target;
	return window;
}

/*-------------------------------------------------------------*/

status_t	BRoster::ActivateApp(team_id team) const
{
	int32		h, v, h0, v0;
	status_t	err = B_OK;
	if (be_app) {
		_BAppServerLink_ link;
		link.session->swrite_l(GR_ACTIVATE_TEAM);
		link.session->swrite_l(team);
		link.session->flush();
		link.session->sread(sizeof(int32), &h0);
		link.session->sread(sizeof(int32), &v0);
		link.session->sread(sizeof(int32), &h);
		link.session->sread(sizeof(int32), &v);

		if ((h != -10000) && (v != -10000)) {
			int32		i, step;
			float		dh, dv, x, y;
			float		dist;
			bigtime_t	before, after;
		
			if (h0 == -10000) {
				BMessage reply;
				BMessage command(IS_SET_MOUSE_POSITION);
				command.AddPoint(IS_WHERE, BPoint(h, v));
				_control_input_server_(&command, &reply);
			}
			else {	
				dh = (float)(h-h0);
				dv = (float)(v-v0);
				dist = sqrt(dh*dh+dv*dv);
				i = (int32)(dist*10);
				if (dist < 1.0)
					step = 0;
				else
					for (step=2; step<40; step++)
						if ((float)(step*(step+1)*(step-1)) > i)
							break;
				dist = 6.0/(float)(step*(step+1)*(step-1));
				dh *= dist;
				dv *= dist;
				x = (float)h0+0.5;
				y = (float)v0+0.5;
		
				for (i=1; i<step; i++) {
					before = system_time() + 12000LL;
					x += dh*(float)(i*(step-i));
					y += dv*(float)(i*(step-i));
					{
						BMessage reply;
						BMessage command(IS_SET_MOUSE_POSITION);
						command.AddPoint(IS_WHERE, BPoint(floor(x), floor(y)));
						_control_input_server_(&command, &reply);
					}
					after = system_time();
					before -= after;
					if (before > 0)
						snooze(before);
				}
			}
		}
	} else {
		// no be_app so we have to do this the hard way. Tell the roster
		// to activate this app!
		BMessage	msg(CMD_ACTIVATE_APP);
		BMessage	reply;
		msg.AddInt32("team", team);
		err = fMess.SendMessage(&msg, &reply);
		if (!err) {
			reply.FindInt32("error", &err);
		}
	}
	return err;
}

/*---------------------------------------------------------------*/

bool BRoster::UpdateActiveApp(team_id team, const BMessenger& window) const
{
	BMessage	msg(CMD_UPDATE_ACTIVE);
	BMessage	reply;
	msg.AddInt32("team", team);
	if (window.IsValid()) msg.AddMessenger("window", window);
	fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("REMOTE: UpdateActive\n"));
	bool b;
	reply.FindBool("result", &b);
	return b;
}

/*---------------------------------------------------------------*/

team_id BRoster::TeamFor(const char *mime_sig) const
{
	team_id		team = -1;

	BMessage	msg(CMD_TEAM_FOR);
	BMessage	reply;
	status_t	err;

	msg.AddString("mime_sig", mime_sig);
	err = fMess.SendMessage(&msg, &reply);
	if (!err)
		reply.FindInt32("result", &team);
//+	SERIAL_PRINT(("REMOTE: TeamFor=%d\n",
//+		reply ?  reply.FindInt32("result") : -1));
	return team;
}

/*---------------------------------------------------------------*/

team_id BRoster::TeamFor(entry_ref *ref) const
{
	team_id	team = -1;

	BMessage	msg(CMD_TEAM_FOR);
	BMessage	reply;
	msg.AddRef("ref", ref);
	status_t err = fMess.SendMessage(&msg, &reply);
	if (!err)
		reply.FindInt32("result", &team);
//+	SERIAL_PRINT(("REMOTE: TeamFor=%d\n",
//+		reply ?  reply.FindInt32("result") : -1));
	return team;
}

/*---------------------------------------------------------------*/

bool BRoster::IsRunning(const char *mime_sig) const
{
	return(TeamFor(mime_sig) != -1);
}

/*---------------------------------------------------------------*/

bool BRoster::IsRunning(entry_ref *ref) const
{
	return(TeamFor(ref) != -1);
}

/*---------------------------------------------------------------*/

void BRoster::SetAppFlags(team_id team, uint32 flags) const
{
	BMessage	msg(CMD_SET_INFO);
	BMessage	reply;
	msg.AddInt32("team", team);
	msg.AddInt32("flags", flags);

//+	status_t err = 
		fMess.SendMessage(&msg, &reply);

//+	SERIAL_PRINT(("REMOTE: SetAppFlags (err=%x)\n", err));
}

/*---------------------------------------------------------------*/

void BRoster::SetSignature(team_id team, const char *mime_sig) const
{
	BMessage	msg(CMD_SET_INFO);
	BMessage	reply;
	msg.AddInt32("team", team);
	msg.AddString("mime_sig", mime_sig);
//+	status_t err = 
		fMess.SendMessage(&msg, &reply);

//+	SERIAL_PRINT(("REMOTE: SetSignature (err=%x)\n", err));
}

/*---------------------------------------------------------------*/

uint32 BRoster::AddApplication(const char *mime_sig, entry_ref *ref,
	uint32 flags, team_id team, thread_id thread, port_id port, bool full_reg) const
{
	uint32		entry_token = 0xFFFFFFFF;

//+	SERIAL_PRINT(("Dumping (AppApplication)\n"));
//+	DumpRoster();
//+	SERIAL_PRINT(("AddApplication: sig=%s, team=%d, port=%d, full=%d,\n\tdir=%d, vol=%d, flags=0x%x\n",
//+		mime_sig, team, port, full_reg,
//+		ref->directory, ref->device, flags));

	BMessage	msg(CMD_ADD_APP);
	BMessage	reply;
	msg.AddString("mime_sig", mime_sig);
	msg.AddRef("ref", ref);
	msg.AddInt32("flags", flags);
	msg.AddInt32("team", team);
	msg.AddInt32("thread", thread);
	msg.AddInt32("port", port);
	msg.AddBool("full_reg", full_reg);
	long e = fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("AddApp - SendMessage err=%x\n", e));
	if (!e) {
		reply.FindInt32("token", (long*) &entry_token);
//+		SERIAL_PRINT(("\tfrom remote(%.4s) entry_token = %d (has tok=%d)\n",
//+			(char*) &(reply.what), entry_token, reply.HasInt32("token")));
	}

	return entry_token;
}

/*---------------------------------------------------------------*/

void BRoster::RemoveApp(team_id team) const
{
//+	SERIAL_PRINT(("LOCAL: RemoveApp(team id=%d)\n", team));
	
	BMessage	msg(CMD_REMOVE_APP);
	BMessage	reply;

	msg.AddInt32("team", team);
	fMess.SendMessage(&msg, &reply);
}

/*---------------------------------------------------------------*/

void BRoster::RemovePreRegApp(ulong entry_token) const
{
	BMessage	msg(CMD_REMOVE_PRE_REG);
	BMessage	reply;
	msg.AddInt32("token", entry_token);
	fMess.SendMessage(&msg, &reply);
//+	SERIAL_PRINT(("REMOTE: RemovePreRegApp\n"));
}

/*---------------------------------------------------------------*/

status_t BRoster::resolve_app(const char *in_type, const entry_ref *in_ref,
	entry_ref *app_ref, char *app_sig, uint32 *app_flags, bool *was_doc) const
{
	BMimeType	meta;
	BFile		file;
	status_t	err;

	if (in_ref) {
		err = translate_ref(in_ref, &meta, app_ref, &file, app_sig, was_doc);
	} else {
		err = translate_type(in_type, &meta, app_ref, &file, app_sig);
	}

//+	PRINT(("translate_xxx returned: sig=%s, was_doc=%d, ref_name=%s, err=%x\n",
//+		app_sig, *was_doc, app_ref->name ? app_ref->name : "null", err));

	if (err != B_OK)
		return err;

	if (meta.IsValid() && !meta.IsInstalled()) {
		// If the app's meta_mime file doesn't exist nows a good time
		// to recreate it!
		BPath	path;
		BEntry	entry(app_ref);
		if (entry.GetPath(&path) == B_OK) {
//+			PRINT(("Creating meta_mime file for %s\n", meta.Type()));
			create_app_meta_mime(path.Path(), false, false, false);
		}
	}

	/*
	 Let's make sure that the application signature in the entry_ref
	 matches the signature from the meta-mime file.
	 Remember that the meta-mime might not be valid (if we're launching
	 an application that doesn't have a signature).
	*/
	if (*app_sig && meta.IsValid() && meta.Type() &&
		(strcasecmp(meta.Type(), app_sig) != 0)) {
//+			PRINT(("Updating cached ref for %s\n", meta.Type()));
			meta.SetAppHint(NULL);		// meta-mime file had bad cached ref
			err = B_MISMATCHED_VALUES;
			// ???
			// just ignore this case for know. Simply launch the app,
			// whatever it may be.
	}

	BAppFileInfo app_info(&file);

	// not finding the information is NOT an error.
	*app_flags = DEFAULT_APP_FLAGS;
	err = app_info.GetAppFlags(app_flags);
//+	PRINT(("appflags=%x (%x)\n", *app_flags, err));

	return B_OK;
}

/*---------------------------------------------------------------*/

static
bool ok_app_hint(const char *pref_app, BMimeType *app_meta,
	entry_ref *app_ref, BFile *app_file, char *app_sig);

bool ok_app_hint(const char *pref_app, BMimeType *app_meta,
	entry_ref *app_ref, BFile *app_file, char *app_sig)
{
	status_t	err;

	// file has a hardcoded preferred_app & app_hint. So use that.
	if ((err = app_file->SetTo(app_ref, O_RDONLY)) != B_OK) {
		/*
		 Cached app doesn't exist. So ignore the hint and try again. We
		 let the hint in place in case it isn't gone, but perhaps on
		 some removeable media, etc.
		*/
		return false;
	}
	BAppFileInfo	ainfo(app_file);
	err = ainfo.GetSignature(app_sig);		// errors here are OK
	if (!err) {
		if (strcasecmp(app_sig, pref_app) != 0) {
			return false;
		}
		app_meta->SetType(app_sig);
	}
	return true;
}

/*---------------------------------------------------------------*/

status_t BRoster::translate_ref(const entry_ref *in_ref,
	BMimeType *app_meta, entry_ref *app_ref, BFile *app_file,
	char *app_sig,  bool *was_doc) const
{
	/*
	 app_meta		- output
	 app_ref		- output
	 app_file		- output
	 app_sig		- output
	 was_doc		- output
	*/

	status_t	err;
	BEntry		entry;
	bool		has_type;
	bool		has_pref_app;
	bool		has_app_hint;
	char		pref_app[B_MIME_TYPE_LENGTH];
	char		mime_type[B_MIME_TYPE_LENGTH];

	*app_sig = 0;		// This should be the signature as indicated in the
						// executable (entry_ref), not the meta_file signature

	// Don't know if in_ref is a document or application at this point.
	// we'll figure it out a little later.

	err = app_file->SetTo(in_ref, O_RDONLY);
	if (err == ENOENT)
		return err;

	BFile		infile(*app_file);
	BNodeInfo	finfo(&infile);

	has_pref_app = (finfo.GetPreferredApp(pref_app) == B_OK && pref_app[0]);
	has_type = (finfo.GetType(mime_type) == B_OK);
	has_app_hint = (finfo.GetAppHint(app_ref) == B_OK);

	if (!has_type) {
		// no MIME type for this file. Use the sniffer to guess.
		*mime_type = '\0';
		err = sniff_file(in_ref, &finfo, mime_type);
		has_type = (*mime_type != '\0');
	}
//+	PRINT(("GetType, err=%x, type=%s\n", err, mime_type));

	if (err != B_OK)
		return err;

restart:

	if (has_pref_app && has_app_hint) {
		// file has a hardcoded preferred_app & app_hint. So use that.
		if (!ok_app_hint(pref_app, app_meta, app_ref,
			app_file, app_sig)) {
				has_app_hint = false;
				goto restart;
		}
		if ((*app_ref) != (*in_ref)) {
			*was_doc = true;
		}
	} else if (has_pref_app) {
//+		PRINT(("translate_ref: preferred app, type=%s\n", pref_app));
		err = translate_type(pref_app, app_meta, app_ref, app_file,
			app_sig);
		if (err == B_LAUNCH_FAILED_APP_NOT_FOUND)
			err = B_LAUNCH_FAILED_FILES_APP_NOT_FOUND;
		*was_doc = true;
	} else if (strcasecmp(mime_type, B_APP_MIME_TYPE) == 0) {
		// 'in_ref' should be an application.
//+		PRINT(("translate_ref: should have an app\n"));
		BAppFileInfo	ainfo(app_file);
		err = ainfo.GetSignature(app_sig);
		if (!err) {
			app_meta->SetType(app_sig);
		}
		err = B_OK;		// error in GetSignature is acceptable.
		*app_ref = *in_ref;
	} else {
		ASSERT(has_type);
		BEntry	entry(in_ref);
		struct	stat	st;
		mode_t	mode_bits;
		bool	is_executable;
		entry.GetStat(&st);
		mode_bits = st.st_mode;
		is_executable = (mode_bits & S_IXUSR) || (mode_bits & S_IXGRP)
							|| (mode_bits & S_IXOTH);
//+		PRINT(("execute=%d, flags=0x%x\n", is_executable, mode_bits));

		if (is_executable) {
			*was_doc = true;
			*app_ref = *in_ref;
			app_file->SetTo(app_ref, O_RDONLY);
			BAppFileInfo	ainfo(app_file);
			ainfo.GetSignature(app_sig);
		} else {
			// Doesn't look like an app, but it does have a mime type
//+			PRINT(("translate_ref: launch doc, type=%s\n", mime_type));
			err = translate_type(mime_type, app_meta, app_ref, app_file,
				app_sig);
			if (!err) {
				if ((*app_ref) != (*in_ref)) {
					// these ref's aren't equal. This means that in_ref
					// should now be a document and get passed
					// to the app once it is launched.
					*was_doc = true;
				}
			}
		}
	}

	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::translate_type(const char *mime_string, BMimeType *app_meta,
	entry_ref *app_ref, BFile *app_file, char *app_sig) const
{
	/*
	 There are 2 cases:
	 	1) mime_string is an application signature, so we want the app
		2) mime_string is data type, so we want the preferred app for that type
	
	 mime_string	- input
	 app_meta		- output
	 app_ref		- output
	 app_file		- output
	 app_sig		- output
	*/

//+	PRINT(("translate_type(%s)\n", mime_string));

	BMimeType	in_meta;
	status_t	err = B_OK;
	BMessage	apps;
	bool		found_meta = false;
	BMimeType	*app_hint_meta = NULL;
	BMimeType	super;

	in_meta.SetType(mime_string);
	if (!in_meta.IsValid())
		return B_BAD_VALUE;

	if (!in_meta.IsInstalled() &&
		(strncasecmp("application/", mime_string, 12) == 0)) {
		// hope that the mime_string is an app_signature and that we'll
		// be able to find the app via the query code below.
		app_meta->SetType(in_meta.Type());
		found_meta = true;
	} else if (in_meta.GetPreferredApp(app_sig) == B_OK) {
		// Found a preferred app for this mime_type. For app's the
		// preferred app is the app itself so the sig's are equal
		if (strcasecmp(app_sig, mime_string) != 0) {
			// we're dealing with a data type, not app signature
			if (in_meta.GetAppHint(app_ref) == B_OK) {
				// file has a hardcoded preferred_app & app_hint. So use that.
				char	pref_app[B_MIME_TYPE_LENGTH];
				strcpy(pref_app, app_sig);
//+				PRINT(("App Hint (%s)\n", app_ref->name));
				if (ok_app_hint(pref_app, app_meta, app_ref,
					app_file, app_sig)) {
						app_hint_meta = &in_meta;
				}
			}
		}
//+		PRINT(("Found preferred app for %s (app=%s)\n", mime_string, app_sig));
		app_meta->SetType(app_sig);
		found_meta = true;
	} else if ((in_meta.GetSupertype(&super) == B_OK) &&
		(super.GetPreferredApp(app_sig) == B_OK)) {
			// Found a preferred app for the super type of the file. Use that!
			if (super.GetAppHint(app_ref) == B_OK) {
				// file has a hardcoded preferred_app & app_hint. So use that.
				char	pref_app[B_MIME_TYPE_LENGTH];
				strcpy(pref_app, app_sig);
//+				PRINT(("App Hint (%s)\n", app_ref->name));
				if (ok_app_hint(pref_app, app_meta, app_ref,
					app_file, app_sig)) {
						app_hint_meta = &super;
				}
			}
//+			PRINT(("Found preferred app for supertype %s (app=%s)\n",
//+				super.Type(), app_sig));
			app_meta->SetType(app_sig);
			found_meta = true;
	}

	if (!found_meta) {
//+		PRINT(("NO PREFERRED APP(%s)\n", mime_string));
		if (err != B_AMBIGUOUS_APP_LAUNCH)
			err = B_LAUNCH_FAILED_NO_PREFERRED_APP;
		return err;
	}

	if (!app_hint_meta) {
		err = app_meta->GetAppHint(app_ref);
		app_hint_meta = app_meta;
	}

	if (!err) {
		struct stat	st;
//+		PRINT(("AppHint: dev=%d, dir=%Ld, name=%s\n",
//+			app_ref->device, app_ref->directory, app_ref->name));
		BEntry	entry(app_ref);
		err = entry.GetStat(&st);
		if (!err) {
			// check to make sure that we can use the file return by GetAppHint
			bool	in_trash;
			if (!ok_to_use(&entry, app_meta, mime_string, &in_trash)) {
				return in_trash ? B_LAUNCH_FAILED_APP_IN_TRASH :B_LAUNCH_FAILED;
			}
		} else {
			app_hint_meta->SetAppHint(NULL);// meta-mime had a bad cached ref
//+			PRINT(("stale AppHint for sig %s\n", app_meta->Type()));
		}
	}

	if (err != B_OK) {
		// either no app "hint" or app "hint" doesn't point to a file.
		// make a query looking for the application.
		err = _query_for_app_(app_meta, mime_string, app_ref, NULL);
	}

	if (!err) {
		// later on we'll actually check to make sure that the
		// cached ref 'app_ref' is actually the correct app!
		app_file->SetTo(app_ref, O_RDONLY);
		BAppFileInfo	ainfo(app_file);
		ainfo.GetSignature(app_sig);
	}

	return err;
}

/*---------------------------------------------------------------*/
bool		match_versions(BEntry *entry, version_info *vers);
status_t	compare_apps(BEntry *best_entry, const BEntry *challenger);

status_t _query_for_app_(BMimeType *meta, const char *type, entry_ref *app_ref,
	version_info *vinfo)
{
	// either no app "hint" or app "hint" doesn't point to a file.
	// make a query looking for the application.
	// If the vinfo ptr is non-NULL then ensure that the versions match.
	char			pred[PATH_MAX];
	BEntry			entry;
	bool			found_one = false;
	bool			found_one_in_trash = false;
	status_t		err;
	BEntry			best_entry;

	sprintf(pred, "%s = %s", B_APP_SIGNATURE_ATTR, meta->Type());
	TQueryWalker	walker(pred);

//+	PRINT(("Querying for signature (%s), type=%s, vers=%x\n",
//+		meta->Type(), type, vinfo));

	while (walker.GetNextEntry(&entry) == B_OK) {
		bool in_trash = false;
		if (vinfo) {
			// if there is a version ptr then only accept apps that match.
			if (match_versions(&entry, vinfo)) {
				if (ok_to_use(&entry, meta, type, &in_trash)) {
					found_one = true;
					best_entry = entry;
					break;
				}
			}
		} else {
			if (ok_to_use(&entry, meta, type, &in_trash)) {
				if (!found_one) {
					found_one = true;
					best_entry = entry;
				} else {
					// we're already found one app. Now we have to get
					// smart and look at versions/mod dates
					compare_apps(&best_entry, &entry);
				}
			}
		}
		if (in_trash) {
			found_one_in_trash = true;
		}
	}

	if (found_one) {
		err = best_entry.GetRef(app_ref);
//+		PRINT(("Query found an app named %s\n", app_ref->name));
		/*
		 if we're looking for a specific version (vinfo != NULL) then
		 don't bother updating any mime info.
		*/
		if (!vinfo) {
			BFile	fi(&best_entry, O_RDONLY);
			if (fi.InitCheck() == B_OK) {
				BAppFileInfo	ai(&fi);
				if (ai.BNodeInfo::InitCheck() == B_OK) {
					BPath	path;
					best_entry.GetPath(&path);
//+					PRINT(("Forcing update of MetaMime for new app %s\n",
//+						app_ref->name));
					uint32	changes;
					ai.UpdateMetaMime(path.Path(), true, &changes);
				}
			}
		}
	} else {
//+		PRINT(("Query couldn't find app_sig %s\n", meta->Type()));
		err = found_one_in_trash ?
			B_LAUNCH_FAILED_APP_IN_TRASH : B_LAUNCH_FAILED_APP_NOT_FOUND;
		char	desc[256];
		if (meta->GetShortDescription(desc)) {
			app_ref->set_name(desc);
		}
	}

	return err;
}

/*----------------------------------------------------------------*/

bool match_versions(BEntry *entry, version_info *v)
{
	version_info	ev;
	BFile			file(entry, O_RDONLY);
	BAppFileInfo	ainfo(&file);
	bool			mm = false;

	if (ainfo.GetVersionInfo(&ev, B_APP_VERSION_KIND) == B_OK) {
		mm =   ((ev.major == v->major)		&&
				(ev.middle == v->middle)	&&
				(ev.minor == v->minor)		&&
				(ev.variety == v->variety)	&&
				(ev.internal == v->internal));
	}

	return mm;
}

/*---------------------------------------------------------------*/

status_t compare_apps(BEntry *king, const BEntry *challenger)
{
#if xDEBUG
	BPath	p1, p2;
	king->GetPath(&p1);
	challenger->GetPath(&p2);
	PRINT(("compare_apps(%s, %s)\n", p1.Path(), p2.Path()));
#endif

	BFile			king_file(king, O_RDONLY);
	BFile			chal_file(challenger, O_RDONLY);

	if (king_file.InitCheck() != B_OK) {
		goto challenger_won;
	}
	if (chal_file.InitCheck() != B_OK)
		goto king_won;

	{
	BAppFileInfo	king_ainfo(&king_file);
	BAppFileInfo	chal_ainfo(&chal_file);
	version_info	kvers;
	version_info	cvers;
	bool			k_has_vers;
	bool			c_has_vers;

	k_has_vers = (king_ainfo.GetVersionInfo(&kvers,B_APP_VERSION_KIND) == B_OK);
	c_has_vers = (chal_ainfo.GetVersionInfo(&cvers,B_APP_VERSION_KIND) == B_OK);

	if (k_has_vers && !c_has_vers) {
//+		PRINT(("king has version, but challenger doesn't\n"));
		goto king_won;
	} else if (!k_has_vers && c_has_vers) {
//+		PRINT(("challenger has version, and king doesn't\n"));
		goto challenger_won;
	} else if (k_has_vers && c_has_vers) {
//+		PRINT(("comparing versions\n"));
		if (cvers.major > kvers.major) {
			goto challenger_won;
		} else if (cvers.major == kvers.major) {
			if (cvers.middle > kvers.middle) {
				goto challenger_won;
			} else if (cvers.middle == kvers.middle) {
				if (cvers.minor > kvers.minor) {
					goto challenger_won;
				} else if (cvers.minor == kvers.minor) {
					if (cvers.variety > kvers.variety) {
						goto challenger_won;
					} else if (cvers.variety == kvers.variety) {
						if (cvers.internal > kvers.internal)
							goto challenger_won;
					}
				}
			}

		}
	} else {
		// use modification dates
		time_t	ktime;
		time_t	ctime;

		if (king->GetModificationTime(&ktime) != B_OK)
			goto challenger_won;
		if (challenger->GetModificationTime(&ctime) != B_OK)
			goto king_won;

		if (ctime > ktime)
			goto challenger_won;
	}
	}

king_won:
//+	PRINT(("king won (%s)\n", p1.Path()));
	return B_OK;

challenger_won:
//+	PRINT(("challenger won (%s)\n", p2.Path()));
	*king = *challenger;
	return B_OK;
}

/*---------------------------------------------------------------*/

status_t BRoster::sniff_file(const entry_ref *file, BNodeInfo *finfo,
	char *mime_type) const
{
	status_t	err;
	BPath		path;
	BEntry		entry(file);

	err = entry.GetPath(&path);
//+	PRINT(("No mime type, firing up the sniffer (%s)\n", path.Path()));
	if (err >= 0) {
		update_mime_info(path.Path(), false, true, false);
		err = finfo->GetType(mime_type);
		if (err != B_OK) {
			BMessage	m(CMD_GUESS_MIME_TYPE);
			BMessage	reply;
			m.AddString("path", path.Path());
			if (_send_to_roster_(&m, &reply, true) == B_OK) {
				const char *t = NULL;
				reply.FindString("type", &t);
				if (t) {
					err = B_OK;
//+					PRINT(("discerned type (%s)\n", t));
					strcpy(mime_type, t);
				}
			}
		}
	}
	return err;
}

/*---------------------------------------------------------------*/

bool BRoster::is_wildcard(const char *sig) const
{
	BMimeType	mt(sig);
	BMessage	sup_types;
	int32		count;
	uint32		t;

	/*
	 An app is a wildcard if it support "application/octet-stream"
	*/

	if (mt.GetSupportedTypes(&sup_types) != B_OK)
		return false;
	
	if (sup_types.GetInfo("types", &t, &count) != B_OK)
		return false;

	int32		i = 0;
	const char	*type;
	while (sup_types.FindString("types", i++, &type) == B_OK) {
		if (strcasecmp(type, "application/octet-stream") == 0) {
			return true;
			break;
		}
	}
	return false;
}

/*---------------------------------------------------------------*/

status_t BRoster::get_unique_supporting_app(const BMessage *apps,
	 char *out_sig) const
{
	/*
	 Only return an app if there is only 1 app that supports this type.
	*/

	int32		sub_count = 0;
	int32		super_count = 0;
	status_t	err = B_OK;
	const char	*pref;

	apps->FindInt32("be:sub", &sub_count);
	apps->FindInt32("be:super", &super_count);

//+	PRINT_OBJECT((*apps));
//+	PRINT(("count: sub=%d, super=%d\n", sub_count, super_count));
	
	if ((sub_count == 1) || ((sub_count == 0) && (super_count == 1))) {
		err = apps->FindString("applications", &pref);
		if (err == B_OK)
			strcpy(out_sig, pref);
	} else {
		err = B_AMBIGUOUS_APP_LAUNCH;
	}

	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::get_random_supporting_app(const BMessage *apps,
	 char *out_sig) const
{
	/*
	 This function is used when a given mime_type doesn't have a preferred
	 handler. We then pick any random app that says it supports that
	 type. However, in some cases don't want to select an app that says
	 that it handles _all_ file types, that type of app is called a
	 wildcard app. Think about it... if we didn't avoid wildcards, then
	 dbl clickin on some unknown file would always end up launching some
	 app. That isn't good/right. Don't want an app like FileTypes
	 (a wildcard app) to launch when you receive an enclosure that contains
	 a file whose app you don't have.
	*/

	uint32		type;
	int32		count = 0;
	int32		i = 0;
	status_t	err;
	bool		found_it = false;

	err = apps->GetInfo("applications", &type, &count);
	if (err != B_OK)
		return err;

//+	PRINT_OBJECT((*apps));

	const char *pref;
	while ((err = apps->FindString("applications", i++, &pref)) == B_OK) {
		// want to avoid wildcard apps
		bool	s = is_wildcard(pref);
//+		PRINT(("super_handler(%s) = %d\n", pref, s));
		if (!s) {
			found_it = true;
			break;
		}
	}
	if (found_it) {
		ASSERT(pref);
		strcpy(out_sig, pref);
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::Broadcast(BMessage *msg) const
{
	return Broadcast(msg, be_app_messenger);
}

/*---------------------------------------------------------------*/

status_t BRoster::Broadcast(BMessage *msg, BMessenger reply_to) const
{
	BMessage	m(CMD_BROADCAST);
	m.AddMessage("message", msg);
	m.AddMessenger("reply_to", reply_to);
	return fMess.SendMessage(&m);
}

/*---------------------------------------------------------------*/

status_t BRoster::StopWatching(BMessenger target) const
{
	return _StopWatching(MAIN_MESSENGER, NULL, CMD_MONITOR_APPS, target);
}

/*---------------------------------------------------------------*/

status_t BRoster::_StopWatching(mtarget t, BMessenger *to, uint32 what,
	BMessenger target) const
{
	BMessage	m(what);
	BMessage	reply;
	status_t	err;

	m.AddBool("start", false);
	m.AddMessenger("target", target);
	if (t == MAIN_MESSENGER) {
		err = fMess.SendMessage(&m, &reply);
	} else if (t == MIME_MESSENGER) {
		err = fMimeMess.SendMessage(&m, &reply);
	} else {
		ASSERT(to);
		err = to->SendMessage(&m, &reply);
	}
	if (!err) {
		status_t	e;
		err = reply.FindInt32("error", &e);
		if (!err)
			err = e;
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t BRoster::StartWatching(BMessenger target, uint32  emask) const
{
	return _StartWatching(MAIN_MESSENGER, NULL, CMD_MONITOR_APPS, target,
		emask);
}

/*---------------------------------------------------------------*/

status_t BRoster::_StartWatching(mtarget t, BMessenger *to, uint32 what,
	BMessenger target, uint32  emask) const
{
	BMessage	m(what);
	BMessage	reply;
	status_t	err;

	m.AddBool("start", true);
	m.AddInt32("event_mask", emask);
	m.AddMessenger("target", target);
	if (t == MAIN_MESSENGER) {
		err = fMess.SendMessage(&m, &reply);
	} else if (t == MIME_MESSENGER) {
		err = fMimeMess.SendMessage(&m, &reply);
	} else {
		ASSERT(to);
		err = to->SendMessage(&m, &reply);
	}
	if (!err) {
		status_t	e;
		err = reply.FindInt32("error", &e);
		if (!err)
			err = e;
	}
	return err;
}

/*---------------------------------------------------------------*/

bool _is_valid_roster_mess_(bool mime)
{
	if (mime)
		return be_roster->fMimeMess.IsValid();
	else
		return be_roster->fMess.IsValid();
}

/*---------------------------------------------------------------*/

status_t _send_to_roster_(BMessage *msg, BMessage *reply, bool mime)
{
	if (!mime) {
		if (reply)
			return be_roster->fMess.SendMessage(msg, reply);
		else
			return be_roster->fMess.SendMessage(msg);
	} else {
		if (reply)
			return be_roster->fMimeMess.SendMessage(msg, reply);
		else
			return be_roster->fMimeMess.SendMessage(msg);
	}
}

/*---------------------------------------------------------------*/

bool ok_to_use(BEntry *entry, BMimeType *meta, const char *type, bool *in_trash)
{
	status_t	err;
	BPath		trash;
	struct stat	sinfo;

//+	PRINT(("ok_to_use: entry=%s, meta=%s, type=%s\n",
//+		BPath(entry).Path(), meta->Type(), type));

	*in_trash = false;

	// Don't use an app that is in the Trash.

	err = entry->GetStat(&sinfo);
	if (err != B_OK)
		return false;
	
	BVolume	vol(sinfo.st_dev);
	find_directory(B_TRASH_DIRECTORY, &trash, false, &vol);
	
	BDirectory	dir(trash.Path());
	if (dir.InitCheck() == B_OK) {
		bool contains = dir.Contains(entry);

		if (contains) {
			*in_trash = true;
			return false;
		}
	}

	BFile		file(entry, O_RDONLY);
	BNodeInfo	info(&file);
	char		t[B_MIME_TYPE_LENGTH];

	err = info.GetType(t);
	if (!err && (strcasecmp(t, "application/x-be-resource") == 0)) {
		// ??? Is this string compare really needed? 
		// This is a resource file... can't use it
		return false;
	}

	// Now make sure that this app truely can handle the given type
	if (strcasecmp(meta->Type(), type) == 0) {
		// This case means we're launching an app via it's signature.
		// Every app implicitly supported itself
		return true;
	}
	BAppFileInfo	ainfo(&file);

	// If the app says that it supports the type...
	if (ainfo.IsSupportedType(type))
		return true;

	// Following items are special cases. If user has force an app_signature
	// to be the preferred app for a non-supported file type. For example,
	// the user made StyledEdit the pref app for video/mumble. We should
	// honor that request.

	// If sig is preferred app for the type then it is OK
	BMimeType	mt(type);
	if ((mt.GetPreferredApp(t) == B_OK) && (strcasecmp(t, meta->Type()) == 0)) {
		return true;
	}
	
	// If the sig is the preferred app for the super type then it is OK
	BMimeType	st;
	if ((mt.GetSupertype(&st) == B_OK) && (st.GetPreferredApp(t) == B_OK)
		&& (strcasecmp(t, meta->Type()) == 0)) {
			return true;
	}

	return false;
}

status_t resolve_ref(const entry_ref *ref, entry_ref* result)
{
	BEntry entry;
	status_t err = entry.SetTo(ref, true);
	if (err == B_OK)
		err = entry.GetRef(result);
	if (err != B_OK) {
		*result = *ref;
		return B_LAUNCH_FAILED_NO_RESOLVE_LINK;
	}
	return B_OK;
}

void 
BRoster::GetRecentDocuments(BMessage *refList, int32 maxCount,
	const char *ofType, const char *openedByAppSig) const
{
	const char * tmp[1] = { ofType };
	GetRecentDocuments(refList, maxCount, tmp, ofType ? 1 : 0, openedByAppSig);
}

void 
BRoster::GetRecentDocuments(BMessage *refList, int32 maxCount,
	const char *ofTypeList[], int32 ofTypeListCount,
	const char *openedByAppSig) const
{
	BMessage message(CMD_GET_RECENT_DOCUMENT_LIST);
	BMessage reply;
 
	message.AddInt32("be:count", maxCount);
	for (int32 index = 0; index < ofTypeListCount; index++)
		message.AddString("be:type", ofTypeList[index]);

	if (openedByAppSig)
		message.AddString("be:app_sig", openedByAppSig);

	if (fMess.SendMessage(&message, &reply) == B_OK)
		reply.FindMessage("result", refList);
}


void 
BRoster::GetRecentFolders(BMessage *refList, int32 maxCount,
	const char *openedByAppSig) const
{
	BMessage message(CMD_GET_RECENT_FOLDER_LIST);
	BMessage reply;
 
	message.AddInt32("be:count", maxCount);
	if (openedByAppSig)
		message.AddString("be:app_sig", openedByAppSig);

	if (fMess.SendMessage(&message, &reply) == B_OK)
		reply.FindMessage("result", refList);
}


void 
BRoster::GetRecentApps(BMessage *refList, int32 maxCount) const
{
	BMessage message(CMD_GET_RECENT_APP_LIST);
	BMessage reply;
 
	message.AddInt32("be:count", maxCount);
	status_t error = fMess.SendMessage(&message, &reply);
	if (error == B_OK)
		error = reply.FindMessage("result", refList);
}

void 
BRoster::AddToRecentDocuments(const entry_ref *doc, const char *appSig) const
{
	PRINT(("document opened %s, %s\n", BPath(&BEntry(doc)).Path(), appSig ? appSig : "currentApp"));
	BMessage message(CMD_DOCUMENT_OPENED);
	if (appSig)
		message.AddString("be:app_sig", appSig);
	else {
		app_info info;
		be_app->GetAppInfo(&info);
		message.AddString("be:app_sig", info.signature);
	}
	message.AddRef("refs", doc);
	fMess.SendMessage(&message);
}

void 
BRoster::AddToRecentFolders(const entry_ref *folder, const char *appSig) const
{
	BMessage message(CMD_FOLDER_OPENED);
	if (appSig)
		message.AddString("be:app_sig", appSig);
	else {
		app_info info;
		be_app->GetAppInfo(&info);
		message.AddString("be:app_sig", info.signature);
	}

	message.AddRef("refs", folder);
	fMess.SendMessage(&message);
}

// --------- Deprecated BRoster methods 11/1999 (Maui) ---------

#if __GNUC__ || __MWERKS__
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	Launch__C7BRosterP9entry_refP5BListPl
	#elif __MWERKS__
	Launch__7BRosterCFP9entry_refP5BListPl
	#endif
	(const BRoster* This, entry_ref *ref, BList *message_list, team_id *app_team)
	{
		return This->Launch(ref, message_list, app_team);
	}
	
	_EXPORT status_t
	#if __GNUC__
	Launch__C7BRosterP9entry_refP8BMessagePl
	#elif __MWERKS__
	Launch__7BRosterCFP9entry_refP8BMessagePl
	#endif
	(const BRoster* This, entry_ref *ref, BMessage *initial_msg, team_id *app_team)
	{
		return This->Launch(ref, initial_msg, app_team);
	}
	
	_EXPORT status_t
	#if __GNUC__
	Launch__C7BRosterP9entry_refiPPcPl
	#elif __MWERKS__
	Launch__7BRosterCFP9entry_refiPPcPl
	#endif
	(const BRoster* This, entry_ref *ref, int argc, char **args, team_id *app_team)
	{
		return This->Launch(ref, argc, args, app_team);
	}

}
#endif

