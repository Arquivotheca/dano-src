#ifndef _DRIVE_NODE_H_
#define _DRIVE_NODE_H_

#include <Binder.h>


/*=================================================================*/

class DriveNode : public BinderContainer
{
	public:
									DriveNode			(const char* device,
														 bool removable,
														 int type,
														 const char* vendor,
														 const char* product,
														 const char* version);
									~DriveNode			();
		void						AddVolume			(const char* name,
														 const char* volume);
		void						RemoveVolume		(const char* name);

	private:
		BinderContainer*			fVolumes;
};
#endif
