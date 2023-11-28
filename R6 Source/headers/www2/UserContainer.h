/*
	UserContainer.h
*/
#ifndef USER_CONTAINER_H
#define USER_CONTAINER_H

#include <support2/String.h>

class UserContainer
{
	public:
		UserContainer();
		UserContainer(const UserContainer &inContainer);
		UserContainer(const char *user);
		UserContainer(const char *user, const char *login, const char *password,
		        const char *server, const char *port);
		~UserContainer();

		UserContainer &	operator=(const UserContainer &container);

		void	SetUserName(const char *name);
		void	SetLogin(const char *login);
		void	SetPassword(const char *password);
		void	SetServer(const char *server);
		void	SetPort(int port);

		const char *	GetUserName() const;
		const char *	GetLogin() const;
		const char *	GetPassword() const;
		const char *	GetServer() const;
		int	GetPort() const;

		bool	IsValid();
		void	Clear();
		bool	SetTo(const char *user);
		void	PrintToStream();

	private:
		BString fUserName;
		BString fLogin;
		BString fPassword;
		BString fServer;
		int fPort;
};

#endif
