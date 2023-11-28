
#ifndef sqlutil_h
#define sqlutil_h

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define MAXFIELDS 32

class SQLInsert {

public:
	SQLInsert(const char *table);
	~SQLInsert();

	void AddValue(const char *field, const char *value);
	void AddValue(const char *field, const int value);
	
	int SQLString(char *outstr, int bufsize);
private:
	char *fTable;
	
	char *fFields[MAXFIELDS];
	char *fValues[MAXFIELDS];
	int	fFieldCount;
	
};


class SQLUpdate {

public:
	SQLUpdate(const char *table, const char *where=NULL);
	~SQLUpdate();
	
	void SetWhere(const char *where);
	
	void AddValue(const char *field, const char *value);
	void AddValue(const char *field, const int value);

	int SQLString(char *outstr, int bufsize);
private:
	char *fTable;
	char *fWhere;

	char *fFields[MAXFIELDS];
	char *fValues[MAXFIELDS];
	int	fFieldCount;
	
};

#endif
