//	StWindowLocker.h


#pragma once

#include <Window.h>
#include <View.h>

class StWindowLocker
{
		BWindow *				fWindow;
public:
inline							StWindowLocker(
									BWindow * window)
								{
									fWindow = window;
									if (fWindow)
										if (!fWindow->Lock())
											fWindow = NULL;
								}
inline							StWindowLocker(
									BView * view)
								{
									fWindow = (view ? view->Window() : NULL);
									if (fWindow)
										if (!fWindow->Lock())
											fWindow = NULL;
								}
inline							~StWindowLocker()
								{
									if (fWindow)
										fWindow->Unlock();
								}
};
