
#include <support2/StdIO.h>
#include <support2/Container.h>
#include <support2/RandomAccessIO.h>
#include <support2/Looper.h>
#include <support2/TextStream.h>
#include <storage2/File.h>
#include <xml2/XMLParser.h>
#include <xml2/XMLRootSplay.h>

#define FILENAME "/boot/home/config/settings/binder/binder2_root"

int main2()
{
	BLooper::InitMain(this_team()+1);

//	BContainer::ptr container(new BContainer());
//	container->Put(BValue().
//		Overlay("name1","value1").
//		Overlay("name2","value2")
//		);
//	BXMLParser parser(new BXMLRootSplay(container));
//	BXMLIByteInputSource source(new BRandomAccessIO(IStorage::ptr(new BFile(FILENAME,B_READ_ONLY))));
//	ParseXML(&parser,&source,0);

//	berr << "hello " << "world!" << endl;
//	berr << container->Get() << endl;
	
//	BLooper::SetRootObject(container);
	BLooper::SetRootObject(Stdout->AsBinder());
	BLooper::Loop();
}

int main1()
{
	rename_thread(find_thread(NULL),"main1");
	IBinder::ptr child = BLooper::GetRootObject(spawn_thread((thread_entry)main2,"main2",B_NORMAL_PRIORITY,NULL));
/*
	child->Put(BValue().
		Overlay("name3","value3").
		Overlay("name4","value4")
		);
	BValue val = child->Invoke(BValue().
		Overlay("name5","value5").
		Overlay("name6","value6")
		);
	berr << "WOOHOO" << endl << indent << val << dedent << endl;
	berr << "Get interface..." << endl << indent << child->Get(BValue::TypeInfo(typeid(IValueOutput))) << dedent << endl;
*/
	IByteOutput::ptr out = IByteOutput::AsInterface(child);
	
	ITextOutput::ptr outTxt (new BTextOutput(out));
	child = NULL;

	debugger("woohoo");
	outTxt << endl << "**********************************************************This is test output!" << endl;
	outTxt = NULL;

	BLooper::Loop();
	return 0;
}

int main()
{
	return main1();
}
