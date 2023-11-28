#include "DictApp.h"
#include <stdio.h>

int main( void )
{
	new DictApp;
	be_app->Run();
	delete( be_app );
}
