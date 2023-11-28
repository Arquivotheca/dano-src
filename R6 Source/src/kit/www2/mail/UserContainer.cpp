/*
	UserContainer.cpp
*/
#include <Binder.h>
#include "UserContainer.h"
#include <stdio.h>
#include <stdlib.h>

UserContainer::UserContainer()
	:	fUserName(B_EMPTY_STRING),
		fLogin(B_EMPTY_STRING),
		fPassword(B_EMPTY_STRING),
		fServer(B_EMPTY_STRING),
		fPort(0)
{
	// empty body
}

UserContainer::UserContainer(const UserContainer &inContainer)
{
	*this = inContainer;
}

UserContainer::UserContainer(const char *user)
{
	SetTo(user);
}

UserContainer::UserContainer(const char *user, const char *login, const char *password,
								const char *server, const char *port)
	: 	fUserName(user),
		fLogin(login),
		fPassword(password),
		fServer(server),
		fPort(atoi(port))
{
	// empty body
}

UserContainer::~UserContainer()
{
	// nothing to manually deallocate...
}

UserContainer &UserContainer::operator=(const UserContainer &container)
{
	if (&container != this) {
		fUserName = container.fUserName;
		fLogin = container.fLogin;
		fPassword = container.fPassword;
		fServer = container.fServer;
		fPort = container.fPort;
	}
	return *this;
}

void UserContainer::SetUserName(const char *name)
{
	fUserName = name;
}

void UserContainer::SetLogin(const char *login)
{
	fLogin = login;
}

void UserContainer::SetPassword(const char *password)
{
	fPassword = password;
}

void UserContainer::SetServer(const char *server)
{
	fServer = server;
}

void UserContainer::SetPort(int port)
{
	fPort = port;
}

const char *UserContainer::GetUserName() const
{
	return fUserName.String();
}

const char *UserContainer::GetLogin() const
{
	return fLogin.String();
}

const char *UserContainer::GetPassword() const
{
	return fPassword.String();
}

const char *UserContainer::GetServer() const
{
	return fServer.String();
}

int UserContainer::GetPort() const
{
	return fPort;
}


bool UserContainer::IsValid()
{
	if (fLogin == B_EMPTY_STRING)
		return false;
	if (fPassword == B_EMPTY_STRING)
		return false;
	if (fServer == B_EMPTY_STRING)
		return false;
	if (fPort < 1)
		return false;
	return true;
}

void UserContainer::Clear()
{
	fUserName = B_EMPTY_STRING;
	fLogin = B_EMPTY_STRING;
	fPassword = B_EMPTY_STRING;
	fServer = B_EMPTY_STRING;
	fPort = -1;
}

bool UserContainer::SetTo(const char *user)
{
	Clear();
	
	// Pull data from user binder node;
	BinderNode::property node = BinderNode::Root()["user"][user]["email"]["account"];
	fUserName = user;
	fLogin = node["Login"];
	fPassword = node["Password"];
	fServer = node["ImapServer"];
	fPort =int(node["ImapPort"].Number());
	
	return IsValid();
}

void UserContainer::PrintToStream()
{
	printf("\t--- UserContainer:\n");
	printf("\tUsername: '%s'\n", fUserName.String());
	printf("\tLogin: '%s'\n", fLogin.String());
	printf("\tPassword: '%s'\n", fPassword.String());
	printf("\tServer: '%s'\n", fServer.String());
	printf("\tPort: '%d'\n", fPort);
}

// End of UserContainer.cpp
