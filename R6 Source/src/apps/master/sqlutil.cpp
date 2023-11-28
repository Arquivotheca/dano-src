
#include "sqlutil.h"


SQLInsert::SQLInsert(const char *table) :
	fFieldCount(0)
{
	fTable = strdup(table);
}


SQLInsert::~SQLInsert()
{
	free(fTable);
	
	while(fFieldCount--) {
		free(fFields[fFieldCount]);
		free(fValues[fFieldCount]);
	}
}

void 
SQLInsert::AddValue(const char *field, const char *value)
{
	fFields[fFieldCount] = strdup(field);
	fValues[fFieldCount] = strdup(value);
	fFieldCount++;
}

void 
SQLInsert::AddValue(const char *field, const int value)
{
	char buffer[32];

	fFields[fFieldCount] = strdup(field);

	sprintf(buffer, "%d", value);
	fValues[fFieldCount] = strdup(buffer);
	fFieldCount++;
}

int
SQLInsert::SQLString(char *outstr, int /* bufsize*/)
{
	int written;

	written = sprintf(outstr, "INSERT INTO %s (", fTable);
	
	for(int i=0; i < fFieldCount; i++) {
		written += sprintf(outstr + written, "%s", fFields[i]);
		if(i < fFieldCount - 1) {
			written += sprintf(outstr + written, ", ");
		}
	}

	written += sprintf(outstr + written, ") VALUES (");
	for(int i=0; i < fFieldCount; i++) {
		if(strlen(fValues[i])) {
			written += sprintf(outstr + written, "'%s'", fValues[i]);
		} else {
			written += sprintf(outstr + written, "NULL");
		}
		
		if(i < fFieldCount - 1) {
			written += sprintf(outstr + written, ", ");
		}
	}
	
	written += sprintf(outstr + written, ")");
	
	printf("SQLString: %s\n", outstr);

	return written;
}

/****************************************************/
/****************************************************/

SQLUpdate::SQLUpdate(const char *table, const char *where=NULL) :
	fFieldCount(0)
{
	fTable = strdup(table);
	fWhere = strdup(where);
}


SQLUpdate::~SQLUpdate()
{
	free(fTable);
	free(fWhere);
	
	while(fFieldCount--) {
		free(fFields[fFieldCount]);
		free(fValues[fFieldCount]);
	}

}

void 
SQLUpdate::SetWhere(const char *where)
{
	if(fWhere) {
		free(fWhere);
	}
	fWhere = strdup(where);
}

void 
SQLUpdate::AddValue(const char *field, const char *value)
{
	fFields[fFieldCount] = strdup(field);
	fValues[fFieldCount] = strdup(value);
	fFieldCount++;
}

void 
SQLUpdate::AddValue(const char *field, const int value)
{
	char buffer[32];

	fFields[fFieldCount] = strdup(field);

	sprintf(buffer, "%d", value);
	fValues[fFieldCount] = strdup(buffer);
	fFieldCount++;
}

int 
SQLUpdate::SQLString(char *outstr, int /*bufsize*/)
{
	int written;
	
	written = sprintf(outstr, "UPDATE %s SET ", fTable);

	for(int i=0; i < fFieldCount; i++) {
		if(strlen(fValues[i])) {
			written += sprintf(outstr + written, "%s = '%s'", fFields[i], fValues[i]);	
		} else {
			written += sprintf(outstr + written, "%s = NULL", fFields[i]);
		}	

		if(i < fFieldCount - 1) {
			written += sprintf(outstr + written, ", ");
		}
	}
	
	if(fWhere) {
		written += sprintf(outstr + written, " WHERE %s", fWhere);
	}
	return written;
}


