/*	$Id: RButtons.r,v 1.5 1998/11/11 21:11:16 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

Type 'MICN' {
	hexstring;
};

Resource 'MICN' (200, "Pause")
{
	$"ffffffffffffffffffffffffffffffff"
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffff00000000000000ffffffffff" 
	$"ffffffff00111111111115ffffffffff" 
	$"ffffffff00111111111115ffffffffff" 
	$"ffffffff00111111111115ffffffffff" 
	$"ffffffff00111111111115ffffffffff" 
	$"ffffffff00111111111115ffffffffff" 
	$"ffffffff00151515151515ffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
};

Resource 'MICN' (201, "Run")
{
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffff0dffffffffffffffffffff" 
	$"ffffffffff00000dffffffffffffffff" 
	$"ffffffffff001100000dffffffffffff" 
	$"ffffffffff0011111100000dffffffff" 
	$"ffffffffff001111111111000dffffff" 
	$"ffffffffff0011111111113f1cffffff" 
	$"ffffffffff001111113f1cffffffffff" 
	$"ffffffffff00113f1cffffffffffffff" 
	$"ffffffffff3f1cffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff"
};

Resource 'MICN' (202, "Step")
{
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffff000000000000000015ffffff" 
	$"ffffffff1611111111111111ffffffff" 
	$"ffffffffff10111111111117ffffffff" 
	$"ffffffffff161111111111ffffffffff" 
	$"ffffffffffff1011111117ffffffffff" 
	$"ffffffffffff16111111ffffffffffff" 
	$"ffffffffffffff101117ffffffffffff" 
	$"ffffffffffffff1611ffffffffffffff" 
	$"ffffffffffffffff19ffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffff000000000000000000ffffff" 
	$"ffffffff001212121212121212ffffff" 
	$"ffffffffffffffffffffffffffffffff" 
};

Resource 'MICN' (203, "Step Out")
{
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffff000000000000000000ffffffff" 
	$"ffffff0011111111111111113fffffff" 
	$"ffffffff3f3f3f3f3f3f3f3fffffffff" 
	$"ffffffffffff140714ffffffffffffff" 
	$"ffffffffffff071107ffffffffffffff" 
	$"ffffffffff140b110b14ffffffffffff" 
	$"ffffffffff0811111108ffffffffffff" 
	$"ffffffff140b1111110b14ffffffffff" 
	$"ffffffff08111111111108ffffffffff" 
	$"ffffff140b11111111110b14ffffffff" 
	$"ffffff0b111111111111110bffffffff" 
	$"ffffffff3f3f3f3f3f3f3f3fffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
};

Resource 'MICN' (204, "Kill")
{
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffff00ffffffffff00ffffffffff" 
	$"ffffff000d00ffffff000d0dffffffff" 
	$"ffff000d111100ff000d11110d1dffff" 
	$"ffffff00111111000d11110f3fffffff" 
	$"ffffffff001111111111113fffffffff" 
	$"ffffffffff001111110d3fffffffffff" 
	$"ffffffff000d111111110dffffffffff" 
	$"ffffff000d11110b1111110fffffffff" 
	$"ffff000d11110d3f0d1111110fffffff" 
	$"ffffff0011113fff3f0f110f3fffffff" 
	$"ffffffff003fffffff3f0f3fffffffff" 
	$"ffffffff3fffffffffff3fffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
};

Resource 'MICN' (205, "Trace")
{
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffff0dffffffffffffffff0000ffff" 
	$"ffffff00000dffffffffffff0011ffff" 
	$"ffffff001100000dffffffff0011ffff" 
	$"ffffff0011111100000dffff0011ffff" 
	$"ffffff001111111111000dff0011ffff" 
	$"ffffff0011111111113f1cff0011ffff" 
	$"ffffff001111113f1cffffff0011ffff" 
	$"ffffff00113f1cffffffffff0011ffff" 
	$"ffffff3f1cffffffffffffff0011ffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff" 
	$"ffffffffffffffffffffffffffffffff"
};

resource 'MICN' (2000, "Function PopUp") {
	$"1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b"
	$"1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b"
	$"1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b"
	$"1b1b1b1800001b1b1b1b1b1b1b1b1b1b"
	$"1b1b1b00141b1b1b1b1b1b1b1b1b1b1b"
	$"1b1b18001b1b1b1b1b1b1b1b1b1b1b1b"
	$"1b000000000d180a1b1b1b1b1b1b0a18"
	$"1b1b18001b1b0a111b001818001b110a"
	$"1b1b110a1b1b00181b180606181b1800"
	$"1b1b0a111b1b001b1b1b0a0a1b1b1b00"
	$"1b1800181b1b001b1b180606181b1b00"
	$"1b1400181b1b00181b001818001b1800"
	$"1b0d001b1b1b0a111b1b1b1b1b1b110a"
	$"0000181b1b1b180a1b1b1b1b1b1b0a18"
	$"1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b"
	$"1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b1b"
};

Resource 'MICN' (1000, "PC") {
	$"ffffffff0000ffffffffffffffffffff" 
	$"ffffffff002200ffffffffffffffffff" 
	$"0000000000202200ffffffffffffffff" 
	$"00b2b2b2b220202200ffffffffffffff" 
	$"00b22020202020202400ffffffffffff" 
	$"00b223232320202400ffffffffffffff" 
	$"0000000000202400ffffffffffffffff" 
	$"ffffffff002400ffffffffffffffffff" 
	$"ffffffff0000ffffffffffffffffffff" 
};

Resource 'MICN' (1001, "Breakpoint Icon")
{
	$"2a5aff5a2affffff"
	$"5a2a5a2a5affffff"
	$"ff5a2a5affffffff"
	$"5a2a5a2a5affffff"
	$"2a5aff5a2affffff"
};

Resource 'MICN' (1002, "Addr") {
	$"ffffff0000ffffffffffffffffffffff"
	$"ffffff002a00ffffffffffffffffffff"
	$"000000002a2c00ffffffffffffffffff"
	$"002a2a2a2a2c2c00ffffffffffffffff"
	$"002a2c2c2a2c2c2c00ffffffffffffff"
	$"002cebeb2a2ceb00ffffffffffffffff"
	$"000000002aeb00ffffffffffffffffff"
	$"ffffff002a00ffffffffffffffffffff"
	$"ffffff0000ffffffffffffffffffffff"
};

Resource 'MICN' (1003, "Watchpoint Icon")
{
	$"2086ff8620ffffff"
	$"8620862086ffffff"
	$"ff862086ffffffff"
	$"8620862086ffffff"
	$"2086ff8620ffffff"
};

