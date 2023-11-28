#define data 'ABCD'

resource('rsrc', 1, "Hello")
{
	data
}

resource('rsrc', 2, "Import")
{
	import("input.rsrc", 'STR ', 2)
}

resource('abut', __BERES__, "predefined")
{
	__CDATE__
}

resource('ppc!', 5, "from PPC")
{
	import("input.ppc.rsrc", 'abut', 200)
}

