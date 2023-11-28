#ifndef _ICONCACHE_H
#define _ICONCACHE_H


struct IconCacheEntry
{
			IconCacheEntry();
			~IconCacheEntry();
	void	SetTo(const char *type, bool isApp);
	
	
	BBitmap *icon;
	char	*mime;
};


class IconCache
{
public:
	IconCache(int size);
	~IconCache();
	
	BBitmap	*Icon(	const char *mimetype,
					const char *signature);
private:
	int				fSize;
	IconCacheEntry	*fList;
};

#endif
