#include <stdio.h>
#include "DictApp.h"
#include "Dictionary.h"
#include "Thesaurus.h"
#include "Words.h"
#include "DictWindow.h"
#include <Alert.h>

Dictionary *gDictonary;
Thesaurus *gThesaurus;
Words *gWords;

static const char *kDictPath = "/boot/optional/goodies/dictionary";
static const char *kDictIndexPath = "/boot/home/config/settings/dictindex";

static const char *kThesPath = "/boot/optional/goodies/thesaurus";
static const char *kThesIndexPath = "/boot/home/config/settings/thesindex";

static const char *kWordsPath = "/boot/optional/goodies/words";
static const char *kWordsIndexPath = "/boot/home/config/settings/wordsindex";

DictApp::DictApp( void )
	: BApplication( "application/x-vnd.Be-Dictionary" )
{
	
}

DictApp::~DictApp( void )
{
	
}

void DictApp::MessageReceived( BMessage *msg )
{
	switch( msg->what )
	{
		default:
			BApplication::MessageReceived( msg );
			break;
	}
}

void DictApp::FileAlert( void )
{
	BAlert *alert = new BAlert( "alert", "Dictionary requires the optional items on your Be CD.", "Exit", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT  );
	alert->Go();
	PostMessage( B_QUIT_REQUESTED );
}

void DictApp::ReadyToRun( void )
{
	BFile		*dataFile;
	BFile		indexFile;
	
	// Setup Dictionary
	dataFile = new BFile();
	gDictonary = new Dictionary( dataFile );
	
	if( dataFile->SetTo( kDictPath, B_READ_ONLY ) != B_OK )
	{
		FileAlert();
		return;
	}
	
	if( indexFile.SetTo( kDictIndexPath, B_READ_ONLY ) == B_OK )
	{
		gDictonary->UnflattenIndex( &indexFile );
		indexFile.Unset();
	}
	else
	{
		printf( "Building Index...\n" );
		gDictonary->InitIndex();
		gDictonary->BuildIndex();
		if( indexFile.SetTo( kDictIndexPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE ) == B_OK )
			gDictonary->FlattenIndex( &indexFile );
	}
	
	// Setup Thesaurus
	dataFile = new BFile();
	gThesaurus = new Thesaurus( dataFile );
	
	if( dataFile->SetTo( kThesPath, B_READ_ONLY ) != B_OK )
	{
		FileAlert();
		return;
	}
	
	if( indexFile.SetTo( kThesIndexPath, B_READ_ONLY ) == B_OK )
	{
		gThesaurus->UnflattenIndex( &indexFile );
		indexFile.Unset();
	}
	else
	{
		printf( "Building Index...\n" );
		gThesaurus->InitIndex();
		gThesaurus->BuildIndex();
		if( indexFile.SetTo( kThesIndexPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE ) == B_OK )
			gThesaurus->FlattenIndex( &indexFile );
	}
	
	// Setup Words
	dataFile = new BFile();
	gWords = new Words( dataFile );
	
	if( dataFile->SetTo( kWordsPath, B_READ_ONLY ) != B_OK )
	{
		FileAlert();
		return;
	}
	
	if( indexFile.SetTo( kWordsIndexPath, B_READ_ONLY ) == B_OK )
	{
		gWords->UnflattenIndex( &indexFile );
		indexFile.Unset();
	}
	else
	{
		printf( "Building Index...\n" );
		gWords->InitIndex();
		gWords->BuildIndex();
		if( indexFile.SetTo( kWordsIndexPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE ) == B_OK )
			gWords->FlattenIndex( &indexFile );
	}
	
	DictWindow	*win;
	
	win = new DictWindow;
	win->InitChildren();
	win->Show();
	
}
