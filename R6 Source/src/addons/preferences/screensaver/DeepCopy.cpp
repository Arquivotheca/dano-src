#include "DeepCopy.h"

#include <Path.h>
#include <File.h>
#include <fs_attr.h>

#include <stdlib.h>

bool DeepCopy(BDirectory &dir, // target dir in here
	const char *targetname,	// target name in here, not null to rename
	entry_ref *ref, // source ref in here
	bool failifexist)	// fail if exist
{
	bool problem = false;
	BPath original;
	BEntry e(ref, true);
	e.GetPath(&original);
	BFile copy;
	BFile input(&e, B_READ_ONLY);

	if(input.InitCheck() == B_OK &&
		dir.CreateFile(targetname ? targetname : original.Leaf(), &copy, failifexist) == B_OK)
	{
		struct stat st;
		void *buf;
		off_t bufsz;
		ssize_t sz;

		// get stats before touching the original
		input.GetStat(&st);

		// copy at most 100k a time
		buf = malloc(bufsz = (st.st_size > 100 * 1024 ? 100 * 1024 : st.st_size));
		if(buf != 0)
		{
			while((sz = input.Read(buf, bufsz)) > 0)
			{
				if(copy.Write(buf, sz) != sz)
				{
					problem = true;
					break;
				}
			}

			free(buf);
	
			if(! problem)
			{
				// all ok, copy attributes
				if(input.RewindAttrs() == B_NO_ERROR)
				{
					char name[B_ATTR_NAME_LENGTH];

					// scan attributes
					while(! problem && input.GetNextAttrName(name) == B_NO_ERROR)
					{
						attr_info info;
						if(input.GetAttrInfo(name, &info) == B_NO_ERROR)
						{
							void *mem = malloc(info.size);
							if(mem != 0)
							{
								// copy attribute
								if(input.ReadAttr(name, info.type, 0, mem, info.size) == info.size)
									copy.WriteAttr(name, info.type, 0, mem, info.size);
								else
									problem = true;
								free(mem);
							}
						}
					}
				}

				if(! problem)
				{
					// copy permissions
					copy.SetOwner(st.st_uid);
					copy.SetGroup(st.st_gid);
					copy.SetPermissions(st.st_mode);

					// copy timestamps
					copy.SetCreationTime(st.st_ctime);
					copy.SetModificationTime(st.st_mtime);
					copy.SetAccessTime(st.st_atime);
				}
			}
		}
		else
			problem = true;

		if(problem)
		{
			copy.Unset();
			BEntry bad(&dir, original.Leaf());
			bad.Remove();
		}
	}
	else
		problem = true;

	return ! problem;
}
