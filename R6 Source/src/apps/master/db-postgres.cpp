
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C" {
#include <libpq-fe.h>
}
#include <OS.h>

#include "master.h"
#include "versioncache.h"
#include "db.h"
#include "config.h"
#include "util.h"
#include "sqlutil.h"

#define CONNSTRING_LENGTH 256
char pg_connectstring[CONNSTRING_LENGTH];

int read_config()
{
	if (!cfg["pq_hostname"] ||
		!cfg["pq_database"] ||
		!cfg["pq_user"]) return ERR_UNKNOWN;

	pg_connectstring[0] = 0;
	strcat(pg_connectstring, "host=");
	strcat(pg_connectstring, cfg["pq_hostname"]);
	strcat(pg_connectstring, " dbname=");
	strcat(pg_connectstring, cfg["pq_database"]);
	strcat(pg_connectstring, " user=");
	strcat(pg_connectstring, cfg["pq_user"]);
	if (cfg["pq_password"]) {
		strcat(pg_connectstring, " password=");
		strcat(pg_connectstring, cfg["pq_password"]);
	}
	return ERR_OK;
}

PGconn *
pg_connect() {

	if(read_config() != ERR_OK) {
		return NULL;
	}

	output("db", "connectstring: %s\n", pg_connectstring);
	return(PQconnectdb(pg_connectstring));
}

void
pg_disconnect(PGconn *conn) {
	PQfinish(conn);
}

status_t doPQ(PGconn *conn, const char *qstr)
{
	PGresult *result;
	ExecStatusType err;
	
	output("db","sending query: %s", qstr);

	result = PQexec(conn, qstr);
	err = PQresultStatus(result);
	PQclear(result);
	if (err != PGRES_COMMAND_OK) {
		output("db","%s : %s",PQresStatus(err),PQerrorMessage(conn));
		return B_ERROR;
	}

	result = PQexec(conn, "COMMIT");
	err = PQresultStatus(result);
	PQclear(result);
	if (err != PGRES_COMMAND_OK) {
		output("db","%s : %s",PQresStatus(err),PQerrorMessage(conn));
		return B_ERROR;
	}
	
	return B_OK;
}

char ** db_get_device_classes()
{
	PGconn *conn;
	PGresult *res;
	ExecStatusType err;
	char qstr[8196];
	char **results=NULL;
	int tuples;

	conn = pg_connect();
	PQexec(conn, "BEGIN");
	strcpy(qstr, "SELECT ALL deviceclassid FROM t_deviceclass");

	res = PQexec(conn, qstr);
	err = PQresultStatus(res);
	if (err != PGRES_TUPLES_OK) {
		output("db","%s : %s",PQresStatus(err),PQerrorMessage(conn));
		goto theEnd;
	}

	tuples = PQntuples(res);
	if(tuples > 0) {
		results = (char**)malloc(sizeof(char*) * (tuples+1));
		int idF = PQfnumber(res, "deviceclassid");
		for (int32 i=0;i<tuples;i++) {
			results[i] = strdup(PQgetvalue(res,i,idF));
		}
		results[tuples] = NULL;
	}

	PQexec(conn, "END");

	theEnd:

	PQclear(res);
	pg_disconnect(conn);
	return results;
}

int
db_delete_version(const char *versionid)
{
	PGconn *conn;
	char qstr[8196];
	int err;

	conn = pg_connect();
	PQexec(conn, "BEGIN");
	snprintf(qstr,8196,"DELETE FROM t_swversion WHERE swversionid='%s'",versionid);
	err = doPQ(conn,qstr);
	pg_disconnect(conn);
	return err;
}

int
db_get_version(const char *versionid, int fields)
{
	PGconn *conn;
	PGresult *res;
	char qstr[8196];
	char path[MAXPATHLEN];
	int got=0;

	Oid versionarchive_oid;
	Oid fullupdate_oid;
	Oid fullbootstrap_oid;
	Oid deltadata_oid;
	
	conn = pg_connect();
	PQexec(conn, "BEGIN");
	sprintf(qstr,
	"SELECT fullbootstrap, fullupdate, versionarchive, deltadata FROM t_swversion WHERE swversionid='%s'",
	versionid);

	res = PQexec(conn, qstr);

	if(PQntuples(res) == 0) {
		PQclear(res);
		pg_disconnect(conn);
		warning("version '%s' not found",versionid);
		return ERR_NOVERSION;
	}
	
	if(PQntuples(res) > 1) {
		PQclear(res);
		pg_disconnect(conn);
		warning("more than one version '%s' found",versionid);
		return ERR_DUPVERSIONS;
	}

	mkdir(CACHE_PATHNAME,0777);
	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s", versionid);
	mkdir(path,0777);

	fullbootstrap_oid = atol(PQgetvalue(res, 0, PQfnumber(res, "fullbootstrap")));
	if(fullbootstrap_oid && (fields & AR_FULLBOOTSTRAP)) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/bootstrap.js", versionid);
		lo_export(conn, fullbootstrap_oid, path);
		got |= AR_FULLBOOTSTRAP;
	}

	versionarchive_oid = atol(PQgetvalue(res, 0, PQfnumber(res, "versionarchive")));	
	if(versionarchive_oid && (fields & AR_VERSIONARCHIVE)) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/versionarchive.zip", versionid);
		lo_export(conn, versionarchive_oid, path);
		got |= AR_VERSIONARCHIVE;
	}

	fullupdate_oid = atol(PQgetvalue(res, 0, PQfnumber(res, "fullupdate")));
	if(versionarchive_oid && (fields & AR_FULLUPDATE)) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/fullupdate.zip", versionid);
		lo_export(conn, fullupdate_oid, path);
		got |= AR_FULLUPDATE;
	}

	deltadata_oid = atol(PQgetvalue(res, 0, PQfnumber(res, "deltadata")));
	if(deltadata_oid && (fields & AR_DELTADATA)) {
		snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/deltadata.zip", versionid);
		lo_export(conn, deltadata_oid, path);
		got |= AR_DELTADATA;
	}

	PQclear(res);

	PQexec(conn, "END");
	pg_disconnect(conn);
	return got;
}


int
db_put_version(const struct version_info *info)
{
	PGconn *conn;
	char path[MAXPATHLEN];
	char qstr[8196];
	status_t err;
	
	/* @#$% labels destroy my symmetry, man */
	SQLInsert tInsert("t_swversion");
	SQLUpdate tUpdate("t_swversion");
	
	Oid versionarchive_oid=0;
	Oid fullupdate_oid=0;
	Oid fullbootstrap_oid=0;
	Oid deltadata_oid=0;

	conn = pg_connect();
	if(conn == NULL) return ERR_BIND;

	/** BEGIN TRANSACTION **/
	PQexec(conn, "BEGIN");

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/versionarchive.zip", info->versionid);
	output("db","uploading archive: %s", path);
	versionarchive_oid = lo_import(conn, path);
	if (!versionarchive_oid) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/fullupdate.zip", info->versionid);
	output("db","uploading fullupdate file: %s", path);
	fullupdate_oid = lo_import(conn, path);
	if (!fullupdate_oid) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/deltadata.zip", info->versionid);
	output("db","uploading delta file: %s", path);
	deltadata_oid = lo_import(conn, path);
	if (!deltadata_oid) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/bootstrap.js", info->versionid);
	output("db","uploading bootstrap file: %s", path);
	fullbootstrap_oid = lo_import(conn, path);
	if (!fullbootstrap_oid) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	/* prepare to insert the record into the database */
	tInsert.AddValue("swversionid", info->versionid);
	tInsert.AddValue("installby", info->installby);
	tInsert.AddValue("finishtime", info->finish_time);
	tInsert.AddValue("checkintime", info->checkin_time);
	tInsert.AddValue("lastswversionid", info->lastversionid);
	tInsert.AddValue("versionarchive", versionarchive_oid);
	tInsert.AddValue("fullupdate", fullupdate_oid);
	tInsert.AddValue("fullbootstrap", fullbootstrap_oid);
	tInsert.AddValue("deltadata", deltadata_oid);
	tInsert.AddValue("status", "New");

	tInsert.SQLString(qstr, 8196);
	output("db","putting into swversion");
	err = doPQ(conn,qstr);

	if (err != B_OK) {
		
		/* looks like we had an error.  We assume it is because the record */
		/* exists, and then build up a query to update the existing record */
		tUpdate.AddValue("swversionid", info->versionid);
		tUpdate.AddValue("installby", info->installby);
		tUpdate.AddValue("finishtime", info->finish_time);
		tUpdate.AddValue("checkintime", info->checkin_time);
		tUpdate.AddValue("lastswversionid", info->lastversionid);
		tUpdate.AddValue("versionarchive", versionarchive_oid);
		tUpdate.AddValue("fullupdate", fullupdate_oid);
		tUpdate.AddValue("fullbootstrap", fullbootstrap_oid);
		tUpdate.AddValue("deltadata", deltadata_oid);
		tUpdate.AddValue("status", "New");
		
		sprintf(qstr, "swversionid = '%s'", info->versionid);
		tUpdate.SetWhere(qstr);

		tUpdate.SQLString(qstr, 8196);
		err = doPQ(conn,qstr);
	}
		
	theEnd:
	
	if (err) {
		if (versionarchive_oid) lo_unlink(conn,versionarchive_oid);
		if (fullupdate_oid) lo_unlink(conn,fullupdate_oid);
		if (fullbootstrap_oid) lo_unlink(conn,fullbootstrap_oid);
		if (deltadata_oid) lo_unlink(conn,deltadata_oid);
	}

	pg_disconnect(conn);
	return err;
}

int
db_put_upgrade_path(const char *versionID, const char *fromVersionID,int flags)
{
	PGconn *conn;
	char path[MAXPATHLEN];
	char qstr[8196];
	char flagstr[1024];
	status_t err;
	int comma;
	
	Oid deltapackage;
	Oid bootstrapscript;

	SQLInsert tInsert("t_upgrade");

	conn = pg_connect();
	if(conn == NULL) {
		return ERR_BIND;
	}

	output("db","putting into upgrade");

/** BEGIN TRANSACTION **/
	PQexec(conn, "BEGIN");

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/delta/%s/archive.zip", versionID, fromVersionID);
	output("db","uploading delta file: %s", path);
	deltapackage = lo_import(conn, path);
	if (!deltapackage) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	snprintf(path,MAXPATHLEN,CACHE_PATHNAME "/%s/delta/%s/bootstrap.js", versionID, fromVersionID);
	output("db","uploading bootstrap script: %s", path);
	bootstrapscript = lo_import(conn, path);
	if (!bootstrapscript) {
		err = ERR_FILENOTFOUND;
		goto theEnd;
	}

	flagstr[0] = 0;
	comma = 0;
	if (flags & UF_REBOOT) {
		if (comma) strcat(flagstr,".");
		strcat(flagstr,"reboot");
		comma++;
	}
	if (flags & UF_REDIAL) {
		if (comma) strcat(flagstr,".");
		strcat(flagstr,"redial");
		comma++;
	}
		if (flags & UF_RESTART) {
		if (comma) strcat(flagstr,".");
		strcat(flagstr,"restart");
		comma++;
	}
		if (flags & UF_FREEZE) {
		if (comma) strcat(flagstr,".");
		strcat(flagstr,"freeze");
		comma++;
	}
		if (flags & UF_FLUSH) {
		if (comma) strcat(flagstr,".");
		strcat(flagstr,"flush");
		comma++;
	}

	snprintf(qstr,8196,
		"DELETE FROM t_upgrade WHERE ( from_version_id, to_version_id ) = ( '%s', '%s' )",
		fromVersionID,
		versionID
	);
	doPQ(conn,qstr);
	
	
	/*
	snprintf(qstr,8196,
		"INSERT INTO t_upgrade ("
		" from_version_id,"
		" to_version_id,"
		" bootstrap_script,"
		" update_archive,"
		" flags"
		" ) VALUES ("
		" '%s',"
		" '%s',"
		" '%d',"
		" '%d',"
		" '%s'"
		" )",

		fromVersionID,
		versionID,
		bootstrapscript,
		deltapackage,
		flagstr
	);
	*/

	tInsert.AddValue("from_version_id", fromVersionID);
	tInsert.AddValue("to_version_id", versionID);
	tInsert.AddValue("bootstrap_script", bootstrapscript);
	tInsert.AddValue("update_archive", deltapackage);
	tInsert.AddValue("flags", flagstr);
	tInsert.SQLString(qstr, 8196);
	
	err = doPQ(conn,qstr);
	pg_disconnect(conn);

	theEnd:
	
	if (err) {
		if (deltapackage) lo_unlink(conn,deltapackage);
		if (bootstrapscript) lo_unlink(conn,bootstrapscript);
	}

	return err;
}

int
devclass_deploy(const char *devclassid,const char *versionid,const char *imgpath) {
	PGconn *conn;
	char qstr[8196];
	Oid fullimage_oid;

	conn = pg_connect();
	if(conn == NULL) {
		return ERR_BIND;
	}

	PQexec(conn, "BEGIN");

	output("db","uploading recovery image: %s", imgpath);
	fullimage_oid = lo_import(conn, imgpath);
	if (!fullimage_oid) {
		output("db","could not upload recovery image");
		pg_disconnect(conn);
		return ERR_FILENOTFOUND;
	}

	sprintf(qstr,
		"UPDATE t_deviceclass SET fullimage = '%d', swversionid='%s' WHERE deviceclassid='%s'",
		fullimage_oid,
		versionid,
		devclassid);

	status_t err = doPQ(conn,qstr);
	pg_disconnect(conn);
	return err;
}

int
devclass_getimgsize(const char *devclassid) {
	PGconn *conn;
	PGresult *res;
	int size=-1;
	char qstr[8196];

	conn = pg_connect();
	if(conn == NULL) return ERR_BIND;

	sprintf(qstr, "SELECT imagesize FROM t_deviceclass WHERE deviceclassid='%s'", devclassid);
	res = PQexec(conn, qstr);
	if(PQntuples(res) == 0) goto theEnd;
	
	size = atol(PQgetvalue(res, 0, PQfnumber(res, "imagesize")));

	theEnd:
	PQclear(res);
	pg_disconnect(conn);
	return size;
}
