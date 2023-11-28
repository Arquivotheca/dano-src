#ifndef _PASSWORD_MANAGER_H
#define _PASSWORD_MANAGER_H

#include <support2/String.h>
#include <support2/Locker.h>
#include <support2/Vector.h>

using namespace B::Support2;

class PasswordManager
{
	public:
		PasswordManager();
		bool GetPassword(const char *hostname, const char *challenge, BString *outUser, BString *outPassword);
		void SetPassword(const char *hostname, const char *challenge, const char *user, const char *password);
	private:
		int FindPassword(const char *host, const char *challenge);

		class SavedPassword
		{
			public:
				SavedPassword();
				SavedPassword(const SavedPassword &pw);
				SavedPassword& operator=(const SavedPassword &pw);

				BString host;
				BString challenge;
				BString user;
				BString password;
		};

		BVector<SavedPassword> fPasswords;
		BNestedLocker fLock;
};

extern PasswordManager passwordManager;

#endif
