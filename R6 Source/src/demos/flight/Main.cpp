#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Main.h"
#include "BG_Graphic.h"
#include <stdio.h>

int main()
{
	new BApplication("application/x-vnd.Be-FSIM");
	
	BG_Init();	
  	be_app->Run();
	BG_Dispose();

/*	{
		thread_id   id;
		thread_info info;
		team_info   info2;
		team_id     team;
		int         i;
		
		id = find_thread("Flight");
		get_thread_info(id,&info);
		team = info.team;
		get_team_info(team,&info2);
		fprintf(stderr,"Threads running : %d\n",info2.thread_count);
		if (info2.thread_count > 1) {
			for (i=0;i<info2.thread_count;i++) {
				get_nth_thread_info(team,i,&info);
				fprintf(stderr,"Thread %d : %s\n",info.thread,info.name);
			}
		}
	}*/
	delete be_app;
	return 0;
}




