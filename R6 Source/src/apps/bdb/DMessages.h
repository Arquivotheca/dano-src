/*	$Id: DMessages.h,v 1.15 1999/05/11 21:31:04 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/08/98 21:15:08
*/

/*
	These are the messages that can be sent to the ThreadWindow.
	They all expect to find a to_num_msg in the message
*/

#define kMsgThreadStopped		'dThS'
#define kMsgTeamCreated			'dTmC'
#define kMsgTeamDeleted			'dTmD'
#define kMsgPefImageCreated		'dPfC'
#define kMsgPefImageDeleted		'dPfD'
#define kMsgPeImageCreated		'dPeC'
#define kMsgPeImageDeleted		'dPeD'
#define kMsgElfImageCreated		'dElC'
#define kMsgElfImageDeleted		'dElD'
#define kMsgThreadCreated		'dThC'
#define kMsgThreadDeleted			'dThD'

/*
	And these are the messages that control the ThreadWindow
*/

#define kMsgRun						'Run '
#define kMsgStop					'Stop'
#define kMsgStep					'Step'
#define kMsgStepOver				'StOv'
#define kMsgStepOut					'StOu'
#define kMsgKill					'Kill'
#define kMsgDebugProc				'DbPr'
#define kMsgDebugThread				'DbTh'

#define kMsgSwitchRun				'SRun'
#define kMsgSwitchStep				'SStp'
#define kMsgSwitchStepOver			'SSOv'
#define kMsgSwitchStepOut			'SSOu'

#define kMsgShowDebuggerMsg		'ShDb'

/*
	Messages for communication between several windows
*/

#define kMsgSetBreakpoint			'StBP'
#define kMsgClearBreakpoint			'ClBP'

#define kMsgNewRegisterData			'NwRg'
#define kMsgRegisterModified		'RgMd'
#define kMsgShowRegisters			'ShRg'

#define kMsgStackListSelection		'SlSl'
#define kMsgFileSelection			'PrSl'
#define kMsgFileOpenInPreferredEditor 'OprE'

#define kMsgQuit					'Quit'
#define kMsgPulse					'Puls'
#define kMsgAbout				'Abut'

#define kMsgThreadWindowClosed		'ThCl'
#define kMsgTeamWindowClosed		'TmCl'
#define kMsgThreadWindowCreated 	'ThCr'

#define kMsgFuncPopup				'PopF'
#define kMsgJumpToFunc				'JmpF'

#define kMsgFormatDefault			'FmDf'
#define kMsgFormatSigned			'FmSi'
#define kMsgFormatUnsigned			'FmUs'
#define kMsgFormatHex				'FmHx'
#define kMsgFormatOctal			'FmOc'
#define kMsgFormatChar				'FmCh'
#define kMsgFormatString			'FmSt'
#define kMsgFormatFloat				'FmFl'
#define kMsgFormatEnum				'FmEn'
#define kMsgDoFormatPopup		'FmPP'

#define kMsgDumpMemory				'DmpM'

#define kMsgBreakpointsChanged		'BrkC'
#define kMsgShowBreakpoints			'ShBr'

#define kMsgWatchpointsChanged		'WpkC'
#define kMsgShowWatchpoints			'ShWp'

#define kMsgShowAddress				'ShAd'
#define kMsgShowAddressCmd			'ShAC'
#define kMsgShowFunction			'ShFn'
#define kMsgShowFunctionCmd			'ShFC'
#define kMsgSetWatchpoint			'StWd'
#define kMsgSetWatchpointCmd		'StWC'
#define kMsgShowAssemblyCmd			'ShAs'

#define kMsgBreakOnException		'BrCE'
#define kMsgBreakOnLoadAddon		'BrLA'
#define kMsgBreakOnSpawnThread		'BrST'


#define kMsgFind							'Find'
#define kMsgDoFind						'DFnd'
#define kMsgFindAgain					'FnAg'
#define kMsgFindAgainBackward		'FnAB'
#define kMsgFindSelection				'FnSl'
#define kMsgFindSelectionBackward	'FnSB'
#define kMsgEnterSearchString			'FnES'

#define kMsgDumpAddOn				'DmpA'

#define kMsgWhereIs						'Wher'
#define kMsgRefreshVariables			'Rfrs'

#define kMsgSelectWindow				'Slct'

#define kMsgSaveState					'SvSt'

#define kMsgSettings						'Sett'

#define kMsgViewAs						'Cast'
#define kMsgViewAsCmd				'CstC'
#define kMsgViewAsArray				'CstA'
#define kMsgAddExpression				'AdEx'
#define kMsgAddWatchpoint				'AdWp'

#define kMsgStatus						'Stat'

#define kMsgFileLocated					'Loca'
#define kMsgFilesChanged				'FlCh'

#define kMsgGlobalsChanged			'GlCh'

#define kMsgRefreshMemory				'RefM'
#define kMsgScrollToAnchor				'ScAn'
#define kMsgNewMemoryLocation			'NMLo'
#define kMsgNewMemoryLocationCmd		'NMLC'
#define kMsgPreviousMemory				'PMem'
#define kMsgNextMemory					'NMem'
