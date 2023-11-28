#include <stdio.h>
#include <File.h>
#include <XSLTProcessor.h>
#include <XMLWriter.h>

using namespace BXmlKit;

int
main(int argc, char ** argv)
{
	status_t err;
	
	if (argc != 4) {
		fprintf(stderr, "usage: xslt <source file> <xslt file> <out file>\n");
		return 1;
	}
	
	const char * sourceFilename = argv[1];
	const char * xsltFilename = argv[2];
	const char * outFilename = argv[3];
	
	BXMLDocumentParser tools(0);
	BDocument doc;
	BDocument xsltDoc;
	BFile file(sourceFilename, B_READ_ONLY);
	BFile xsltFile(xsltFilename, B_READ_ONLY);
	
	tools.Parse(&file, &doc);
	tools.Parse(&xsltFile, &xsltDoc);
	
	BXsltParseTree xslt;
	MakeXsltTree(xsltDoc.Element(), &xslt);
	
	BElement out("out");
	
	BootstrapXSLTProcessing(&out, &xslt, &doc);
	
	BFile outFile(outFilename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	WriteHTML(out.FirstChild(B_XML_ELEMENT_CONTENT), &outFile);
	
	return 0;
}
