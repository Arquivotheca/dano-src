#include <stdlib.h>
#include <stdio.h>
#include "FtpConnection.h"

/*
 * Definitions for the TELNET protocol. Snarfed from the BSD source.
 */
static const unsigned int IAC  = 255;
static const unsigned int DONT = 254;
static const unsigned int DO   = 253;
static const unsigned int WONT = 252;
static const unsigned int WILL = 251;
static const unsigned int xEOF = 236;

status_t FtpConnection::Login(const char *username, const char *password,
	int *outcode)
{
	status_t retval;
	int code, codetype;
	BString cmd, replystr;

	FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X about to log in as %s:%s.\n"
		FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), username, password));
	
	// First, if we're already logged in under a different name
	// or password, try to REINitialize:
	if (fLoggedIn &&
		(fUsername != username || fPassword != password)) {
		FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X is already logged in as %s:%s. "
			"Attempting to REIN.\n" FTP_NORMAL_COLOR,
			reinterpret_cast<unsigned int>(this), username, password));
		retval = Reinitialize();
		if (retval <= B_ERROR) {
			FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X failed to send REIN.\n"
				FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this)));
			fLoggedIn = false;
			fUsername.Truncate(0);
			fPassword.Truncate(0);
			return retval;
		}
	}
	
	fLoggedIn = false;
	fUsername.Truncate(0);
	fPassword.Truncate(0);
	retval = B_ERROR;
	//
	// read the welcome message, do the login
	//
	if (GetReply(&replystr, &code, &codetype) >= B_OK) {
		*outcode = code;
		if (code != 421 && codetype != 5) {
			bool anonymous = false;
			cmd.SetTo("USER ");
			BString login(username);
			if (login.Length() == 0) {
				anonymous = true;
				login.SetTo("anonymous");
			}
			FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X logging in as user '%s'\n"
				FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), login.String()));
			cmd += login;
			SendRequest(cmd);

			if (GetReply(&replystr, &code, &codetype) >= B_OK) {
				*outcode = code;
				switch(code)
				{
					case 230:
					case 202:
						retval = B_OK;
						break;

					case 331:  // password needed
						cmd.SetTo("PASS ");
						if (anonymous) {
							cmd += "-anon@anon.com"; // XXX: what should this be?
						} else {
							cmd += password;
						}
						SendRequest(cmd);
						if (GetReply(&replystr, &code, &codetype) >= B_OK) {
							*outcode = code;
							if (codetype == 2) {
								retval	= B_OK;
								FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X: login successful\n"
									FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this)));
							}
#if ENABLE_FTP_TRACE
							else {
								// incorrect user/password combination?
								FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X: Error logging in. Reply outcode = %d.\n"
									FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), code));
							}
#endif // ENABLE_FTP_TRACE
						}
						break;

					default:
						break;
				}
			}
		}
#if ENABLE_FTP_TRACE
		else {
			FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X: reply outcode = %d.\n"
				FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), code));
		}
#endif // ENABLE_FTP_TRACE
	}
#if ENABLE_FTP_TRACE
	else {
		FTP_TRACE((FTP_STATUS_COLOR "FtpConnection %08X could not read welcome message.\n"
			FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this)));
	}
#endif // ENABLE_FTP_TRACE
	
	if (retval >= B_OK) {
		fUsername = username;
		fPassword = password;
		fLoggedIn = true;
	}
	
	return retval;
}

bool FtpConnection::IsLoggedInAs(const char *username, const char *password) const
{
	return (fLoggedIn && fUsername == username && fPassword == password);
}

status_t FtpConnection::Reinitialize()
{
	BString cmd;

	cmd.SetTo("REIN");
	return SendRequest(cmd);
}

status_t FtpConnection::SendRequest(const BString &cmd)
{
	BString cmd_copy = cmd;

	cmd_copy += "\r\n";
	if (Write(cmd_copy.String(), cmd_copy.Length()) >= 0) {
		FTP_TRACE((FTP_REQUEST_COLOR "FtpConnection %08X sent request \"%s\".\n"
			FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), cmd.String()));
		return B_OK;
	} else {
		FTP_TRACE((FTP_REQUEST_COLOR "FtpConnection %08X failed to send request \"%s\".\n"
			FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), cmd.String()));
		return B_ERROR;
	}
}

status_t FtpConnection::GetReply(BString *outstr, int *outcode, int *outcodetype)
{
	status_t rc = B_ERROR;
	BString line, tempstr;

	//
	// comment from the ncftp source:
	//

	/* RFC 959 states that a reply may span multiple lines.  A single
	 * line message would have the 3-digit code <space> then the msg.
	 * A multi-line message would have the code <dash> and the first
	 * line of the msg, then additional lines, until the last line,
	 * which has the code <space> and last line of the msg.
	 *
	 * For example:
	 *	123-First line
	 *	Second line
	 *	234 A line beginning with numbers
	 *	123 The last line
	 */

	if ((rc = GetReplyLine(&line)) >= B_OK) {
		*outstr = line;
		*outstr += '\n';
		tempstr.Append(line, 3);
		*outcode = atoi(tempstr.String());

		if (line[3] == '-') {
			while ((rc = GetReplyLine(&line)) >= B_OK) {
				*outstr += line;
				*outstr += '\n';
				//
				// we're done with nnn when we get to a "nnn blahblahblah"
				//
				if ((line.FindFirst(tempstr) == 0) && line[3] == ' ') {
					break;
				}
			}
		}
	}

	if (rc <= B_ERROR && *outcode != 421)
	{
		*outstr += "Remote host has closed the connection.\n";
		*outcode = 421;
	}

	if (rc >= B_OK && *outcode == 421) {
		rc = B_ERROR;
	}

	*outcodetype = *outcode / 100;

	return rc;
}

status_t FtpConnection::GetReplyLine(BString *line)
{
	status_t rc = B_ERROR;
	bool done = false;
	unsigned char c = '\0';

	line->Truncate(0);

	// EOF is detected by Read() returning zero bytes read.
	// c itself will never have a value of EOF, since
	// we're Read()ing just one byte into it.
	while (!done && (Read(&c, 1) > 0)) {
		if (c == xEOF || c == '\n') {
			rc = B_OK;
			done = true;
		} else {
			if (c == IAC) {
				Read(&c, 1);
				switch(c)
				{
					unsigned char treply[3];
					case WILL:
					case WONT:
						Read(&c, 1);
						treply[0] = IAC;
						treply[1] = DONT;
						treply[2] = c;
						Write(treply, 3);
					break;

					case DO:
					case DONT:
						Read(&c, 1);
						Read(&c, 1);
						treply[0] = IAC;
						treply[1] = WONT;
						treply[2] = c;
						Write(treply, 3);
					break;

					case xEOF:
						rc = B_OK;
						done = true;
					break;

					default:
						*line += static_cast<char>(c);
					break;
				}
			} else {
				//
				// normal char
				//
				if (c != '\r')
					*line += static_cast<char>(c);
			}
		}
	}

	FTP_TRACE((FTP_RESPONSE_COLOR "FtpConnection %08X received reply line \"%s\".\n"
		FTP_NORMAL_COLOR, reinterpret_cast<unsigned int>(this), line->String()));

	return rc;
}
