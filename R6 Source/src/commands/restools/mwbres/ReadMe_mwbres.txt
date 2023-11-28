Readme for the usage of mwbres
970912
Jon Watte & Deepa Ramani

mwbres is a tool that transforms textual representations of resources into actual 
binary resource data in a Be resource file (such as your executable).

The basics of the mwbres language is the resource declaration, which looks like so:

resource('TYPE', id, "name")
{
	data
}

TYPE is the four-character type code of the resource; id is the integer resource ID 
used to identify the particular resource of that type, and name is the name of the 
generated resource. To have no name, use two double-quotes: "".

data is just an enumeration of all data. Data will be padded on four-byte boundaries. 
Data can be C strings, integers, read() clauses or import() clauses:

resource ('DATA', 1, "level 1")
{
	1,
	"level 1",
	read("somefile"),
	import("otherfile.rsrc", 'TYPE', 12)
}

read() reads the contents of a file into the resource; import() imports the data of some 
other resource from some other file into the resource.

You can group data with brackets:

{
	{ 1, "level 1" },
	{ read("somefile"), import("otherfile.rsrc", 'TYPE', 12) }
}

There is one additional directive: the pad() directive, which adds 0 bytes until the 
current resource or group becomes an integral multiple of the size given:

	{ 1, pad(12) }	// this will emit a four-byte integer valued 1
					// and 8 0-bytes for a total of 12 bytes

	{ "some string", pad(64) }	//	this will make the string occuppy 64 bytes

All data is generated as big-endian data. Typically, you can match a struct declaration 
to a resource declaration if your struct only uses the types int, long, unsigned versions 
thereof, and char arrays that have a size which is an integral multiple of four.

For more recent additions to the language syntax, see the file GRAMMAR.txt.
