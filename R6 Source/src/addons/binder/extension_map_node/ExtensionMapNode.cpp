#include <Binder.h>

/*=================================================================*/

class ExtensionMapNode : public BinderContainer
{
	public:
									ExtensionMapNode	();
		virtual	get_status_t		ReadProperty		(const char *name,
														 property &prop,
														 const property_list &args = empty_arg_list);
};


/*=================================================================*/

#define kMIME_TYPE		"_mime_type"
#define kNAME			"_name"
#define kEXTENSION		"_extension"

static const char*	properties[] = {kMIME_TYPE,		/* return mime_type */
									kNAME,			/* return file name */
									kEXTENSION};	/* return file extension */


/*=================================================================*/

int32 cistrcmp(const char* str1, const char* str2)
{
	char		c1;
	char		c2;

	while (1)
	{
		c1 = *str1++;
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = *str2++;
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 < c2)
			return -1;
		if (c1 > c2)
			return 1;
		if (!c1)
			break;
	}
	return 0;
}


/*=================================================================*/

ExtensionMapNode::ExtensionMapNode()
 : BinderContainer()
{
	uint32	index = 0;

	while (index < sizeof(properties) / sizeof(properties[0]))
		AddProperty(properties[index++], (double)0, permsWrite);
}


/*-----------------------------------------------------------------*/

get_status_t ExtensionMapNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	bool	handled = true;

	if (args.Count() == 1)
	{
		int32	extension;
		BString	file_name = args[0].String();

		// sniff for extension
		extension = file_name.FindLast(".");
		// BString::FindLast doesn't seem to include the first char.
		if ((extension == B_ERROR) && (file_name.String()[0] == '.'))
			extension = 0;

		if (!strcmp(kMIME_TYPE, name))
		{
			BinderNode::property	map = BinderNode::Root()["service"]["extension_map"];

			// set default value
			prop = property("application/octet-stream");
			if (map->IsValid())
			{
				if (extension != B_ERROR);
				{
					BString					property_name;
					BinderNode::iterator	i = map->Properties();

					while ((property_name = i.Next()).String() != "")
					{
						if ((property_name.String()[0] == '.') &&
							(cistrcmp(property_name.String(), &file_name[extension]) == 0))
						{
							map->GetProperty(property_name.String(), prop);
							break;
						}
					}
				}
			}
		}
		else if (!strcmp(kNAME, name))
		{
			int32	leaf;

			if (extension != B_ERROR)
				file_name.Truncate(extension);
			leaf = file_name.FindLast('/');
			if ((leaf == B_ERROR) && (file_name.String()[0] == '/'))
				leaf = 0;
			if (leaf != B_ERROR)
				prop = property(&file_name.String()[leaf + 1]);
			else
				prop = property(file_name.String());
		}
		else if (!strcmp(kEXTENSION, name))
		{
			if (extension != B_ERROR)
				prop = property(&file_name.String()[extension]);
			else
				prop = property("unknown!");
		}
		else
			handled = false;
	}
	else
		handled = false;

	if (!handled)
		return BinderContainer::ReadProperty(name, prop, args);

	return B_NO_ERROR;
}


/*=================================================================*/

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new ExtensionMapNode();
}
