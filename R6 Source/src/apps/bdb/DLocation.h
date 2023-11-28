/*	$Id: DLocation.h,v 1.5 1999/03/05 14:25:15 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#ifndef DLOCATION_H
#define DLOCATION_H

#include <vector>

class DStackFrame;
class DNub;

	// DLocationString is a DWARF-2 style program to calculate the
	// location of a variable. Other symformats will have to create a
	// compatible location program from their own data.
typedef std::vector<char> DLocationString;

enum locType
{
	ltError,
	ltRegister,
	ltMemory
};

	// returns the kind of address encoded in loc
	// if (locType == ltRegister) addr = registernumber;
	// else addr is address in memory
	// ••• WARNING •••
	// base should in fact be the address you can find in DW_AT_frame_base
	// but since it is always frame.GetFP, you can pass in 0 and GetFP will be used.
	// This may result in incorrect behaviour in some future however
	//
locType LocationMachine (const DLocationString& loc, const DStackFrame& frame, ptr_t& addr, ptr_t base = 0);

	// for globals
ptr_t LocationMachine (const DLocationString& loc, DNub& nub, ptr_t base = 0);

#endif
