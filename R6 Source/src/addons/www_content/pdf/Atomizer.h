#if !defined(_ATOMIZER_H_)
#define _ATOMIZER_H_

class BAtomizer {
public:
				BAtomizer();
				~BAtomizer();
	const char *Atom(const char *key);			// strdup()'s key if not found
	const char *StaticAtom(const char *key);	// uses key if not found
	bool		IsAtom(const char *key) const;

private:
	typedef struct private_data private_data;
	private_data	*fPrivate;
};

#endif
