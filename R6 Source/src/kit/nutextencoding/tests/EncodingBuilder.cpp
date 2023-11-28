
#include <SupportDefs.h>
#include "BArray.h"
#include <stdio.h>
#include <string.h>

struct codepoint {
					codepoint(uint16 _charcode = 0x0000, uint16 _unicode = 0x0000) :
						charcode(_charcode),
						unicode(_unicode) {}
		void		print_to_stream(FILE *out) {fprintf(out, "0x%04x->0x%04x\n", charcode, unicode);}
		uint16 charcode;
		uint16 unicode;
};


enum {
			REPLACEMENT,
			BLOCK,
			SEQUENCE
};


class rundef {
	public:
						rundef(uint32 _start, uint32 _end, uint8 _type) :
							start(_start), end(_end), type(_type) {}
		
		uint32			count() const {return (end - start) + 1;};
		uint32			size() {
				
				if (type == BLOCK)
					return 12 + (((end - start) + 1) * 2) + 4;
				else return 12;
			};
		void				print_to_stream(FILE *file);
		void				export_to_stream(FILE *file);
		uint32 start; // index in fCodepoints - not charcode
		uint32 end; // index in fCodepoints - not charcode
		uint8 type;
};

void print_encoding_def_size(BArray<rundef> &array, FILE *out) {
	rundef *items = array.Items();
	uint32 size = 0;
	for (int32 ix = 0; ix < array.CountItems();	ix++) {
		size += items[ix].size();	
	}

	fprintf(out, "%ld:%ld\n", array.CountItems(), size + array.CountItems() * 4 + 4);
};




class EncodingBuilder {
	public:
								EncodingBuilder();
								~EncodingBuilder();
	
		void					Build(const char *inpath, const char *outpath, const char *base, uint8 threshold, const char *fmt);					
	
		status_t				LoadCodePoints(uint32 format);
		uint32					EvaluateFormat(const char *fmt);
		status_t				BuildEncodingDef();
		void					EvaluateBlock(int32 start, int32 after);
		void					OptimizeBlock(BArray<rundef> &block);
					
		status_t				PrintCodePoints(FILE *out);
		status_t				PrintEncodingDef(FILE *out);
		void					PrintBlock(int32 start, int32 after);
	
		status_t				ExportEncoding(FILE *out);	
		void					ExportCodecSubclass(FILE *out);
		codepoint *				CodePoints() { return fCodePoints.Items();}
		const char *			Base() { return fBase; }		
		
		void					PrintSize();
	
	private:
		FILE *					fInput;
		FILE *					fOutput;
		const char *			fBase;
		uint8					fThreshold;
		BArray<codepoint>		fCodePoints;
		BArray<rundef>			fEncodingDef;
		BArray<rundef>			fBlocks;
		BArray<rundef>			fReplacements;
};

EncodingBuilder eb;

void 
rundef::print_to_stream(FILE *out)
{
	codepoint *items = eb.CodePoints();
	char *tString = NULL;
	if (type == REPLACEMENT)
		tString = "BReplacement";
	else if (type == BLOCK)
		tString = "BBlockRun";
	else if (type == SEQUENCE)
		tString = "BSequentialRun";
	fprintf(out, "%s 0x%04x : 0x%04x\n", tString,  items[start].charcode, items[end].charcode);
	
};

void 
rundef::export_to_stream(FILE *out)
{
	codepoint *items = eb.CodePoints();
	switch(type) {
		case REPLACEMENT:
			fprintf(out, "const BEncodingRun\t\t%s_%04X(0x%04X, 0x%04X);\n", eb.Base(), items[start].charcode, items[start].charcode, items[end].unicode );
			break;
		
		case SEQUENCE:
			fprintf(out, "const BEncodingRun\t\t%s_%04X(0x%04X, 0x%04X, %s0x%04X);\n",
				eb.Base(), items[start].charcode, items[start].charcode, items[end].charcode,
				(items[start].unicode == 0) ? "(uint16)" : "",
				items[start].unicode);
			break;
		case BLOCK: {
			fprintf(out, "\nconst uint16\t%sBlock_%04X[] = {\n", eb.Base(), items[start].charcode);
			int32 count = 0;
			for (int32 ix = start; ix <= end; ix++) {
				fprintf(out, "0x%04X", items[ix].unicode);	
				if (ix == end)
					fprintf(out, "\n");
				else if (count < 15)
					fprintf(out, ", ");
				else if (count == 15) {
					fprintf(out,",\n");
					count = -1;
				}
				count++;
			}
			fprintf(out, "};\n");
			fprintf(out, "const BEncodingRun\t\t%s_%04X(0x%04X, 0x%04X, %sBlock_%04X);\n\n",
				eb.Base(), items[start].charcode, items[start].charcode, items[end].charcode, eb.Base(), items[start].charcode);
			break;
		}
	}
}


EncodingBuilder::EncodingBuilder() :
	fInput(NULL), fOutput(NULL), fBase(NULL)
{
}

void 
EncodingBuilder::Build(const char *inpath, const char *outpath, const char *base, uint8 threshold, const char * fmt)
{
	fInput = fopen(inpath, "r");
	fOutput = fopen(outpath, "w+");
	fBase = base;
	fThreshold = threshold;
	LoadCodePoints(EvaluateFormat(fmt));
	BuildEncodingDef();
	printf("----------------------\n");
//	print_encoding_def_size(fBlocks, stdout);
//	print_encoding_def_size(fEncodingDef, stdout);
//
//	printf("replace size: %ld\n",(uint32)(fReplacements.CountItems() * 8));
	PrintSize();
//	PrintEncodingDef(fOutput);
	ExportEncoding(fOutput);
}

EncodingBuilder::~EncodingBuilder()
{
	if (fInput)
		fclose(fInput);
	if (fOutput)
		fclose(fOutput);
	fCodePoints.MakeEmpty();
	fEncodingDef.MakeEmpty();
}

uint32 
EncodingBuilder::EvaluateFormat(const char *fmt)
{
	printf("fmt: %s\n", fmt);
	if (!fmt)
		return 0;
		
	if (strcasecmp(fmt, "0xU\\t0xC") == 0)
		return 1;
	else if (strcasecmp(fmt, "C\\tU") == 0)
		return 2;
	else if (strcasecmp(fmt, "U\\tC") == 0)
		return 3;
	else if (strcasecmp(fmt, "C:U") == 0)
		return 4;
	else
		return 0;	
}

// formats
//0:	0xC\t0xU
//1:	0xU\t0xC
//2:	C\tU
//3:	U\tC
//4:	C:U

status_t 
EncodingBuilder::LoadCodePoints(uint32 format)
{
	// seek the file to the beginning
	printf("use fmt %d\n", format);
	if (!fInput)
		return B_NO_INIT;
	fseek(fInput, 0L, SEEK_SET);
	
	char line[256];
	while (fgets(line, 256, fInput) != NULL) {
		if (line[0] != '#') {
			codepoint cp;
			size_t cnt = 0;

			if (format == 1)
				cnt = sscanf(line, "%hi\t%hi", &cp.unicode, &cp.charcode);
			else if (format == 2) {
				char C[16];
				char U[16]; 
				strcpy(C, "0x"); 
				strcpy(U, "0x");
				
				cnt = sscanf(line, "%s\t%s", &C[2], &U[2]);
				sscanf(C, "%hi", &cp.charcode);
				sscanf(U, "%hi", &cp.unicode);
			}
			else if (format == 3) {
				char C[16];
				char U[16]; 
				strcpy(C, "0x"); 
				strcpy(U, "0x");
				
				cnt = sscanf(line, "%s\t%s", &U[2], &C[2]);
				sscanf(C, "%hi", &cp.charcode);
				sscanf(U, "%hi", &cp.unicode);
			}
			else if (format == 4) {
				char C[16];
				char U[16]; 
				strcpy(C, "0x"); 
				strcpy(U, "0x");
				
				cnt = sscanf(line, "%4c:%s", &C[2], &U[2]);
				sscanf(C, "%hi", &cp.charcode);
				sscanf(U, "%hi", &cp.unicode);
			}
			else
				cnt = sscanf(line, "%hi\t%hi", &cp.charcode, &cp.unicode);
			
			//cp.print_to_stream(stdout);
			if (cnt == 2)
				fCodePoints.AddItem(cp);
		}
	}
//	PrintCodePoints(stdout);
}

status_t 
EncodingBuilder::BuildEncodingDef()
{
	int32 count = fCodePoints.CountItems();
	codepoint *items = fCodePoints.Items();
	
	int32 ix = 0;
	while (ix < count) {
		
		int32 jx = 1;
		
		while (ix + jx < count && items[ix + jx].charcode == items[ix].charcode + jx)
			jx++;
		
		// so jx is the item after the end of a block
		EvaluateBlock(ix, ix + jx);
		ix += jx;	
	}
	
	BArray<rundef> tmpDefs(fEncodingDef);
	fEncodingDef.MakeEmpty();
	OptimizeBlock(tmpDefs);
	
	tmpDefs.MakeEmpty();
	tmpDefs.AddArray(&fEncodingDef);
	fEncodingDef.MakeEmpty();
	
	
	rundef *defs = tmpDefs.Items();
	for (int ix = 0; ix < tmpDefs.CountItems(); ix++) {
		if (defs[ix].type == REPLACEMENT)
			fReplacements.AddItem(defs[ix]);
		else
			fEncodingDef.AddItem(defs[ix]);
	}
}

void 
EncodingBuilder::EvaluateBlock(int32 start, int32 after)
{
	//PrintBlock(start, after);
	rundef b(start, after - 1, BLOCK);
	fBlocks.AddItem(b);
	codepoint *items = fCodePoints.Items();
	BArray<rundef> block;
	fprintf(stdout, "EvaluateBlock: 0x%04x : 0x%04x\n", items[start].charcode, items[after - 1].charcode);
	
	
	// shortcut if there is only one item
	// if there are two or fewer items they will always be replacements
	if (after - start <=2) {
		printf("adding replacements: 0x%04x - 0x%04x\n", items[start].charcode, items[after-1].charcode);
		for (int32 ix = start; ix < after; ix++) {
			fEncodingDef.AddItem(rundef(ix, ix, REPLACEMENT));
		}
		return;
	}

	BArray<rundef> seq;
	
	for (int32 ix = start; ix < after; ix++) {
		int32 jx = 1;
		while ( ix + jx < after && items[ix+jx].unicode == items[ix].unicode + jx)
			jx++;
		
		if (jx > fThreshold) {
			printf("adding temp sequence: 0x%04x - 0x%04x\n", items[ix].charcode, items[ix+jx-1].charcode);
			rundef rd(ix, ix + jx - 1, SEQUENCE);
			seq.AddItem(rd);
		}
		ix += jx - 1;
	}
	

	int32 sx = seq.CountItems();
	int32 ix = start;
	while (ix < after && sx >= 0)
	{
		int32 end = after;
		if (sx > 0)
			end = seq[0].start;
		
		
		while (ix < end) {
			int32 jx = 1;
			while (ix+jx < end && items[ix].charcode + jx == items[ix+jx].charcode)
				jx++;
			
			if (jx > 3) {
				rundef rd(ix, ix+jx-1, BLOCK);
				block.AddItem(rd);
				printf("adding block: 0x%04x - 0x%04x\n", items[rd.start].charcode, items[rd.end].charcode);
			}
			else {
				printf("adding replacements: 0x%04x - 0x%04x\n", items[ix].charcode, items[ix+jx-1].charcode);
				for (int32 rx = 0; rx < jx && ix+rx < end; rx++) {
					rundef rd(ix+rx, ix+rx, REPLACEMENT);
					block.AddItem(rd);
				}
			}
			ix +=jx;
		}
		
		if (sx > 0) {
			printf("adding sequence: 0x%04x - 0x%04x\n", items[seq[0].start].charcode, items[seq[0].end].charcode);
			ix = seq[0].end + 1;
			block.AddItem(seq[0]);
			seq.RemoveItem(0L);
		}
		sx--;
	}

	
//	int32 bx = start;
//	bool inSeq = false;
//	while (bx < after) {
//		fprintf(stdout, "start loop: %ld < %ld\n", bx, after);
//		int32 jx = 1;
//		while ( bx + jx < after && items[bx+jx].unicode == items[bx].unicode + jx)
//			jx++;
//	
//		
//		if (jx > fThreshold) {
//			rundef rd(bx, bx + jx -1, SEQUENCE);
//			block.AddItem(rd);
//			rd.print_to_stream(stdout);
//			bx += jx;
//		}
//		else {
//			// we have a block or replacement - walk the list
//			uint16 lastuni = items[bx+jx].unicode;
//			for (jx ; bx + jx < after; jx++) {
//				uint16 unicode = items[bx+jx].unicode;
//				if (unicode == lastuni + 1)
//					break;
//				else
//					lastuni = unicode; 
//			}
//			if (bx + jx == after)
//				jx = after - bx + 1;
//			
////			fprintf(fOutput, "jx: %ld\n", jx);
//			
//			rundef rd(bx, bx + jx - 2, (jx == 2) ? REPLACEMENT : BLOCK);
//			block.AddItem(rd);
//			rd.print_to_stream(stdout);
//			bx += (jx - 1);
//		}
//	}
	
#if 1
	OptimizeBlock(block);
#else
	fEncodingDef.AddArray(&block);
#endif
	
}

void 
EncodingBuilder::OptimizeBlock(BArray<rundef> &block)
{
	print_encoding_def_size(block, stdout);
	
	BArray<rundef> optimal;
	int32 count = block.CountItems();
	bool optimized = false;
	rundef *defs = block.Items();
	codepoint *codes = CodePoints();
	
	for (int32 ix = 0; ix < count; ix++) {
		uint8 type = defs[ix].type;
		if (type == BLOCK) {
			if (defs[ix].end - defs[ix].start + 1 <= 3) {
				printf("replace block: 0x%04x - 0x%04x\n", defs[ix].start, defs[ix].end);
				for (int32 jx = defs[ix].start; jx <= defs[ix].end; jx++) {
					rundef rd(jx, jx, REPLACEMENT);
					printf("adding 0x%04x\n", jx);
					optimal.AddItem(rd);
				}
				ix += (defs[ix].end - defs[ix].start + 1);
				optimized = true;
			}
			else if (ix + 1 < count && defs[ix+1].type == BLOCK && codes[defs[ix].end].charcode + 1 == codes[defs[ix + 1].start].charcode) {
				// two adjacent consecutive blocks
				printf("combining 2 adjacent blocks: 0x%04x to 0x%04x\n", codes[defs[ix].start].charcode, codes[defs[ix + 1].end].charcode);
				rundef rd(defs[ix].start, defs[ix+1].end, BLOCK);
				optimal.AddItem(rd);
				ix++;
				optimized = true;
			}
			else if ((ix + 2 < count) &&
					((defs[ix+1].type == SEQUENCE) &&
						(defs[ix + 1].count() < 7)) &&
					(defs[ix + 2].type == BLOCK) &&
					codes[defs[ix].end].charcode + 1 == codes[defs[ix + 1].start].charcode &&
					codes[defs[ix+1].end].charcode + 1 == codes[defs[ix + 2].start].charcode) {
				// one big block
				printf("making one big block: from 0x%04x to 0x%04x\n", codes[defs[ix].start].charcode, codes[defs[ix + 2].end].charcode);
				rundef rd(defs[ix].start, defs[ix+2].end, BLOCK);
				optimal.AddItem(rd);
				ix += 2;
				optimized = true;
			}
			else if (ix + 1 < count && defs[ix + 1].type == REPLACEMENT &&
				codes[defs[ix + 1].start].charcode == codes[defs[ix].end].charcode) {
				printf("adding replacement to end of block: 0x%04x to 0x%04x\n", codes[defs[ix].start].charcode, codes[defs[ix + 1].end].charcode);
				rundef rd(defs[ix].start, defs[ix+1].end, BLOCK);
				optimal.AddItem(rd);
				ix ++;
				optimized = true;	
			}
			else
				optimal.AddItem(defs[ix]);
		}
		else if (type == REPLACEMENT) {
			if (ix + 1 < count && defs[ix+1].type == BLOCK && codes[defs[ix].start].charcode +1 == codes[defs[ix+1].start].charcode) {
				printf("adding replacement to front of block: 0x%04x to 0x%04x\n", codes[defs[ix].start].charcode, codes[defs[ix + 1].end].charcode);
				rundef rd(defs[ix].start, defs[ix+1].end, BLOCK);
				optimal.AddItem(rd);
				ix++;
				optimized = true;
			}
//			else if (ix + 2 < count && defs[ix+1].type == REPLACEMENT && defs[ix+2].type == REPLACEMENT &&
//				codes[defs[ix].start].charcode + 1 == codes[defs[ix+1].start].charcode &&
//				codes[defs[ix+1].start].charcode + 1 == codes[defs[ix+2].start].charcode) {
//				rundef rd(defs[ix].start, defs[ix+2].end, BLOCK);
//				optimal.AddItem(rd);
//				ix += 2;
//				optimized = true;
//			}
			else
				optimal.AddItem(defs[ix]);
		}
		else if (type == SEQUENCE) {
			if (ix + 2 < count && defs[ix+1].type == REPLACEMENT && defs[ix+2].type == SEQUENCE  &&
				codes[defs[ix].end].charcode + 1 == codes[defs[ix+1].start].charcode &&
				codes[defs[ix+1].end].charcode + 1 == codes[defs[ix+2].start].charcode &&
				codes[defs[ix].end].unicode + 2 == codes[defs[ix+2].start].unicode) {
				printf("joining sequence around a replacement: 0x%04x to 0x%04x\n", codes[defs[ix].start].charcode, codes[defs[ix + 2].end].charcode);
				optimal.AddItem(defs[ix+1]);
				rundef rd(defs[ix].start, defs[ix+2].end, SEQUENCE);
				optimal.AddItem(rd);
				ix += 2;
				optimized = true;
			}
			else
				optimal.AddItem(defs[ix]);
		}
	} 	

	if (optimized) {
		uint32 oldSize = 0;
		uint32 oldCount = block.CountItems();
		uint32 newSize = 0;
		uint32 newCount = optimal.CountItems();
		for (int ix = 0; ix < oldCount; ix++)
			oldSize += block[ix].size();	
		for (int ix = 0; ix < newCount; ix++)
			newSize += optimal[ix].size();	
		printf("optimized block: %ld:%ld -> %ld:%ld (saved %ld:%ld)\n", oldCount, oldSize, newCount, newSize, oldCount - newCount, oldSize - newSize);
		OptimizeBlock(optimal);
	}else 
		fEncodingDef.AddArray(&block);
}

void 
EncodingBuilder::PrintBlock(int32 start, int32 after)
{
	codepoint *items = fCodePoints.Items();
	fprintf(fOutput, "print block --------------------------\n");
	for (int32 bx = 0; bx < after; bx++) {
		items[start + bx].print_to_stream(fOutput);
	}
}

status_t 
EncodingBuilder::PrintCodePoints(FILE *out)
{
	int32 count = fCodePoints.CountItems();
	codepoint *items = fCodePoints.Items();
	
	for (int32 ix = 0; ix < count; ix++) {
		items[ix].print_to_stream(out);
	} 
}

status_t 
EncodingBuilder::PrintEncodingDef(FILE *out)
{
	int32 count = fEncodingDef.CountItems();
	rundef *items = fEncodingDef.Items();
	for (int32 ix = 0; ix < count; ix++) {
		items[ix].print_to_stream(out);
	} 
}

status_t 
EncodingBuilder::ExportEncoding(FILE *out)
{
	fprintf(out, "\n#include <textencoding/BTextEncoding.h>\n#include <textencoding/TextEncodingNames.h>\n");
	fprintf(out, "#include <add-ons/textencoding/BTextCodec.h>\n#include <add-ons/textencoding/BEncodingRun.h>\n#include <string.h>\n");
	fprintf(out, "\nusing namespace B::TextEncoding;\n\n");
	int32 count = fEncodingDef.CountItems();
	rundef *items = fEncodingDef.Items();
	for (int32 ix = 0; ix < count; ix++) {
		items[ix].export_to_stream(out);
	}
	
	fprintf(out, "\nconst BEncodingRun * const %sStack[] = {\n", Base());
	codepoint *codes = CodePoints();
	int32 c = 0;
	for (int32 ix = 0; ix < count; ix++) {
		fprintf(out, "&%s_%04X", Base(), codes[items[ix].start].charcode);
		if (ix == count - 1)
			fprintf(out, "\n");
		else if (c < 8)
			fprintf(out, ", ");
		else if (c == 8) {
			fprintf(out,",\n");
			c = -1;
		}
		c++;
	}
	fprintf(out, "};\n");
	fprintf(out, "const uint16 %sStackSize = %ld;\n\n", Base(), count);

	
	uint16 stackSize = count;
	count = fReplacements.CountItems();
	items = fReplacements.Items();
	
	if (count > 0) {
		for (int ix = 0; ix < count; ix++) {
			fprintf(out, "const BReplacement	%sReplace_%04X(0x%04X, 0x%04X);\n", Base(), codes[items[ix].start].charcode, codes[items[ix].start].charcode, codes[items[ix].start].unicode);
		}
		fprintf(out, "const BReplacement * const %sReplacements[] = {\n", Base());
		for (int32 ix = 0; ix < count; ix++) {
			fprintf(out, "&%sReplace_%04X", Base(), codes[items[ix].start].charcode);
			if (ix == count - 1)
				fprintf(out, "\n");
			else if (c < 8)
				fprintf(out, ", ");
			else if (c == 8) {
				fprintf(out,",\n");
				c = -1;
			}
			c++;
		}
		fprintf(out, "};\n");
		fprintf(out, "const uint16 %sReplacementCount = %ld;\n\n", Base(), count);
	}
	
	
	ExportCodecSubclass(out);
	fprintf(out, "\nBTextCodec * make_codec(const char * encoding, BTextEncodingAddOn *addOn) {\n");
//	if (count > 0)
		fprintf(out, "\tif (strcasecmp(encoding,%sName) == 0)\n\t\treturn new %sCodec(addOn);\n", Base(), Base());
//	else
//		fprintf(out, "\tif (strcmp(encoding, XXXX) == 0)\n\t\treturn new BTextCodec(%Stack, %ld, addOn);\n", Base(), stackSize);
	fprintf(out, "\telse\n\t\treturn NULL;\n}\n\n");
}

void 
EncodingBuilder::ExportCodecSubclass(FILE *out)
{
	printf("ExportCodecSubclass\n");
	fprintf(out, "\nclass %sCodec : public BTextCodec {\n", Base());
	fprintf(out, "\tpublic:\n");
	fprintf(out, "\t\t\t\t\t\t\t\t\t%sCodec(const BEncodingRun * const *array, uint16 arraySize, const BReplacement * const *replacements, uint16 replaceCount, BTextEncodingAddOn *addOn, uint32 flags = 0);\n", Base());
	fprintf(out, "\t\tvirtual\t\t\t\t\t\t~%sCodec();\n", Base());
	fprintf(out, "\t\tvirtual status_t\t\t\tConvertToUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info, BEncodingStack *stack) const;\n");
	fprintf(out, "\t\tvirtual status_t\t\t\tConvertFromUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info, BEncodingStack *stack) const;\n");
	fprintf(out, "};\n\n");
	
	fprintf(out, "%sCodec::%sCodec(BTextEncodingAddOn *addOn) :\n\tBTextCodec(%sStack, %sStackSize, ", Base(), Base(), Base(), Base());
	if (fReplacements.CountItems() > 0)
		fprintf(out, "%sReplacements, %sReplacementCount, ", Base(), Base());
	else
		fprintf(out, "NULL, 0, ");
	fprintf(out, "addOn)\n{\n}\n\n");
	fprintf(out, "%sCodec::~%sCodec()\n{\n}\n\n", Base(), Base());
	fprintf(out, "status_t\n%sCodec::ConvertToUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info, BEncodingStack *stack) const\n{\n\treturn BTextCodec::ConvertToUnicode(src, srcLen, dst, dstLen, info, stack);\n}\n\n", Base());
	fprintf(out, "status_t\n%sCodec::ConvertFromUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info, BEncodingStack *stack) const\n{\n\treturn BTextCodec::ConvertFromUnicode(src, srcLen, dst, dstLen, info, stack);\n}\n\n", Base());
}



void 
EncodingBuilder::PrintSize()
{
	uint32 blockSize = 0;
	rundef *blockItems = fBlocks.Items();
	uint32 blockCount = fBlocks.CountItems();
	for (int32 ix = 0; ix < blockCount; ix++) {
		blockSize += blockItems[ix].size();
	}
	blockSize += blockCount * 4;
	blockSize += 4;
	
	uint32 defSize = 0;
	rundef *defs = fEncodingDef.Items();
	uint32 defCount = fEncodingDef.CountItems();
	for (int32 ix = 0; ix < defCount; ix++) {
		defSize += defs[ix].size();
	}
	defSize += defCount * 4;
	defSize += 4;
	
	uint32 replaceCount = fReplacements.CountItems();
	uint32 replaceSize = (replaceCount * 8) + 4;

	printf("original: %ld for %ld\nend: %ld\n\tarray: %ld for %ld\n\treplace: %ld for %ld\n", blockCount, blockSize, defSize + replaceSize,
		defCount, defSize, replaceCount, replaceSize);
	

};

#include <stdlib.h>
int main(int argc, char *argv[]) {

	if (argc < 5)
		exit;
	
	printf("argc: %ld\n", argc);
	
	eb.Build(argv[1], argv[2], argv[3], atoi(argv[4]), (argc > 5) ? argv[5]: NULL);	
}

