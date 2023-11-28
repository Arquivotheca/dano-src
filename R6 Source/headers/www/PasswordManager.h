#ifndef _PASSWORD_MANAGER_H
#define _PASSWORD_MANAGER_H

#include <String.h>
#include <Locker.h>
#include "SmartArray.h"

class PasswordManager {
public:
	PasswordManager();
	bool GetPassword(const char *hostname, const char *challenge, BString *outUser, BString *outPassword);
	void SetPassword(const char *hostname, const char *challenge, const char *user, const char *password);
private:
	int FindPassword(const char *host, const char *challenge);

	class SavedPassword {
	public:
		SavedPassword();
		SavedPassword(const SavedPassword &pw);
		SavedPassword& operator=(const SavedPassword &pw);

		BString host;
		BString challenge;
		BString user;
		BString password;
	};

	SmartArray<SavedPassword> fPasswords;
	BLocker fLock;
};

extern PasswordManager passwordManager;

#endif
