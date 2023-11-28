/* Copyright 1994 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

#include "RK.h"

#define	NCHASH		101
#define	hash(x)		((ulong)((x)%NCHASH))

static struct ncache Nchash[NCHASH];
static struct ncache Ncfree;

#define ainserttop(p) { \
(p)->nc_anext = Ncfree.nc_anext; (p)->nc_aprev = &Ncfree; \
Ncfree.nc_anext->nc_aprev = (p); Ncfree.nc_anext = (p); \
}

#define ainsertbottom(p) { \
(p)->nc_anext = &Ncfree; (p)->nc_aprev = Ncfree.nc_aprev; \
Ncfree.nc_aprev->nc_anext = (p); Ncfree.nc_aprev = (p); \
}

#define	aremove(p)	{\
(p)->nc_anext->nc_aprev = (p)->nc_aprev; \
(p)->nc_aprev->nc_anext = (p)->nc_anext; (p)->nc_anext = (p)->nc_aprev = (p);\
}

#define	hremove(p)	{\
(p)->nc_hnext->nc_hprev = (p)->nc_hprev; \
(p)->nc_hprev->nc_hnext = (p)->nc_hnext; (p)->nc_hnext = (p)->nc_hprev = (p);\
}


int
_RkInitializeCache(int size)
{
	struct RkParam     *sx = &SX;
	int                 i;

	sx->maxcache = size;
	if (!(sx->cache = (struct ncache *) calloc((unsigned)size, sizeof(struct ncache))))
		return -1;
	for (i = 0; i < size; i++) {
		sx->cache[i].nc_anext = &sx->cache[i + 1];
		sx->cache[i].nc_aprev = &sx->cache[i - 1];
		sx->cache[i].nc_hnext = sx->cache[i].nc_hprev = &sx->cache[i];
		sx->cache[i].nc_count = 0;
	}

	Ncfree.nc_anext = &sx->cache[0];
	sx->cache[sx->maxcache - 1].nc_anext = &Ncfree;
	Ncfree.nc_aprev = &sx->cache[sx->maxcache - 1];
	sx->cache[0].nc_aprev = &Ncfree;
	for (i = 0; i < NCHASH; i++)
		Nchash[i].nc_hnext = Nchash[i].nc_hprev = &Nchash[i];
	return 0;
}


void
_RkFinalizeCache()
{
	struct RkParam     *sx = &SX;

	if (sx->cache)
		free(sx->cache);
	sx->cache = (struct ncache *) 0;
}

static int
flushCache(struct DM * dm, struct ncache * cache)
{
	if (cache->nc_word) {
		if (dm && (cache->nc_flags & NC_DIRTY)) {
			DST_WRITE(dm, cache);
		}
		cache->nc_flags &= ~NC_DIRTY;
		return 0;
	}
	return -1;
}

static struct ncache *
newCache(struct DM * ndm, uchar *address)
{
	struct ncache      *newc;

	if ((newc = Ncfree.nc_anext) != &Ncfree) {
		flushCache(newc->nc_dic, newc);
		aremove(newc);
		hremove(newc);
		newc->nc_dic = ndm;
		newc->nc_word = (unsigned char *)0;
		newc->nc_flags = 0;
		newc->nc_address = address;
		newc->nc_count = 0;
		return (newc);
	}
	return (struct ncache *) 0;
}

int
_RkRelease()
{
	struct ncache      *newc;

	for (newc = Ncfree.nc_anext; newc != &Ncfree; newc = newc->nc_anext) {
		if (!newc->nc_word || (newc->nc_flags & NC_NHEAP))
			continue;
		flushCache(newc->nc_dic, newc);
		hremove(newc);
		newc->nc_dic = (struct DM *) 0;
		newc->nc_flags = 0;
		newc->nc_word = (unsigned char *)0;
		newc->nc_address = 0;
		newc->nc_count = 0;
		return 1;
	};
	return 0;
}

void
_RkDerefCache(struct ncache * cache)
{
	if (--cache->nc_count) {
		aremove(cache);
		if (cache->nc_flags & NC_ERROR) {
			ainserttop(cache);
		} else {
			ainsertbottom(cache);
		}
	}
}


void
_RkPurgeCache(struct ncache * cache)
{
	hremove(cache);
	aremove(cache);
	ainserttop(cache);
}


void
_RkKillCache(struct DM * dm)
{
	struct ncache      *cache;
	int                 i;

	for (i = 0, cache = SX.cache; i < SX.maxcache; i++, cache++) {
		if (dm == cache->nc_dic) {
			flushCache(dm, cache);
			_RkPurgeCache(cache);
		}
	};
}

struct ncache      *
_RkFindCache(struct DM * dm, uchar *addr)
{
	struct ncache      *head, *cache;

	head = &Nchash[hash((long)addr)];
	for (cache = head->nc_hnext; cache != head; cache = cache->nc_hnext)
		if (cache->nc_dic == dm && cache->nc_address == addr)
			return cache;
	return (struct ncache *) 0;
}


void
_RkRehashCache(struct ncache * cache, uchar *addr)
{
	struct ncache      *head;
	long a = (long)addr;
	
	if ((head = &Nchash[hash(a)]) != &Nchash[hash(((long)cache->nc_address))]) {
		hremove(cache);
		cache->nc_hnext = head->nc_hnext;
		cache->nc_hprev = head;
		head->nc_hnext->nc_hprev = cache;
		head->nc_hnext = cache;
	}
	cache->nc_address = addr;
}

struct ncache      *
_RkReadCache(struct DM * dm, uchar *addr)
{
	struct ncache      *head, *cache;
	ulong a = (ulong)addr;
	head = &Nchash[hash(a)];
	
	for (cache = head->nc_hnext; cache && (cache != head); cache = cache->nc_hnext) {
		if (cache->nc_dic == dm && cache->nc_address == addr) {
			aremove(cache);
			if (cache != head->nc_hnext) {
				hremove(cache);
				cache->nc_hnext = head->nc_hnext;
				cache->nc_hprev = head;
				head->nc_hnext->nc_hprev = cache;
				head->nc_hnext = cache;
			}
		_RkEnrefCache(cache);
			return (cache);
		}
	}

	cache = newCache(dm, addr);
	if (cache) {
		if (DST_READ(dm, cache)) {
			ainserttop(cache);
			return (struct ncache *) 0;
		} else {
			cache->nc_hnext = head->nc_hnext;
			cache->nc_hprev = head;
			head->nc_hnext->nc_hprev = cache;
			head->nc_hnext = cache;
			_RkEnrefCache(cache);
			return (cache);
		}
	} else {
		return (struct ncache *) 0;
	}
}
