
/*** possible improvement: search for translator chain when ***/
/*** no direct translation is available ***/

#include <TranslationKit.h>
#include <File.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ByteOrder.h>


static void
usage()
{
	fprintf(stderr, "usage: translate { --list | input output format }\n");
	exit(1);
}

static void
error(
	status_t err, 
	const char * message)
{
	if (err < B_OK) {
		fprintf(stderr, "%s: %x (%s)\n", message, err, strerror(err));
		exit(3);
	}
}

static const char * 
rev(
	uint32 code,
	char * out)
{
	out[0] = code>>24;
	out[1] = code>>16;
	out[2] = code>>8;
	out[3] = code;
	out[4] = 0;
	return out;
}


type_code type_from_string(const char * str)
{
	const unsigned char * ptr = (const unsigned char *)str;
	type_code tc = (*(ptr++)<<24);
	tc |= (*(ptr++)<<16);
	tc |= (*(ptr++)<<8);
	tc |= *ptr;
	return tc;
}


int
main(
	int argc,
	char * argv[])
{
	BTranslatorRoster * r = BTranslatorRoster::Default();
	status_t err;
	if (!r) {
		fprintf(stderr, "translate: can't initialize translation kit\n");
		exit(2);
	}
	if (argc != 2 && argc != 4) {
		usage();
	}
	if (argc == 2 && strcmp(argv[1], "--list")) {
		usage();
	}
	else if (argc == 2) {
		translator_id * ids = NULL;
		int32 num_ids = 0;
		err = r->GetAllTranslators(&ids, &num_ids);
		error(err, "GetAllTranslators()");
		for (int t = 0; t < num_ids; t++) {
			const char * name;
			const char * info;
			int32 version;
			err = r->GetTranslatorInfo(ids[t], &name, &info, &version);
			error(err, "GetTranslatorInfo()");
			printf("Translator %s version %.2f\n", 
				name, (float)version/100.0);
			printf("%s\n", info);
			int32 num_fmts = 0;
			const translation_format * fmts = NULL;
			err = r->GetInputFormats(ids[t], &fmts, &num_fmts);
			error(err, "GetInputFormats()");
			printf("inputs: %d\n", num_fmts);
			char rev1[5], rev2[5];
			for (int in=0; in<num_fmts; in++) {
				printf("'%s' (%s) %g %g %s ; %s\n", rev(fmts[in].type, rev1), 
					rev(fmts[in].group, rev2), (float)fmts[in].quality, 
					(float)fmts[in].capability, fmts[in].MIME, fmts[in].name);
			}
			num_fmts = 0;
			fmts = NULL;
			err = r->GetOutputFormats(ids[t], &fmts, &num_fmts);
			error(err, "GetOutputFormats()");
			printf("outputs: %d\n", num_fmts);
			for (int in=0; in<num_fmts; in++) {
				printf("'%s' (%s) %g %g %s ; %s\n", rev(fmts[in].type, rev1), 
					rev(fmts[in].group, rev2), (float)fmts[in].quality, 
					(float)fmts[in].capability, fmts[in].MIME, fmts[in].name);
			}
			printf("\n");
		}
		delete[] ids;
		return 0;
	}
	BFile input;
	err = input.SetTo(argv[1], O_RDONLY);
	error(err, argv[1]);
	translator_info out_info;
	uint32 out_code;

	if (strchr(argv[3], '/') != NULL) {
		bool ok = false;
		translator_id * ids = NULL;
		int32 num_ids = 0;
		status_t err = r->GetAllTranslators(&ids, &num_ids);
		error(err, "GetAllTranslators()");
		for (int t = 0; t < num_ids && !ok; t++) {
			const char * name;
			const char * info;
			int32 version;
			err = r->GetTranslatorInfo(ids[t], &name, &info, &version);
			error(err, "GetTranslatorInfo()");
			int32 num_fmts = 0;
			const translation_format * fmts = NULL;
			err = r->GetOutputFormats(ids[t], &fmts, &num_fmts);
			error(err, "GetOutputFormats()");
			for (int in=0; in<num_fmts; in++) {
				if (!strcasecmp(fmts[in].MIME, argv[3])) {
					ok = true;
					out_code = fmts[in].type;
					break;
				}
			}
		}
		delete[] ids;
		if (!ok) {
			goto bad_mojo;
		}
	}
	else if (strlen(argv[3]) != 4) {
bad_mojo:
		fprintf(stderr, "bad format: %s\n", argv[3]);
		fprintf(stderr, "format is 4-byte type code or full MIME type\n");
		exit(3);
	}
	else {
		out_code = type_from_string(argv[3]);
	}
	err = r->Identify(&input, NULL, &out_info, 0, NULL, out_code);
	error(err, "Identify()");
	BFile output;
	err = output.SetTo(argv[2], O_RDWR | O_CREAT | O_TRUNC);
	error(err, argv[2]);
	err = r->Translate(&input, &out_info, NULL, &output, out_code);
	error(err, "Translate()");
	return 0;
}
