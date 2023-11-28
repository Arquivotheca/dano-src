//========================================================================
//	stDrawingMode.h
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	


class stDrawingMode {
public:
								stDrawingMode(
									BView*			inView,
									drawing_mode 	inNewMode)
									: fView(inView)
								{
									fMode = fView->DrawingMode();
									fView->SetDrawingMode(inNewMode);
								}
								stDrawingMode(
									BView&			inView,
									drawing_mode 	inNewMode)
									: fView(&inView)
								{
									fMode = fView->DrawingMode();
									fView->SetDrawingMode(inNewMode);
								}

								~stDrawingMode()
								{
									fView->SetDrawingMode(fMode);
								}

private:
	BView*				fView;
	drawing_mode		fMode;	
};
