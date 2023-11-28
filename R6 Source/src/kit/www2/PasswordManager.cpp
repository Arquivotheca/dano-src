#include <Autolock.h>
#include <www2/PasswordManager.h>
#include <stdio.h>

PasswordManager passwordManager;



PasswordManager::PasswordManager()
{
}

bool PasswordManager::GetPassword(const char *hostname, const char *challenge, BString *outUser, BString *outPassword)
{
	BAutolock _lock(fLock.Lock());
	int slot = FindPassword(hostname, challenge);
	if (slot == -1)
		return false;
		
	SavedPassword &pw = const_cast<SavedPassword &>(fPasswords.ItemAt(slot));
	*outUser = pw.user;
	*outPassword = pw.password;
	return true;
}

void PasswordManager::SetPassword(const char *hostname, const char *challenge, const char *user, const char *password)
{
	BAutolock _lock(fLock.Lock());
	int slot = FindPassword(hostname, challenge);
	if (slot == -1) {
		// add
		SavedPassword pw;
		pw.host = hostname;
		pw.challenge = challenge;
		pw.user = user;
		pw.password = password;
		fPasswords.AddItem(pw);
	} else {
		// update
		SavedPassword &pw = const_cast<SavedPassword &>(fPasswords.ItemAt(slot));
		pw.user = user;
		pw.password = password;
	}
}

int PasswordManager::FindPassword(const char *host, const char *challenge)
{
	for (int i = 0; i < fPasswords.CountItems(); i++) {
		SavedPassword &pw = const_cast<SavedPassword &>(fPasswords.ItemAt(i));
		if (pw.host == host && pw.challenge == challenge)
			return i;
	}

	return -1;
}


PasswordManager::SavedPassword::SavedPassword()
{
}

PasswordManager::SavedPassword::SavedPassword(const SavedPassword &pw)
	:	host(pw.host),
		challenge(pw.challenge),
		user(pw.user),
		password(pw.password)
{
}

PasswordManager::SavedPassword& PasswordManager::SavedPassword::operator=(const SavedPassword &pw)
{
	host = pw.host;
	challenge = pw.challenge;
	user = pw.user;
	password = pw.password;
	return *this;
}



