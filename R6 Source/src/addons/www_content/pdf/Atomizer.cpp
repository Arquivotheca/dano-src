#include "Atomizer.h"

#include <SupportDefs.h>
#include <algorithm>
#include <string.h>

#if 1
#include <hash_map>
struct str_equal_to {
	bool operator()(const char *a, const char *b) {	return (::strcmp(a, b) == 0); };
};
typedef hash_map<const char *, void *, hash<const char *>, str_equal_to> Map;
#else
#include <map>
struct str_less {
	bool operator()(const char *a, const char *b) {	return (::strcmp(a, b) < 0 ? 1 : 0); };
};
typedef map<const char *, void *> Map;
#endif

struct BAtomizer::private_data {
	Map items;
};

const char *
BAtomizer::Atom(const char *key)
{
	Map::iterator i	= fPrivate->items.find(key);
	if (i != fPrivate->items.end()) return i->first;
	char *newkey = ::strdup(key);
	fPrivate->items.insert(Map::value_type(newkey, newkey));
	return newkey;
}

const char *
BAtomizer::StaticAtom(const char *key)
{
	Map::iterator i	= fPrivate->items.find(key);
	if (i != fPrivate->items.end()) return i->first;
	fPrivate->items.insert(Map::value_type(key, 0));
	return key;
}

bool 
BAtomizer::IsAtom(const char *key) const
{
	Map::iterator i	= fPrivate->items.find(key);
	return i != fPrivate->items.end();
}

BAtomizer::BAtomizer()
{
	fPrivate = new private_data;
}


BAtomizer::~BAtomizer()
{
	vector<void *> nukem(fPrivate->items.size());
	vector<void *>::iterator vi = nukem.begin();
	Map::iterator i = fPrivate->items.begin();
	while (i != fPrivate->items.end())
	{
		*vi = i->second;
		vi++;
		i++;
	}
	delete fPrivate;
	vi = nukem.begin();
	while (vi != nukem.end())
	{
		free(*vi);
		vi++;
	}
}

#undef TEST_ATOMIZER
#if defined(TEST_ATOMIZER)
#include <stdio.h>
static char *names[] = {"one", "two", "three", "four", "five", "six", "one", "four", 0 };

int main() {
	BAtomizer a;
	const char *r;
	for (char **s = names; *s; s++) {
		r = a.Atom(*s);
		fprintf(stdout, "s: %0lx (%s), r: %0lx (%s)\n", *s, *s, r, r);
	}
	fprintf(stdout, "IsAtom(five): %d\n", (int)a.IsAtom("five"));
	fprintf(stdout, "IsAtom(ten): %d\n", (int)a.IsAtom("ten"));
	return 0;
}
#endif
