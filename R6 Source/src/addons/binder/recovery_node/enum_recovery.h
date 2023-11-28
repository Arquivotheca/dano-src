/*
 * enum_recovery.hh -- enumerator for zrecover config
 *
 * $au$: manuel<mpetit@be.com> -- Sun Oct 15 16:06:41 PDT 2000
 * $ww$: ts=4
 *
 * $ed$: manuel<mpetit@be.com> -- Sun Oct 15 16:06:41 PDT 2000
 *
 *
 * Copyright 2000, Be Incorporated, All Rights Reserved.
 *
 */
#ifndef __recoveyy__config_enumerator__hh__
#define __recovery__congig_enumerator__hh__


#include <support/SupportDefs.h>


struct zrecover_config_item_t
{
public:
	enum  kind_e {
        ZR_INFO,		// read only
	    ZR_SETTING		// read/write
	};

private:
	char   *key;
	char   *value;
	kind_e  kind;

    zrecover_config_item_t(char const *key, char const *value, unsigned kind);

public:
	char const *Key  (void) const { return key;  };
	char const *Value(void) const { return value; };
	unsigned    Kind (void) const { return kind;  };


	virtual ~zrecover_config_item_t(void);
	
	friend zrecover_config_item_t *new_config_item(char const *key, char const *value, unsigned kind);
};


struct zrecover_config_t
{
private:
    void *hidden;

public:
    zrecover_config_t(void);
	zrecover_config_t(char const *path);

	virtual ~zrecover_config_t(void);

	unsigned                      CountItems(void) const;
	zrecover_config_item_t const *ItemAt(unsigned) const;


    status_t Configure(char const *key, char const *value);
    status_t ZapConfiguration(void);

	status_t Cancel(void);
	status_t Commit(void);
};


#endif
