
#include "db.h"
#include "versioncache.h"

#include <ldap.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <image.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <OS.h>


#define MY_CONFIG_PATH		"/boot/station/config"

struct config {
	char *ldap_hostname;
	char *ldap_passwd;
	char *ldap_bind_dn;
	char *ldap_base_dn;
} cache_cfg;

/******************/
/* functions here */
/******************/

static LDAPMod *
fill_berval_file(const char *name, const char *path) {
	FILE *f;
	struct stat st;
	void *buf;
	struct berval *bv;
	LDAPMod *mod;
	struct berval **bvlist;

	if (stat(path,&st) == 0) {
		if ((f = fopen(path,"rb")) != NULL) {
			buf = malloc(st.st_size);
			if (fread(buf,st.st_size,1,f) == 1) {
				bv = (struct berval *)malloc(sizeof(struct berval));
				bv->bv_len = st.st_size;
				bv->bv_val = (char *)buf;

				bvlist = (struct berval **)calloc(sizeof(struct bercal *),2);
				bvlist[0] = bv;

				mod = (LDAPMod *)malloc(sizeof(LDAPMod));
				mod->mod_op = LDAP_MOD_BVALUES;
				mod->mod_type = strdup(name);
				mod->mod_bvalues = bvlist;
				
				return mod;
			} else {
				/* fread */
				perror(path);
			}
			free(buf);
			fclose(f);
		} else {
			/* fopen */
			perror(path);
		}
	} else {
		/* stat */
		perror(path);
	}
	return NULL;
}

static LDAPMod *
fill_berval(const char *archive, const char *versionid) {
	char path[MAXPATHLEN];
	
	snprintf(path,MAXPATHLEN,MY_CACHE_BASE "/%s/%s.zip",versionid, archive);
	return fill_berval_file(archive, path);
}
/*
	XXX - Only use this to free an LDAPMod filled by fill_berval!
*/
static void
free_berval(LDAPMod *mod)
{
	free(mod->mod_bvalues[0]->bv_val);
	free(mod->mod_bvalues[0]);
	free(mod->mod_bvalues);
	free(mod->mod_type);
	free(mod);
}

static LDAPMod *
fill_strval(const char *name, const char *value)
{
	LDAPMod *mod;
	char **vlist;

	vlist = (char **)calloc(sizeof(char *),2);
	vlist[0] = strdup(value);

	mod = (LDAPMod *)malloc(sizeof(LDAPMod));
	mod->mod_op = 0;
	mod->mod_type = strdup(name);
	mod->mod_values = vlist;

	return mod;
}

/*
	XXX - Only use this to free an LDAPMod filled by fill_strval!
*/
static void
free_strval(LDAPMod *mod)
{
	free(mod->mod_values[0]);
	free(mod->mod_values);
	free(mod);
}

int
write_file(char *path,void *data,unsigned long len)
{
	FILE *f;
	int ok;

	/* printf("write_file: %ld bytes to %s\n",len,path); */

	ok = ERR_OK;
	if ((f = fopen(path,"wb")) != NULL) {
 		if (fwrite(data,len,1,f) != 1) {
			perror(path);
			ok = ERR_UNKNOWN;
		}
		fclose(f);
	} else {
		perror(path);
		ok = ERR_UNKNOWN;
	}

	return ok;
}

int
read_config()
{
	FILE *f;
	char line[1024];
	char name[33];
	char val[1024];

	memset(&cache_cfg,0,sizeof(cache_cfg));
	
	if ((f = fopen(MY_CONFIG_PATH,"r")) != NULL) {
		do {
		fgets(line,1024,f);
		if ((!feof(f)) && (!ferror(f))) {
			name[0] = 0;
			val[0] = 0;
			sscanf(line,"%32s %[^\n]\n",name,val);
		
			if (!strcmp(name,"ldap_hostname")) {
				cache_cfg.ldap_hostname = strdup(val);
				continue;
			}
			
			if (!strcmp(name,"ldap_password")) {
				cache_cfg.ldap_passwd = strdup(val);
				continue;
			}
			
			if (!strcmp(name,"ldap_bind_dn")) {
				cache_cfg.ldap_bind_dn = strdup(val);
				continue;
			}
			
			if (!strcmp(name,"ldap_base_dn")) {
				cache_cfg.ldap_base_dn = strdup(val);
				continue;
			}
		}
	} while ((!feof(f)) && (!ferror(f)));

	return ERR_OK;
	} else {
		perror(MY_CONFIG_PATH);
	return ERR_UNKNOWN;
	}
}

void
free_config()
{
	if (cache_cfg.ldap_hostname) free(cache_cfg.ldap_hostname);
	if (cache_cfg.ldap_passwd) free(cache_cfg.ldap_passwd);
	if (cache_cfg.ldap_bind_dn) free(cache_cfg.ldap_bind_dn);
	if (cache_cfg.ldap_base_dn) free(cache_cfg.ldap_base_dn);
}


int
devclass_deploy(const char *devclassid,const char *versionid,const char *imgpath)
{
	LDAP *ld;
	int ok,err;
	LDAPMod *mods[3];
	char *dn;

	ok = ERR_OK;

	if (read_config() != ERR_OK) {
		return ERR_NOCONFIG;
	}

	ld = ldap_init(cache_cfg.ldap_hostname,LDAP_PORT);
	if (ld == NULL) {
		perror("ldap_init");
		return ERR_UNKNOWN;
	}

	if (ldap_simple_bind_s(ld,cache_cfg.ldap_bind_dn,cache_cfg.ldap_passwd) != LDAP_SUCCESS) {
		ldap_perror(ld,"bind");
		ldap_unbind(ld);
		return ERR_BIND;
	}

	mods[0] = fill_strval("swversionid", versionid);
	mods[1] = fill_berval_file("fullimage",imgpath);
	mods[2] = NULL;

	if ((mods[0] != NULL) && (mods[1] != NULL)) {
		asprintf(&dn,"deviceclassid=%s, %s",devclassid,cache_cfg.ldap_base_dn);
		if ((err = ldap_modify_s(ld,dn,mods)) != LDAP_SUCCESS) {
			ldap_perror(ld,"ldap_modify_s");
			ok = ERR_UNKNOWN;
		}
	} else {
		ok = ERR_UNKNOWN;
	}

	if (mods[0]) free_strval(mods[0]);
	if (mods[1]) free_berval(mods[1]);

	ldap_unbind(ld);
	free_config();
	return ok;
}

int
devclass_getimgsize(const char *devclassid)
{
	LDAP *ld;
	char *filter;
	char *dn;
	LDAPMessage *result,*first_entry;
	char *attrs[2] = { "imagesize", NULL } ;
	char ok;
	int imgsize;
	int n;
	int err;
	char *nm;
	BerElement *ber;
	char **vals;
	char *endptr;

	ok = ERR_INTERNAL;
	imgsize = -1;

	if (read_config() != ERR_OK) {
		return ERR_NOCONFIG;
	}

	ld = ldap_init(cache_cfg.ldap_hostname,LDAP_PORT);
	if (ld == NULL) {
		perror("ldap_init");
		return ERR_UNKNOWN;
	}

	if (ldap_simple_bind_s(ld,cache_cfg.ldap_bind_dn,cache_cfg.ldap_passwd) != LDAP_SUCCESS) {
		ldap_perror(ld,"bind");
		ldap_unbind(ld);
		return ERR_BIND;
	}

	asprintf(&filter,"(&(deviceclassid=%s)(objectclass=deviceclass))",devclassid);
	asprintf(&dn,"deviceclassid=%s, %s",devclassid,cache_cfg.ldap_base_dn);

	if ((err = ldap_search_s(ld,
 				cache_cfg.ldap_base_dn,	/* it resides here */
				LDAP_SCOPE_ONELEVEL,	/* we know where it is */
				filter,					/* we're interested in only this record */
				attrs,					/* return these attributes only */
				0,						/* return data, too, please. */
				&result)) == LDAP_SUCCESS) {

		if ((n = ldap_count_entries(ld,result)) == 1) {
			if ((first_entry = ldap_first_entry(ld,result)) != NULL) {
				if ((nm = ldap_first_attribute(ld,first_entry,&ber)) != NULL) {
					ok = ERR_OK;
					if (!strcasecmp(nm,"imagesize")) {
						vals = ldap_get_values(ld,first_entry,nm);
						if (vals != NULL) {
							if (vals[0] != NULL) {
								imgsize = strtol(vals[0],&endptr,10);
								if ((vals[0][0] != '\0') && (*endptr == '\0')) {
									ok = ERR_OK;
								} else {
									/* vals[0][0] ... */
									imgsize = -1;
									ok = ERR_INVALID;
								}
							}
							ldap_value_free(vals);
						}
						/*
							In the Netscape LDAP SDK, we're responsible for freeing the string passed back.
							If we suddenly need to do that with OpenLDAP, this is the spot to do it.
						*/
						/* ldap_memfree(nm); */
					} else {
						/* strcasecmp */
						ok = ERR_EMPTY;
					}

					/*
						Even though "LDAP Programming Directory-Enabled Application with Lightweight Directory Access Protocol"
						(MacMillan Technology Series) says we should free this ourselves, this appears to be incorrect, because
						the library does it itself upon dlap_unbind().
					*/
					/*
					if (ber != NULL) {
						ber_free(ber,0);
					}
					*/
				} else {
					/* ldap_first_attribute */
					ok = ERR_EMPTY;
				}
			} else {
				/* ldap_first_entry */
				ok = ERR_UNKNOWN;
			}
		} else {
			/* ldap_count_entries */
			ok = ERR_UNKNOWN;
			switch (n) {
			case -1:	ok = ERR_UNKNOWN;
						ldap_perror(ld,"ldap_count_entries");
						break;

			case 0:		ok = ERR_NOVERSION;
						break;

			default:	ok = ERR_DUPVERSIONS;
						break;
			}
		}
	} else {
 		/* ldap_search_s */
		switch (err) {
		case LDAP_NO_SUCH_OBJECT:	ok = ERR_NOVERSION;
									break;

		default: 					ldap_perror(ld,"ldap_search_s");
									ok = ERR_UNKNOWN;
									break;
		}
	}

	free(filter);
	free(dn);
	ldap_unbind(ld);
	free_config();

	if (ok != ERR_OK) {
		return ok;
	}
	return imgsize;
}



static int
put_via_ldap(LDAP *ld,const struct version_info *info)
{
	int i,ok,err;
	LDAPMod *mods[14];
	char *dn;

	ok = ERR_OK;

	mods[0]  = fill_strval("objectclass","swversion");
	mods[1]  = fill_strval("swversionid",info->versionid);
	mods[2]  = fill_strval("installby",info->installby);
	mods[3]  = fill_strval("checkouttime",info->checkout_time);
	mods[4]  = fill_strval("finishtime",info->finish_time);
	mods[5]  = fill_strval("reboot",info->reboot);
	mods[6]  = fill_strval("recall",info->recall);
	mods[7]  = fill_strval("restart",info->restart);
	mods[8]  = fill_strval("lastswversionid",info->lastversionid);

	mods[9]  = fill_berval("versionarchive", info->versionid);
	mods[10] = fill_berval("fullupdate", info->versionid);
	mods[11] = fill_berval("deltaupdate", info->versionid);
	mods[12] = fill_berval("deltadata", info->versionid);

	mods[13] = NULL;

	if ((mods[2] != NULL) && (mods[3] != NULL) && (mods[4] != NULL) && (mods[5] != NULL)) {
		asprintf(&dn,"swversion=%s, %s",info->versionid,cache_cfg.ldap_base_dn);
		if ((err = ldap_add_s(ld,dn,mods)) != LDAP_SUCCESS) {
			ldap_perror(ld,"ldap_add_s");
			ok = ERR_UNKNOWN;
		}
	} else {
		ok = ERR_UNKNOWN;
	}

	for (i = 0;i <  9;i++) if (mods[i]) free_strval(mods[i]);
	for (i = 9;i < 13;i++) if (mods[i]) free_berval(mods[i]);

	return ok;
}

static int
get_via_ldap(LDAP *ld,const char *versionid,int fields)
{
	char path[MAXPATHLEN];
	char *filter;
	char *dn;
	LDAPMessage *result,*first_entry;
	char *attrs[5];
	char ok,dowrite;
	int n,i;
	int got;
	int err;
	char *nm;
	BerElement *ber;
	struct berval **berval;

	asprintf(&filter,"(&(swversionid=%s)(objectclass=swversion))",versionid);
	asprintf(&dn,"swversion=%s, %s",versionid,cache_cfg.ldap_base_dn);
	ok = ERR_INTERNAL;

	i = 0;
	if (fields & AR_FULLUPDATE) attrs[i++] = "fullupdate";
	if (fields & AR_DELTAUPDATE) attrs[i++] = "deltaupdate";
	if (fields & AR_VERSIONARCHIVE) attrs[i++] = "versionarchive";
	if (fields & AR_DELTADATA) attrs[i++] = "deltadata";
	attrs[i] = NULL;

	if ((err = ldap_search_s(ld,
 				cache_cfg.ldap_base_dn,	/* it resides here */
				LDAP_SCOPE_ONELEVEL,	/* we know where it is */
				filter,					/* we're interested in only this record */
				attrs,					/* return these attributes only */
				0,						/* return data, too, please. */
				&result)) == LDAP_SUCCESS) {

		if ((n = ldap_count_entries(ld,result)) == 1) {
			if ((first_entry = ldap_first_entry(ld,result)) != NULL) {
				/*
					At this point, we assume that if we got the attribute, we want it. Even
					if we got more info that we wanted, there's no point in wasting the bandwidth
					by throwing away the extra fields.
				*/
				if ((nm = ldap_first_attribute(ld,first_entry,&ber)) != NULL) {
					got = 0;
					ok = ERR_OK;
					do {
						berval = ldap_get_values_len(ld,first_entry,nm);
						if (berval != NULL) {
							if (berval[0] != NULL) {
								dowrite = 0;
								if (!strcasecmp(nm,"fullupdate")) {
									dowrite = 1;
									got |= AR_FULLUPDATE;
								}

								if (!strcasecmp(nm,"deltaupdate")) {
									dowrite = 1;
									got |= AR_DELTAUPDATE;
								}

								if (!strcasecmp(nm,"versionarchive")) {
									dowrite = 1;
									got |= AR_VERSIONARCHIVE;
								}

								if (!strcasecmp(nm,"deltadata")) {
									dowrite = 1;
									got |= AR_DELTADATA;
								}

								if (dowrite) {
									snprintf(path,MAXPATHLEN,MY_CACHE_BASE "/%s",versionid);
									if (touchdir(path) == 0) {
										snprintf(path,MAXPATHLEN,MY_CACHE_BASE "/%s/%s.zip",versionid,nm);
										if (write_file(path,berval[0]->bv_val,berval[0]->bv_len) != 0) {
											ok = ERR_UNKNOWN;
										}
									} else {
										/* touchdir */
										ok = ERR_UNKNOWN;
									}
								}
							}
							ldap_value_free_len(berval);
						}
						/*
							In the Netscape LDAP SDK, we're responsible for freeing the string passed back.
							If we suddenly need to do that with OpenLDAP, this is the spot to do it.
						*/
						/* ldap_memfree(nm); */
					} while ((nm = ldap_next_attribute(ld,first_entry,ber)) != NULL);

					/*
						Even though "LDAP Programming Directory-Enabled Application with Lightweight Directory Access Protocol"
						(MacMillan Technology Series) says we should free this ourselves, this appears to be incorrect, because
						the library does it itself upon dlap_unbind().
					*/
					/*
					if (ber != NULL) {
						ber_free(ber,0);
					}
					*/

					/* what we were able to get if there weren't any errors. */
					if (ok >= ERR_OK) {
						ok = got;
					}
				} else {
					/* ldap_first_attribute */

					/* return that we got nothing */
					ok = 0;
				}
			} else {
				/* ldap_first_entry */
				ok = ERR_UNKNOWN;
			}
		} else {
			/* ldap_count_entries */
			ok = ERR_UNKNOWN;
			switch (n) {
			case -1:	ok = ERR_UNKNOWN;
						ldap_perror(ld,"ldap_count_entries");
						break;

			case 0:		ok = ERR_NOVERSION;
						break;

			default:	ok = ERR_DUPVERSIONS;
						break;
			}
		}
	} else {
 		/* ldap_search_s */
		switch (err) {
		case LDAP_NO_SUCH_OBJECT:	ok = ERR_NOVERSION;
									break;

		default: 					ldap_perror(ld,"ldap_search_s");
									ok = ERR_UNKNOWN;
									break;
		}
	}

	free(filter);
	free(dn);
	return ok;
}



int
db_get_version(const char *versionid, int fields) {
	/*
		Now download from the LDAP server.
	*/
	LDAP *ld;
	int f;
	
	printf("db_get_version: %s  fields: 0x%x\n", versionid, fields);
	
	ld = ldap_init(cache_cfg.ldap_hostname,LDAP_PORT);
	if (ld == NULL) {
		perror("ldap_init");
		return ERR_UNKNOWN;
	}

	if (ldap_simple_bind_s(ld,cache_cfg.ldap_bind_dn,cache_cfg.ldap_passwd) != LDAP_SUCCESS) {
		ldap_perror(ld,"bind");
		ldap_unbind(ld);
		return ERR_BIND;
	}

	f = get_via_ldap(ld, versionid, fields);
	ldap_unbind(ld);

	/*
		Return everything we know is in place that we were asked to grab.
		(unless there was an error)
	*/
//	if (f < 0) {
		return f;
//	}
//	return got;
}

int
db_put_version(const struct version_info *info) {
	int ok;
	/*
		Now upload to the LDAP server.
	*/
	LDAP *ld;
	ld = ldap_init(cache_cfg.ldap_hostname,LDAP_PORT);
	if (ld == NULL) {
		perror("ldap_init");
		return ERR_UNKNOWN;
	}
	
	if (ldap_simple_bind_s(ld,cache_cfg.ldap_bind_dn,cache_cfg.ldap_passwd) != LDAP_SUCCESS) {
		ldap_perror(ld,"bind");
		ldap_unbind(ld);
		return ERR_BIND;
	}
	
	ok = put_via_ldap(ld,info);
	ldap_unbind(ld);
	return ok;
}

