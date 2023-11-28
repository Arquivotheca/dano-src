
#include <Message.h>
#include <Path.h>
#include <FindDirectory.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "JapaneseCommon.h"
#include "JapaneseControl.h"

/*
	The configuration messages contain 3 int32 values named
	punctuationmode, spacetype and threshold. These values
	correspond to the index of the three popup menus in the
	first tabview of the japanese input method preferences.

	ReadJapaneseSettings() will fill out the message you pass it with
	these fields (read from the configuration file).

	WriteJapaneseSettings() will take a message with these fields and
	write the configuration file accordingly.
*/


void ReadJapaneseSettings(BMessage *message)
{
	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_NO_ERROR)
		return;
	
	settingsPath.Append("japanese_settings");

	FILE *settingsFile = fopen(settingsPath.Path(), "r");
	if (settingsFile == NULL)
		return;

	int		argc = 0;
	char	**argv = NULL;
	char	buf[512] = "";

	while (fgets(buf, sizeof(buf), settingsFile) != NULL) {
		if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r')
			continue;

		int32 i = 0;
		for (i = 0; isspace(buf[i]) && buf[i]; i++)
			;

		if (buf[i] == '\0')
			continue;

		argc = 0;
		argv = build_argv(buf, &argc);
	
		if (argv == NULL)
			continue;

		if (argc < 2)
			continue;

		if (strcmp(argv[0], J_SETTINGS_KUTOUTEN_TYPE) == 0) {
			uint32 punctMode = strtoul(argv[1], NULL, 0);
			punctMode = (punctMode < 0) ? 0 : punctMode;
			punctMode = (punctMode > 3) ? 3 : punctMode;

			message->AddInt32("punctuationmode",punctMode);
		}
		else if (strcmp(argv[0], J_SETTINGS_SPACE_TYPE) == 0) {
			int32 spacetype = strtol(argv[1], NULL, 0) != 0;
			message->AddInt32("spacetype",spacetype);
		}
		else if (strcmp(argv[0], J_SETTINGS_THRESHOLD_VALUE) == 0) {
			int32 threshold = strtol(argv[1], NULL, 0);
			threshold = (threshold < 1) ? 1 : threshold;
			message->AddInt32("threshold",threshold);
		}

		free(argv);
	}

	fclose(settingsFile);
}


void WriteJapaneseSettings(BMessage *message)
{
	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_NO_ERROR)
		return;
	
	settingsPath.Append("japanese_settings");

	FILE *settingsFile = fopen(settingsPath.Path(), "w+");
	if (settingsFile == NULL)
		return;

	/* ===== */
		int32 punctMode;
		if(B_OK!=message->FindInt32("punctuationmode",&punctMode))
			punctMode=0;
		punctMode = (punctMode < 0) ? 0 : punctMode;
		punctMode = (punctMode > 3) ? 3 : punctMode;
		fprintf(settingsFile, J_SETTINGS_KUTOUTEN_TYPE" %ld\n", punctMode); 
		{
			BMessage command;
			command.what = J_CHANGE_KUTOUTEN_TYPE;
			command.AddInt32(J_KUTOUTEN, punctMode);
			SendToJIM(&command);
		}
	/* ===== */
		int32 spacetype;
		if(B_OK!=message->FindInt32("spacetype",&spacetype))
			spacetype=0;
		fprintf(settingsFile, J_SETTINGS_SPACE_TYPE" %ld\n", (spacetype) ? 1 : 0);
		{
			BMessage command;
            command.what = J_CHANGE_SPACE_TYPE;
            command.AddInt32(J_SPACE, (spacetype) ? 1 : 0);
			SendToJIM(&command);
		}
	/* ===== */
		int32 threshold;
		if(B_OK!=message->FindInt32("threshold",&threshold))
			threshold=0;
		threshold = (threshold < 1) ? 1 : threshold;
		fprintf(settingsFile, J_SETTINGS_THRESHOLD_VALUE" %ld\n", threshold);
		{
			BMessage command;
            command.what = J_CHANGE_HENKAN_WINDOW_THRESHOLD;
            command.AddInt32(J_THRESHOLD, threshold);
			SendToJIM(&command);
		}
 
	/* ===== */
	fclose(settingsFile);
}

static void SendToJIM(BMessage *msg)
{
		// get a messenger to the japanese input method
		BMessage 	methodAddress;
		ssize_t		size = 0;
		char		*buf = NULL;
		int32		code = 0;
		status_t	err = B_ERROR;

		port_id dropBox = find_port(J_DROP_BOX_NAME);
		if (dropBox < 0)
			return;
	
		size = port_buffer_size(dropBox);
		buf = (char *)malloc(size);
		code = 0;
	
		read_port(dropBox, &code, buf, size);
		err = methodAddress.Unflatten(buf);
	
		free(buf);
		
		if (err != B_NO_ERROR)
			return;
	
		BMessenger sMethod;
		if (methodAddress.FindMessenger(J_MESSENGER, &sMethod) != B_NO_ERROR)
			return;
	
		sMethod.SendMessage(J_GRABBED_DROP_BOX); // this causes the port to be filled with another messenger

		sMethod.SendMessage(msg);
}


/*
	The AddToDictionary-message contains three fields:
	- a string named yomi
	- a string named hyoki
	- an int32 named hinshi
	These correspond to the values of the 2 textfields
	and the popup menu in the second tabview of the
	japanese input method preferences app.

*/

void AddToJapaneseDictionary(BMessage *message)
{
	const char	*yomiText;
	const char	*hyokiText;
	int32		hinshi;
	if( (message->FindString("yomi",&yomiText)==B_OK) &&
		(message->FindString("hyoki",&hyokiText)==B_OK) &&
		(message->FindInt32("hinshi",&hinshi)==B_OK))
	{
		if ((strlen(yomiText) < 1) || (strlen(hyokiText) < 1))
			return;

		BMessage message(J_ADD_TO_DICTIONARY);
		message.AddString(J_YOMI, yomiText);
		message.AddString(J_HYOKI, hyokiText);
		message.AddInt32(J_HINSHI, hinshi);

		SendToJIM(&message);
	}
}

static char**
build_argv(
	char	*str, 
	int		*argc)
{
	char	**argv = NULL;
	int32	table_size = 16;

	if (argc == NULL)
		return (NULL);

	*argc = 0;
	argv  = (char **)calloc(table_size, sizeof(char *));

	if (argv == NULL)
		return (NULL);
	
	while (*str) {
		// skip intervening white space
		while(*str != '\0' && (*str == ' ' || *str == '\t' || *str == '\n'))
			str++;
		
		if (*str == '\0')
			break;
		
		if (*str == '"') {
			argv[*argc] = ++str;
			while (*str && *str != '"') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		} else if (*str == '\'') {
			argv[*argc] = ++str;
			while (*str && *str != '\'') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		} else {
			argv[*argc] = str;
			while (*str && *str != ' ' && *str != '\t' && *str != '\n') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		}
		
		if (*str != '\0')
			*str++ = '\0';   // chop the string
		
		*argc += 1;
		if (*argc >= table_size-1) {
			table_size *= 2;
			char **nargv = (char **)calloc(table_size, sizeof(char *));
			
			if (nargv == NULL) {   // drats! failure.
				free(argv);
				return (NULL);
			}
			
			memcpy(nargv, argv, (*argc) * sizeof(char *));
			free(argv);
			argv = nargv;
		}
	}
	
	return (argv);
}

