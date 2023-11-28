
#define VERBOSE 0

#include <xml/BParser.h>
#include <xml/BContent.h>
#include <xml/BWriter.h>

#include <File.h>
#include <StreamIO.h>

#include <stdio.h>
#include <string.h>

using namespace B::XML;

int
main(int argc, char ** argv)
{
	enum {VALIDATE, WELL_FORMED} check;
	const char * input_file = NULL;
	const char * output_file = NULL;
	
	if (argc < 2)
		goto PRINT_USAGE;
	
	if (0==strcmp("-V", argv[1]))
		check = VALIDATE;
	else if (0==strcmp("-W", argv[1]))
		check = WELL_FORMED;
	else
		goto PRINT_USAGE;
	
	if (argc >= 3)
	{
		if (0==strcmp("-o", argv[2]))
		{
			if (argc == 4)
				output_file = argv[3];
			else
				goto PRINT_USAGE;
		}
		else
		{
			input_file = argv[2];
			if (argc >= 4)
			{
				if (0==strcmp("-o", argv[3]))
				{
					if (argc == 5)
						output_file = argv[4];
					else
						goto PRINT_USAGE;
				}
			}
		}
	}
	
	// Done with args now
#if VERBOSE
	if (check == VALIDATE)
		fprintf(stderr, "CHECK: VALIDATE\n");
	else
		fprintf(stderr, "CHECK: WELL_FORMED\n");
	if (!input_file)
		fprintf(stderr, "INPUT: stdin\n");
	else
		fprintf(stderr, "INPUT: \"%s\"\n", input_file);
	if (!output_file)
		fprintf(stderr, "OUTPUT: stdout\n");
	else
		fprintf(stderr, "OUTPUT: \"%s\"\n", output_file);
#endif
	
	{
		BFile * inputFile = NULL;
		BFile * outputFile = NULL;
		
		if (input_file)
			inputFile = new BFile(input_file, B_READ_ONLY);
		if (output_file)
			outputFile = new BFile(output_file, B_WRITE_ONLY | B_CREATE_FILE);
		
		BDataIO & input(inputFile ? *inputFile : BIn);
		BDataIO & output(outputFile ? *outputFile : BOut);
		
		BDocument doc;
		BXMLDataIOSource dataSource(&input);
		BXMLParseContext * context;
		status_t err;
		
		if (check == VALIDATE)
			context = new BXMLValidatingContext(&doc);
		else
			context = new BXMLDocumentParseContext(&doc);
		
		err = ParseXML(context, &dataSource,
						B_XML_HANDLE_NAMESPACES | B_XML_GET_NAMESPACE_DTDS);
		if (err != B_OK)
		{
			fprintf(stderr, "Error 0x%08x occured at line %d  column %d\n",
								err, context->line, context->column);
			return 1;
		}
		
		err = WriteXML(&doc, &output, true);
		if (err != B_OK)
		{
			fprintf(stderr, "Error 0x%08x occurred writing xml file\n", err);
			return 1;
		}
	}
	
	return 0;
	
PRINT_USAGE:
	fprintf(stderr, "checkxml\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Utility for validating and checking the well-formedness\n");
	fprintf(stderr, "of XML documents.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "usage: checkxml CHECK [INPUT] [-o OUTPUT]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Check xml according to OPTIONS. If INPUT is given, use that file\n");
	fprintf(stderr, "otherwise, use stdin.  If OUTPUT is give, write canonical XML to\n");
	fprintf(stderr, "the OUTPUT file.  Otherwise, write it to stdout, unless -s is\n");
	fprintf(stderr, "specified as an option\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "CHECK\n");
	fprintf(stderr, "Check should be one of the following:\n");
	fprintf(stderr, "   -W      Well Formedness. Just see if we can parse it.\n");
	fprintf(stderr, "   -V      Validate.  Validate the Document\n");
	fprintf(stderr, "\n");
	return 0;
}
