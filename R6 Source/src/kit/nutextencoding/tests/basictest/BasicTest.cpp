

#include <textencoding/BTextEncoding.h>
#include <textencoding/TextEncodingNames.h>
#include <stdio.h>
#include <image.h>
#include <OS.h>

using namespace B::TextEncoding;
/*
	name				size	shared/each
	libtextencoding:   415409b	404k/8k
		no chinese		86193b	84k/4k
			

	libnutextencoding:	29115b	28k/8k
	language:
		classical.so:	12712b	12k/8k
		cyrillic.so:	 7667b	 8k/4k
		latin.so:		10178b	12k/4k
		misc.so:		10340b	12k/4k
		sjis.so:		28263b	28k/4k
		korean.so:		25773b	24k/8k
	
		big5.so:		43601b	40k/8k
		gbk.so:			72832b	64k/12k
	
	total			   
		no chinese	    

		
	language	
		latin:
			(west and central):
			iso1, iso2, iso3, iso15, ms1250, ms1252, dos437, dos850, dos852,
			dos860, macroman, maccenteuro, macexpert, unicode
		
		classical:
			arabic, greek, hebrew, turkish:
			iso6, iso7, iso8, iso9, ms1253, ms1254, ms1255, ms1256, dos737,  dos857,
			dos862, dos864, dos869, macgreek, machebrew, macturkish
		
		cyrillic:
			cyrillic, russian, baltic:
			iso5, iso13, ms1251, ms1257, dos775, dos855, dos866,
			maccroatian, maccyrillic, koi8r
	
	
		misc:
			nordic, celtic, iceland, french canada, thai, vietnamese:
			iso4, iso10, iso14, ms1258, dos861, dos863, dos865, dos874, maciceland
		

*/

void print_textencoding_memory(FILE *file) {
	fprintf(file, "print_textencoding_memory:\n");
	// look through our team images to get the right data
	thread_info tinfo;
	get_thread_info(find_thread(NULL), &tinfo);
	fprintf(file, "team: %ld\n", tinfo.team);
	team_id team = tinfo.team;
	image_info info;
	int32 cookie = 0;
	fprintf(file, "print image info:\n");
	while (get_next_image_info(team, &cookie, &info) == B_OK) {
//		fprintf(file, "%s id: %ld type: %ld text: %ld data: %ld\n", info.name, info.id, info.type, info.text_size, info.data_size);

		
		
		if ((info.type == B_LIBRARY_IMAGE) && (strstr(info.name, "libnutextencoding.so") != 0)) {
			fprintf(file, "libnutextencoding.so id: %ld text: %ld data: %ld\n", info.id, info.text_size, info.data_size);
		}
		else if (info.type == B_ADD_ON_IMAGE) {
			fprintf(file, "%s id: %ld text: %ld data: %ld\n", info.name, info.id, info.text_size, info.data_size);
		}
	}
	
	// check the areas
	fprintf(file, "print area info:\n");
	area_info ainfo;
	cookie = 0;
	while (get_next_area_info(team, &cookie, &ainfo) == B_OK) {
		fprintf(file, "%s area:%ld size: %ld ram_size: %ld copy_count: %ld\n",
			ainfo.name, ainfo.area, ainfo.size, ainfo.ram_size, ainfo.copy_count);
	}
}

#include <Application.h>
int main() {
	BTextEncoding iso1(ISO1Name);
	BTextEncoding maciceland(MacIcelandName);
	BTextEncoding koi8r(B_KOI8R_CONVERSION);
	BTextEncoding macturkish(MacTurkishName);
	BTextEncoding sjis(SJISName);
	BTextEncoding big5(Big5Name);
	BTextEncoding gbk(GBKName);
	BTextEncoding euckr("euc-kr");
	
	printf("sjis.HasCodec(): %d\n", sjis.HasCodec());
	
	print_textencoding_memory(stderr);

}
