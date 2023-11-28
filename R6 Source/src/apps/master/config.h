
#ifndef CONFIG_H
#define CONFIG_H

#include <Binder.h>

//class config : public BStringMap {
class config {
	public:

	//config();

	const char *operator[](const char *key) const {
		return BinderNode::Root()["application"]["map_master"][key].String().String();

		/*
		BString *s = Find(key);
		if (s) return s->String();
		else return NULL;
		*/

	}
};

extern config cfg;

#endif
