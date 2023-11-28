#if ! defined SSDEFS_INCLUDED
#define SSDEFS_INCLUDED

extern const char *module_runner_signature;
extern const char* const kModuleRunnerPath;

extern const char* const kScreenSaversDir;

extern const char *SETTINGS_FILE_NAME;

extern const char *kWindowFrame;
extern const char *kWindowTab;

extern const char *kCornerNow;
extern const char *kCornerNever;

extern const char *kTimeFlags;
extern const char *kTimeFade;
extern const char *kTimeStandby;
extern const char *kTimeSuspend;
extern const char *kTimeOff;

extern const char *kLockEnable;
extern const char *kLockDelay;
extern const char *kLockMethod;
extern const char *kLockPassword;
extern const char *kLockSettingsPrefix;

extern const char *kModuleName;
extern const char *kModuleSettingsPrefix;

enum Corner
{
	kCornerLeftTop,
	kCornerRightTop,
	kCornerRightBottom,
	kCornerLeftBottom
};

enum TimeFlags
{
	kDoFade = 1 << 0,
	kDoStandby = 1 << 1,
	kDoSuspend = 1 << 2,
	kDoOff = 1 << 3
};

#endif
