#if ! defined __BENAPHORE_H
#define __BENAPHORE_H 1

#include <kernel/OS.h>
#include <support/SupportDefs.h>

class Benaphore
{
public:
	Benaphore(const char *name = "Benaphore")
	{
		atom = 0;
		ben_sem = create_sem(0, name);
	}

	virtual ~Benaphore(void)
	{
		delete_sem(ben_sem);
	}

	bool isValid(void)
	{
		return ben_sem < B_NO_ERROR ? false : true;
	}

	status_t Lock(void)
	{
		status_t	ret = B_OK;
		long		prev = atomic_add(&atom, 1);

		if(prev > 0)
			ret = acquire_sem(ben_sem);

		return ret;
	}

	status_t Unlock(void)
	{
		status_t	ret = B_OK;
		long		prev = atomic_add(&atom, -1);

		if(prev > 1)
			ret = release_sem(ben_sem);

		return ret;
	}

private:
	sem_id ben_sem;
	long   atom;
};

#endif
