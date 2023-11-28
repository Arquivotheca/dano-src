//
//	StreamCache tests
//

#include <stdio.h>
#include "../StreamCache.h"

char dummyData[1024];

int testnum = 1;

#define TEST(x) 									\
		printf("Test %d: ", testnum++);				\
		if (!(x)) {									\
			printf("FAIL line %d\n", __LINE__); 	\
			return -1;								\
		} else 										\
			printf("PASS\n");



int main()
{
	// 1. Test permutations for overlap
	//     0    10   20   30   40   50
	//          xxxxx     xxxxx
	//
	//  a. xxxxx						touch beginning of block
	//  b.  xxxxx						overlap beginning of block
	//  c.         xxxxx                overlap end of block
    //  d.        xxxxxxxxxx            overlap two blocks
    //  e.           xxxxx              touch end of one block and beginning of another
	//  f.                xxxxx         same size as block
	//  g.      xxxxx                   same size as block
	//  h.	                 xxx        subset of block
	
	
	ChunkCache cachev[8];
	for (int i = 0; i < 8; i++) {
		cachev[i].WriteAt(10, dummyData, 10);
		cachev[i].WriteAt(30, dummyData, 10);
	}

	TEST(cachev[0].SanityCheck());
	TEST(cachev[0].IsDataAvailable(0, 10) == false);
	TEST(cachev[0].IsDataAvailable(10, 10) == true);
	TEST(cachev[0].IsDataAvailable(20, 20) == false);
	TEST(cachev[0].IsDataAvailable(30, 10) == true);
	TEST(cachev[0].IsDataAvailable(40, 10) == false);
	TEST(cachev[0].IsDataAvailable(5, 10) == false);	// test overlapped checks
	TEST(cachev[0].IsDataAvailable(15, 20) == false);
	TEST(cachev[0].IsDataAvailable(0, 50) == false);
	TEST(cachev[0].IsDataAvailable(9, 2) == false);		// Look for off by one errors
	TEST(cachev[0].IsDataAvailable(20, 1) == false);
	TEST(cachev[0].IsDataAvailable(29, 2) == false);
	TEST(cachev[0].IsDataAvailable(39, 2) == false);
	
	// Test a.
	cachev[0].WriteAt(0, dummyData, 10);
	TEST(cachev[0].SanityCheck());
	TEST(cachev[0].IsDataAvailable(0, 20) == true);
	TEST(cachev[0].IsDataAvailable(20, 10) == false);
	TEST(cachev[0].IsDataAvailable(30, 10) == true);
	TEST(cachev[0].IsDataAvailable(40, 10) == false);
	TEST(cachev[0].IsDataAvailable(5, 20) == false);	// test overlapped checks
	TEST(cachev[0].IsDataAvailable(15, 20) == false);
	TEST(cachev[0].IsDataAvailable(0, 50) == false);
	TEST(cachev[0].IsDataAvailable(20, 1) == false);	// Look for off by one errors
	TEST(cachev[0].IsDataAvailable(19, 2) == false);
	TEST(cachev[0].IsDataAvailable(29, 1) == false);
	TEST(cachev[0].IsDataAvailable(29, 2) == false);

	// Test b.
	cachev[1].WriteAt(5, dummyData, 10);
	TEST(cachev[1].SanityCheck());
	TEST(cachev[1].IsDataAvailable(0, 5) == false);
	TEST(cachev[1].IsDataAvailable(5, 15) == true);
	TEST(cachev[1].IsDataAvailable(20, 10) == false);
	TEST(cachev[1].IsDataAvailable(30, 10) == true);
	TEST(cachev[1].IsDataAvailable(40, 10) == false);
	TEST(cachev[1].IsDataAvailable(0, 6) == false); 	// Look for off by one errors
	TEST(cachev[1].IsDataAvailable(4, 1) == false);
	TEST(cachev[1].IsDataAvailable(4, 2) == false);
	TEST(cachev[1].IsDataAvailable(19, 2) == false);

	// Test c.
	cachev[2].WriteAt(15, dummyData, 10);
	TEST(cachev[2].SanityCheck());
	TEST(cachev[2].IsDataAvailable(0, 10) == false);
	TEST(cachev[2].IsDataAvailable(10, 15) == true);
	TEST(cachev[2].IsDataAvailable(25, 5) == false);
	TEST(cachev[2].IsDataAvailable(30, 10) == true);
	TEST(cachev[2].IsDataAvailable(40, 10) == false);
	TEST(cachev[2].IsDataAvailable(5, 20) == false);	// test overlapped checks
	TEST(cachev[2].IsDataAvailable(0, 50) == false);
	TEST(cachev[2].IsDataAvailable(9, 2) == false); 	// Look for off by one errors
	TEST(cachev[2].IsDataAvailable(25, 2) == false);
	TEST(cachev[2].IsDataAvailable(29, 2) == false);

	// Test d.
	cachev[3].WriteAt(15, dummyData, 20);
	TEST(cachev[3].SanityCheck());
	TEST(cachev[3].IsDataAvailable(0, 10) == false);
	TEST(cachev[3].IsDataAvailable(10, 30) == true);
	TEST(cachev[3].IsDataAvailable(40, 10) == false);
				
	// Test e.
	cachev[4].WriteAt(20, dummyData, 10);
	TEST(cachev[4].SanityCheck());
	TEST(cachev[4].IsDataAvailable(0, 10) == false);
	TEST(cachev[4].IsDataAvailable(10, 30) == true);
	TEST(cachev[4].IsDataAvailable(40, 10) == false);
	
	// Test f.
	cachev[5].WriteAt(30, dummyData, 5);
	TEST(cachev[5].IsDataAvailable(0, 10) == false);
	TEST(cachev[5].IsDataAvailable(10, 10) == true);
	TEST(cachev[5].IsDataAvailable(20, 20) == false);
	TEST(cachev[5].IsDataAvailable(30, 10) == true);
	TEST(cachev[5].IsDataAvailable(40, 10) == false);
	
	// Test g.
	cachev[6].WriteAt(10, dummyData, 10);
	TEST(cachev[6].IsDataAvailable(0, 10) == false);
	TEST(cachev[6].IsDataAvailable(10, 10) == true);
	TEST(cachev[6].IsDataAvailable(20, 20) == false);
	TEST(cachev[6].IsDataAvailable(30, 10) == true);
	TEST(cachev[6].IsDataAvailable(40, 10) == false);
	
	// Test h.
	cachev[7].WriteAt(35, dummyData, 5);
	TEST(cachev[7].IsDataAvailable(0, 10) == false);
	TEST(cachev[7].IsDataAvailable(10, 10) == true);
	TEST(cachev[7].IsDataAvailable(20, 20) == false);
	TEST(cachev[7].IsDataAvailable(30, 10) == true);
	TEST(cachev[7].IsDataAvailable(40, 10) == false);

	// Consume a bunch of blocks
	ChunkCache cacheg;
	for (int i = 0; i < 10; i++)
		cacheg.WriteAt(i * 4, dummyData, 3);

	TEST(cacheg.SanityCheck());
	for (int i = 0; i < 10; i++) {
		TEST(cacheg.IsDataAvailable(i * 4, 1) == true);
		TEST(cacheg.IsDataAvailable(i * 4, 2) == true);
		TEST(cacheg.IsDataAvailable(i * 4, 3) == true);
		TEST(cacheg.IsDataAvailable(i * 4, 4) == false);
		TEST(cacheg.IsDataAvailable(i * 4, 5) == false);
	}
	
	cacheg.WriteAt(0, dummyData, 40);
	TEST(cacheg.SanityCheck());
	TEST(cacheg.IsDataAvailable(0, 40) == true);
	TEST(cacheg.IsDataAvailable(40, 1) == false);
	
	printf("tests finished\n");
}