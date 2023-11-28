/*
 * @(#)wkcpmgr.c	1.21 97/12/22

	Contains:       Windows only Color Processor management function

	Written by:     Drivin' Team

	Copyright:      (c) 1991-1997 by Eastman Kodak Company, all rights reserved.
 */

/*
    COPYRIGHT (c) Eastman Kodak Company, 1991-1997
    As  an  unpublished  work pursuant to Title 17 of the United
    States Code.  All rights reserved.
*/


#include "kcms_sys.h"

#include <string.h>
#include <time.h>

#include "kcmptlib.h"
#include "kcptmgr.h"
#include "ctelib.h"

/* standard functions */
static void addDirSep (char_p);
static void PTTerminateM_I86(void);

#if defined (KCP_INI_FILE)
#if defined (ICM)
#define KICMINI "kicm32"
PTErr_t makeICMIniFile(initializedGlobals_p iGblP);
#else
PTErr_t makeIniFile(initializedGlobals_p iGblP);
static KpInt32_t getOEMInfo (initializedGlobals_p iGblP);
#endif  /* ICM */

static
void loadHardwareDLL (void);

#define KPCPOEMNAME	0x6FFF

char	kcp_cte_name [200];
#endif	/* defined KCP_INI_FILE */

#define PTFILEEXT	".pt"
#define PTFILETYPE	"KODAK PRECISION Transform"
#define PTKEYNAME	"ptfile"
#define PTKEYDEF	"ptfile\\defaultIcon"

#if !defined(ICM) && !defined(JAVACMM)
static void KpRegisterIcon (initializedGlobals_p iGblP);
#endif  /* !ICM  && !JAVACMM */

int		KcpUsageCount = 0;
static		KpGenericPtr_t	IGPtr = NULL;
char		kcp_ini [200];
HINSTANCE	kcp_cte_hnd;
KcpCteFunc      kcp_cte;

/*****************************************************/
/* open driver to load it into memory, then close it */
/*****************************************************/
PTErr_t
PTInitialize (void)
{
	return (PTInitializeEx(NULL));
}

/*****************************************************/
/* open driver to load it into memory, then close it */
/*                                                   */
/* This version allows Windows apps to specify the   */
/* HINSTANCE (KpModuleId) of the app/dll containing  */
/* a string which points to unique registry entries  */
/* for that application.                             */
/*****************************************************/
PTErr_t
PTInitializeEx (PTInitInfo_t * InitInfo)
{
	threadGlobals_p	threadGlobalsP;
	processGlobals_p processGlobalsP;
	KpInt32_t	retErr;

#if defined (KPWIN16)
/*-----------------------*/
/* get 32-Bit DLL Loaded */
/*-----------------------*/
	load_32Bit_DLL();
#endif


	if (KcpUsageCount == 0) {

		/*------------------------*/
		/* now set up this driver */
		/*------------------------*/

		/* Pass the InitInfo to KCMDSetup.  This
		  info is used to setup the IGPtr data. */
		IGPtr = (KpGenericPtr_t)InitInfo;
		retErr = KCMDsetup (&IGPtr);
		if (retErr != KCMS_SUCCESS)
			return (KCP_NO_MEMORY);

		retErr = KCPappSetup (IGPtr);  /* set globals for this task */
		if (retErr != KCMS_SUCCESS)
			return (KCP_NO_THREAD_GLOBAL_MEM);

		KcpUsageCount++;
  	}
	else {
 		/* find if thread already exists */
		threadGlobalsP = (threadGlobals_p)KpThreadMemFind (&theRootID, KPTHREADMEM);
		if (threadGlobalsP == NULL) {
			/* find if process already exists */
			processGlobalsP = (processGlobals_p)KpThreadMemFind (&theRootID, KPPROCMEM);
			if (processGlobalsP != NULL)	{
				/* get memory for thread globals */
				threadGlobalsP = KpInitThread();
				if (threadGlobalsP == NULL)
					return (KCP_NO_THREAD_GLOBAL_MEM);

				threadGlobalsP->processGlobalsP = processGlobalsP;

				/* increment counter, new thread */
				threadGlobalsP->processGlobalsP->threadCount++;

				/* unlock the globals */
				KCMDunloadGlobals ();
			}
			else {
				/* setup process globals */
				retErr = KCPappSetup (IGPtr);  /* set globals for this task */ 
				if (retErr != KCMS_SUCCESS)
					return (KCP_NO_THREAD_GLOBAL_MEM);

				KcpUsageCount++;
			}
		}
		else {
			threadGlobalsP->threadUseCount++;

			/* unlock thread globals */
			KpThreadMemUnlock (&theRootID, KPTHREADMEM);
		}
	}			

	return (KCP_SUCCESS);
}

/* Initialize platform specific portions of the instance Globals */
void
KCPInitIGblP(KpGenericPtr_t FAR* IGPtr, initializedGlobals_p iGblP)
{

int	len;
PTInitInfo_t	*InitInfo;
SYSTEM_INFO		systemInfo;	
	
	InitInfo = (PTInitInfo_t *) *IGPtr;

/*-----------------------*/
/* get path to .ini file */
/*-----------------------*/
	/* get the instance and module id of the Color Processor */
	iGblP->moduleId = GetModuleHandle ("kpcp32.dll");

	/* make the instance the same as the module id */
	iGblP->instance = iGblP->moduleId;

/* ICM and JAVA do not register the PT icons */
#if !defined(ICM) && !defined(JAVACMM)
	/* register the PT icon */
	 KpRegisterIcon (iGblP);
#endif  /* ICM */

	/* locate windows directory */
    len = GetWindowsDirectory (kcp_ini, sizeof (kcp_ini));
	if (len == 0) {
		diagWindow(" PTInitialize: Can not get Windows Path", len);
		return ;
	}
	addDirSep (kcp_ini);			/* make sure the name is terminated */

 	strcpy (iGblP->KCPDataDir, kcp_ini);	/* default location for CP data */

	GetSystemInfo (&systemInfo);
	iGblP->numProcessors = systemInfo.dwNumberOfProcessors;	/* Get the processor count */

#if defined (KCP_INI_FILE)
#if defined (ICM)
	makeICMIniFile(iGblP);
#else
	/* Setup up iGblP with info passed from APP */
	if (InitInfo == NULL)
		iGblP->appModuleId = NULL;
	else
		iGblP->appModuleId = InitInfo->appModuleId;
	makeIniFile(iGblP);
#endif  /* ICM */
	loadHardwareDLL ();
#endif	/* KCP_INI_FILE */	
}

#if defined (KCP_INI_FILE)
#if defined (ICM)
PTErr_t
makeICMIniFile(initializedGlobals_p iGblP)
{
	KpUInt32_t theSize;
	char defDrv [3];
	char defPath [200];
	char tmp_KCPDataDir [200];

	/* save the default windows drive */
	strncpy (defDrv, iGblP->KCPDataDir, 2);
	defDrv[2] = '\0';

	/* save the default windows path */
	strcpy (defPath, &iGblP->KCPDataDir[2]);

	/* must handle "no .ini file" and "no subdirectory" conditions */
	/* get the drive letter */
	theSize = sizeof(iGblP->KCPDataDir);
	KpReadStringPreference (KICMINI, 0, 0, 0,
							"KEPS Precision", "DRIVE",
							defDrv, iGblP->KCPDataDir, &theSize);

	/* make sure there is no back slash, will be added below */
	iGblP->KCPDataDir[2] = '\0';

	/* get the cp path */
	theSize = sizeof(tmp_KCPDataDir);
	KpReadStringPreference (KICMINI, 0, 0, 0,
							"KCP", "CP_DIR",
							defPath, tmp_KCPDataDir, &theSize);

	if (tmp_KCPDataDir[0] != KcpFileDirSep[0]) {
		addDirSep (iGblP->KCPDataDir);
	}

	strcat (iGblP->KCPDataDir, tmp_KCPDataDir);
	addDirSep (iGblP->KCPDataDir);	/* make sure the name is terminated */

	theSize = sizeof(kcp_cte_name);
	KpReadStringPreference (KICMINI, 0, 0, 0,
							"KCP", "CTE32_NAME",
							"", kcp_cte_name, &theSize);
	return (KCP_SUCCESS);
}
#else
PTErr_t
makeIniFile(initializedGlobals_p iGblP)
{
	KpUInt32_t theSize, len;
	char tmp_KCPDataDir [200];

	/* add name of our .ini file */
	strcat (kcp_ini, "kpcms.ini");

	/* get location of the rules files */
	if (getOEMInfo (iGblP) != 0) {

		/* must handle "no .ini file" and "no subdirectory" conditions */
		/* get the drive letter */
		theSize = sizeof(iGblP->KCPDataDir);
		KpReadStringPreference (kcp_ini, 0, 0, 0,
								"KEPS Precision", "DRIVE",
								"c:", iGblP->KCPDataDir, &theSize);

		/* make sure there is no back slash, will be added below */
		len = strlen(iGblP->KCPDataDir);
		if (iGblP->KCPDataDir[len-1] == KcpFileDirSep[0])
			iGblP->KCPDataDir[len-1] = '\0';

		/* get the cp path */
		theSize = sizeof(tmp_KCPDataDir);
		KpReadStringPreference (kcp_ini, 0, 0, 0,
								"KCP", "CP_DIR",
								"kpcms\\cmscp\\", tmp_KCPDataDir, &theSize);

		if (tmp_KCPDataDir[0] != KcpFileDirSep[0]) {
			addDirSep (iGblP->KCPDataDir);
		}

		strcat (iGblP->KCPDataDir, tmp_KCPDataDir);
		addDirSep (iGblP->KCPDataDir);	/* make sure the name is terminated */

		theSize = sizeof(kcp_cte_name);
		KpReadStringPreference (kcp_ini, 0, 0, 0,
								"KCP", "KPCP_MP",
								"", tmp_KCPDataDir, &theSize);
		if (theSize > 1) {
			iGblP->numProcessors = tmp_KCPDataDir[0] - '0';
			if (iGblP->numProcessors == 0) {
				iGblP->numProcessors = 1;
			}
		}

		/*-----------------------------------*/
		/* get address of hardware evaluator */
		/*-----------------------------------*/
#if defined (KPWIN16)
		GetPrivateProfileString ("KCP", "CTE_NAME", "",
							kcp_cte_name, sizeof(kcp_cte_name), kcp_ini);
#elif defined (KPWIN32)
		theSize = sizeof(kcp_cte_name);
		KpReadStringPreference (kcp_ini, 0, 0, 0,
								"KCP", "CTE32_NAME",
								"", kcp_cte_name, &theSize);
#endif /* end of KPWIN32 */
	}
	return (KCP_SUCCESS);
}

static
KpInt32_t getOEMInfo (initializedGlobals_p iGblP)
{
	KpModuleId appId;
	KpInt32_t ret;
	KpUInt32_t theSize;
	char resBuf[128], regPath[256];
	char tmpBuf [256], iniName[20];

	if (iGblP == NULL) {
		return (-1);
	}

	/* get the ini file name */
	KpFileStripPath (kcp_ini, iniName);

	/* locate the application id for the app that loaded the */
	/* Color Processor  but use the one provided by the app  */
	/* if there is one. */
	if (iGblP->appModuleId == NULL)
		appId = GetModuleHandle (NULL);
	else
		appId = iGblP->appModuleId;
	if (appId == NULL) {
		return (-1);
	}

	/* get the oem company name from the resource of the app */
	ret = KpGetCString (appId, 0, KPCPOEMNAME, resBuf, sizeof(resBuf));
	if (ret != KCMS_SUCCESS) {
		return (-1);
	}

	/* create the registry sub keys */
	strcpy (regPath, "OEM\\");
	strcat (regPath, resBuf);
	strcat (regPath, KcpFileDirSep);
	strcat (regPath, iniName);
	strcat (regPath, KcpFileDirSep);
	strcat (regPath, "KEPS Precision");

	/* get the drive letter */
	theSize = sizeof(iGblP->KCPDataDir);
	ret = KpReadRegistryString (KPLOCAL_MACHINE, regPath, "DRIVE",
								iGblP->KCPDataDir, &theSize);
	if (ret != KCMS_SUCCESS) {
		return (-1);
	}

	/* make sure that the retrun data string is not null */
	if (iGblP->KCPDataDir == NULL) {
		return (-1);
	}

	/* make sure there is no back slash, will be added below */
	iGblP->KCPDataDir[2] = '\0';

	/* create the registry sub keys */
	strcpy (regPath, "OEM\\");
	strcat (regPath, resBuf);
	strcat (regPath, KcpFileDirSep);
	strcat (regPath, iniName);
	strcat (regPath, KcpFileDirSep);
	strcat (regPath, "KCP");

	/* get the CP data path */
	theSize = sizeof(tmpBuf);
	ret = KpReadRegistryString (KPLOCAL_MACHINE, regPath, "CP_DIR",
								tmpBuf, &theSize);
	if (ret != KCMS_SUCCESS) {
		return (-1);
	}

	if (tmpBuf[0] != KcpFileDirSep[0]) {
		addDirSep (iGblP->KCPDataDir);
	}

	strcat (iGblP->KCPDataDir, tmpBuf);
	addDirSep (iGblP->KCPDataDir);

	/* get the hardware accelerator */
	theSize = sizeof(kcp_cte_name);
	KpReadRegistryString (KPLOCAL_MACHINE, regPath, "CTE32_NAME",
								kcp_cte_name, &theSize);
	if (ret != KCMS_SUCCESS) {
		kcp_cte_name[0] = '\0';
	}

	return (0);
}
#endif  /* ICM */
static
void loadHardwareDLL (void)
{
/*------------------------------------------*/
/* get address of hardware evaluator		*/
/* Note that LoadLibrary returns a			*/
/* different value in Win32 than for		*/
/* Win16, therefore the messy conditionals	*/
/*------------------------------------------*/
	if (strcmp (kcp_cte_name, "")) {
#if defined (KPWIN16)
		kcp_cte_hnd = LoadLibrary (kcp_cte_name);
		if (kcp_cte_hnd < HINSTANCE_ERROR) {
            diagWindow(" PTInitialize: Can not load hardware driver", (int)kcp_cte_hnd);
			kcp_cte = NULL;
			kcp_cte_hnd = NULL;
		}
		else
            kcp_cte = (KcpCteFunc)GetProcAddress (kcp_cte_hnd, "CTE_PROC_SEND");											  
#elif defined (KPWIN32)
		kcp_cte_hnd = LoadLibrary (kcp_cte_name);
		if (kcp_cte_hnd == NULL) {
			diagWindow(" PTInitialize: Can not load hardware driver", (int)kcp_cte_hnd);
			kcp_cte = NULL;
			kcp_cte_hnd = NULL;
		}
		else
			kcp_cte = (KcpCteFunc)GetProcAddress (kcp_cte_hnd, "cte_proc_send");
#endif
	}
}

#endif	/* KCP_INI_FILE */


/*************************************/
/* release hardware drive, if loaded */
/*************************************/
static void
PTTerminateM_I86 (void)
{

/* Unload the 32 Bit Evaluation DLL */
#if defined (KPWIN16)
	unload_32Bit_DLL();
#endif

/*   free the library if the cte driver is loaded */
	if (kcp_cte_hnd != 0) {
		FreeLibrary (kcp_cte_hnd);
		kcp_cte = NULL;
		kcp_cte_hnd = 0;
	}
}

/* Platform specific terminate processing */
PTErr_t
PTTerminatePlatform (threadGlobals_p threadGlobalsP)
{
	PTErr_t PTErr = KCP_SUCCESS;

	threadGlobalsP->threadUseCount--;

	if (threadGlobalsP->threadUseCount == 0) {

		threadGlobalsP->processGlobalsP->threadCount--;

		if (threadGlobalsP->processGlobalsP->threadCount == 0) {
			/* free app globals */
			PTErr = PTProcessorReset ();

			KcpUsageCount--;

			if (KcpUsageCount == 0)	{
				/* release Color Processor globals */
				PTErr = KCMDTerminate ();

				/* release hardware driver */
				PTTerminateM_I86 ();
			}

			KpDeleteCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
			KpTermThread (threadGlobalsP);
			KpTermProcess (threadGlobalsP);
		}
		else {
			KpThreadMemUnlock (&theRootID, KPPROCMEM);
			KpTermThread(threadGlobalsP);
		}
	}
	else
		/* unlock the globals */
		KCMDunloadGlobals ();

	return (PTErr);
}

/* make sure a directory pathname has a directory separator at the end */
static void
	addDirSep (char_p theName)
{
	int32	len;

	len = strlen (theName);
	
	if (theName[len-1] != KcpFileDirSep[0]) {
		strcat (theName, KcpFileDirSep);
	}
}

#if !defined(ICM) && !defined(JAVACMM)
static
void KpRegisterIcon (initializedGlobals_p iGblP)
{
	KpOsType_t	wintype;
	KpInt32_t	version, ret;
	DWORD		disposition, bufSize, bufType;
	KpInt8_t	buffer[MAX_PATH], readBuf[MAX_PATH];
	HKEY		hkey;
	KpBool_t	exists;
	char		*fPtr;

	/* get the Windows OS type */
	KpGetSystemInfo (&wintype, &version);

	/* register the PT file only if not Win32s */
	if (wintype != KPOSTYPE_WIN32S)	{
		/* create/open the ".pt" registry key */
		ret = RegCreateKeyEx (HKEY_CLASSES_ROOT, PTFILEEXT, 0, NULL,
								REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
								NULL, &hkey, &disposition);
		if (ret != ERROR_SUCCESS) {
				return;
		}

		/* if this is a new created key, write out to the registry */
		if (disposition == REG_CREATED_NEW_KEY) {
			RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)PTKEYNAME, strlen(PTKEYNAME)+1);
		}
		else {
			/* make sure the data exists */
			bufSize = MAX_PATH;
			bufType = REG_SZ;
			ret = RegQueryValueEx (hkey, NULL, NULL, &bufType,
								(BYTE *)readBuf, (DWORD *)&bufSize);
			if (ret != ERROR_SUCCESS) {
				/* data didn't exists, write it out */
				RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)PTKEYNAME, strlen(PTKEYNAME)+1);
			}
		}

		/* close the registry key */
		RegCloseKey(hkey);

		/* create/open the "ptfile" registry key, this creates the file type */
		ret = RegCreateKeyEx (HKEY_CLASSES_ROOT, PTKEYNAME, 0, NULL,
								REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
								NULL, &hkey, &disposition);
		if (ret != ERROR_SUCCESS) {
			return;
		}

		/* if this is a new created key, write out to the registry the file type */
		if (disposition == REG_CREATED_NEW_KEY) {
			RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)PTFILETYPE, strlen(PTFILETYPE)+1);
		}
		else {
			/* make sure the data exists */
			bufSize = MAX_PATH;
			bufType = REG_SZ;
			ret = RegQueryValueEx (hkey, NULL, NULL, &bufType,
								(BYTE *)readBuf, (DWORD *)&bufSize);
			if (ret != ERROR_SUCCESS) {
				/* data didn't exists, write it out */
				RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)PTFILETYPE, strlen(PTFILETYPE)+1);
			}
		}

		/* close the registry key */
		RegCloseKey(hkey);

		/* create/open the "ptfile\defaultIcon" registry key */
		ret = RegCreateKeyEx (HKEY_CLASSES_ROOT, PTKEYDEF, 0, NULL,
								REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
								NULL, &hkey, &disposition);
		if (ret != ERROR_SUCCESS) {
			return;
		}

		/* get the path of the Color Processor dll */
		ret = GetModuleFileName (iGblP->moduleId, buffer, MAX_PATH);
		if (ret != 0) {
			/* if this is a new created key, write out to the registry */
			if (disposition == REG_CREATED_NEW_KEY) {
				strcat (buffer, ",0");
				RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)buffer, strlen(buffer)+1);
			}
			else {
				/* test if the Color Processor dll exists at the give path */
				bufSize = MAX_PATH;
				bufType = REG_SZ;
				ret = RegQueryValueEx (hkey, NULL, NULL, &bufType,
								(BYTE *)readBuf, (DWORD *)&bufSize);
				if (ret == ERROR_SUCCESS) {
					/* strip off the index at the end of the path */
					fPtr = strrchr (readBuf, ',');
					if (fPtr != NULL) {
						*fPtr = '\0';
					}

					KpFileExists (readBuf, NULL, &exists);
					if (exists == KPFALSE) {
						strcat (buffer, ",0");
						RegSetValueEx (hkey, NULL, 0, REG_SZ, (BYTE *)buffer, strlen(buffer)+1);
					}
				}
			}
		}

		/* close the registry key */
		RegCloseKey(hkey);
	}
}
#endif
ÿ