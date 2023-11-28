# $Header: u:/rcs/cv/rcs/palette.awk 3.2 1994/05/12 12:48:14 timr Exp $
# $Log: palette.awk $
# Revision 3.2  1994/05/12 12:48:14  timr
# Shift by 6 in the table instead of in the code.
# Revision 3.1  1994/03/03  12:00:30  timr
# Add nRgbLookup.
# 
# Revision 3.0  1993/12/10  06:27:43  timr
# Initial revision, NT C-only version.
# 

#
# Convert the PALETTE.INC file to a PALETTE.H.
#

# NOTE!  This script produces tables which are NOT COMPATIBLE with the
# tables produced by the MASM macros in expand8.asm.

# We produce 5 tables:  YLookup, ULookup, VLookup, stdPAL8 and RgbLookup.

BEGIN	{
    nRgb = 0
    nY = 0
    nU = 0
    nV = 0

    print "typedef struct { unsigned long R; unsigned long L; } QWORD;";
}

# Skip comments and blank lines.

/^;/	{ next }
/^ *$/	{ next }

# Read constants.

$1 == "nRgb"	{ nRgb = $3; next; }
$1 == "nY"	{ nY = $3; next; }
$1 == "nU"	{ nU = $3; next; }
$1 == "nV"	{ nV = $3; next; }


# Read the Ys table and produce YLookup.  Each line is of the form:

#   Ys@l** = $$

# This tells us that the next change to the table entry occurs in entry $$,
# where byte @ gets incremented.

# I assume that $$ is monotonically increasing.

/^Ys/	{
    if ($3 == 256)
        next;
    print "QWORD YLookup [] = {";
    Byte[0] = 0;
    Byte[1] = 0;
    Byte[2] = 0;
    Byte[3] = 0;

    for (i = 0; i < 256; i++)
    {
        if (i == $3)
        {
            Byte [ord (substr ($1,3,1)) - 48] += 64;	# 1 << 6
	    getline;
	}
        printf "    { 0x%04x%04x, 0x%04x%04x },\n", 
	    Byte[2], Byte[0], Byte[3], Byte[1];
    }
    print "};\n"
    # No Next -- Flow into U
}


# Read the Us table and produce ULookup.  Each line is of the form:

#   Us@l** = $$

# This tells us that the next change to the table entry occurs in entry $$,
# where byte @ gets incremented.  Since U is a signed value, entries 128
# through 255 get 

# I assume that $$ is monotonically increasing.

/^Us/	{
    if ($3 == 256)
        next;
    Byte[0] = 0;
    Byte[1] = 0;
    Byte[2] = 0;
    Byte[3] = 0;

    for (i = 0; i < 128; i++)
    {
        if (i == $3)
        {
            Byte [ord (substr ($1,3,1)) - 48] += 8;
	    getline;
	}
        Pending [i] = sprintf ("{ 0x%04x%04x, 0x%04x%04x },", 
	    Byte[0], Byte[3], Byte[1], Byte[2]);
    }
    print "QWORD ULookup [] = {";
    for (; i < 256; i++)
    {
        if (i == $3)
        {
            Byte [ord (substr ($1,3,1)) - 48] += 8;
	    getline;
	}
        printf "    { 0x%04x%04x, 0x%04x%04x },\n", 
	    Byte[0], Byte[3], Byte[1], Byte[2];
    }
    for (i = 0; i < 128; i++)
        print "    " Pending [i];
    print "};\n"
    # No Next -- Flow into V
}

/^Vs/	{
    if ($3 == 256)
        next;
    Byte[0] = 0;
    Byte[1] = 0;
    Byte[2] = 0;
    Byte[3] = 0;

    for (i = 0; i < 128; i++)
    {
        if (i == $3)
        {
            Byte [ord (substr ($1,3,1)) - 48] ++;
	    getline;
	}
        Pending [i] = sprintf ("{ 0x%04x%04x, 0x%04x%04x },", 
	    Byte[3], Byte[1], Byte[2], Byte[0]);
    }
    print "QWORD VLookup [] = {";
    for (; i < 256; i++)
    {
        if (i == $3)
        {
            Byte [ord (substr ($1,3,1)) - 48] ++;
	    getline;
	}
        printf "    { 0x%04x%04x, 0x%04x%04x },\n", 
	    Byte[3], Byte[1], Byte[2], Byte[0];
    }
    for (i = 0; i < 128; i++)
        print "    " Pending [i];
    print "};\n"
    # No Next -- Flow into R
}

/^Rgb/	{
    print "unsigned int numStdPAL8 = " nRgb ";\n"
    print "unsigned long stdPAL8 [" nRgb "] = {";

    while (substr ($1,1,1) == "R")
    {
        print "    0x" substr($3,1,7) ","
	getline;
    }

    print "};\n"
    # flow into Y
}

/^Y[0-9]/	{
    printf "unsigned int nRgbLookup = %d;\n\n", nY * nU * nV;
    printf "unsigned char RgbLookup [%d] = \n{\n", nY * nU * nV;
    for (Y = 0; Y < nY; Y++)
    {
	for (U = 0; U < nU; U++)
	{
	    for (V = 0; V < nV; V++)
	    {
	        # 01abh
		hx = index ("123456789abcdef", substr($3,3,1)) * 16;
		hx += index ("123456789abcdef", substr($3,4,1));
		printf "0x%02x, ", hx+10;
		getline;
	    }
	    print ""
	}
	print ""
    }
    print "};"
}
