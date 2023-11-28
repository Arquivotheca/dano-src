
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <kernel/OS.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_attr.h>
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <AppDefs.h>
#include <File.h>
#include <List.h>
#include <Path.h>
#include <Node.h>
#include <parsedate.h>
#include <sys/time.h>
#include <SymLink.h>
#include <TypeConstants.h>
#include <Directory.h>
#include <Drivers.h>

#include "db.h"
#include "master.h"
#include "config.h"
#include "util.h"
#include "versioncache.h"

const char *workingVersion=NULL;
const char *installby=NULL;
const char *flashSize=NULL;
const char *doRename=NULL;
bool fReboot = false;
bool fRestart = false;
bool fFlush = false;
bool fFreeze = false;
bool fRedial = false;
extern char **environ;
const char *wagnerSig = "application/x-vnd.Web";

FILE *_stdin = NULL;
FILE *_stdout = NULL;

inline short nybble(char c)
{
	return (c >= '0' && c <= '9') ? c - '0' : c - 'A' + 10;
}

const char *strtime(time_t when)
{
	static char str[25];

	/* cut the trailing '\n' off of the ctime() string */
	strncpy(str,ctime(&when),24);
	return str;
}

void clean_name(const char *src, char *dst)
{
	char c;
	do {
		c = *src++;
		if (c == '%') {
			*dst++ = (nybble(src[0]) << 4) | nybble(src[1]);
			src += 2;
		} else
			*dst++ = c;
	} while (c);
}

void wipe_version(const char *name, bool wipeParams=true)
{
	BPath p,root(CACHE_PATHNAME,name);
	mkdir(root.Path(),0777);

	arglist args;
	args.add("/bin/rm");
	args.add("-rf");
	p.SetTo(root.Path(),"fullupdate"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"images"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"delta"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"bootstrap.js"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"fullupdate.zip"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata.zip"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"versionarchive.zip"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/pakdict"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/unpakdict"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/symbol.match"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/symbol.db"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/zrecover.configured"); args.add(strdup(p.Path()),true);
	p.SetTo(root.Path(),"deltadata/updt-remove-list"); args.add(strdup(p.Path()),true);
	if (wipeParams) {
		p.SetTo(root.Path(),"deltadata/dsaparams"); args.add(strdup(p.Path()),true);
		p.SetTo(root.Path(),"deltadata/dsakey.pub"); args.add(strdup(p.Path()),true);
		p.SetTo(root.Path(),"deltadata/dsakey.priv"); args.add(strdup(p.Path()),true);
	}

	if (simple_exec(args.count(),args))
		error_die("could not remove old files");
	
	p.SetTo(root.Path(),"deltadata");
	mkdir(p.Path(),0777);
	p.SetTo(root.Path(),"versionarchive");
	mkdir(p.Path(),0777);
	p.SetTo(root.Path(),"versionarchive/install");
	mkdir(p.Path(),0777);
	p.SetTo(root.Path(),"versionarchive/spec");
	mkdir(p.Path(),0777);
}

void do_info(const char *name)
{
	version_rec r;
	read_version_record(name,&r,true);

	filelist newVersionFiles;
	BPath newTree(CACHE_PATHNAME,name);
	newTree.Append("versionarchive/install");
	BPath oldTree(CACHE_PATHNAME,r.derivedFrom);
	if (r.checkinTime != -1) {
		if (db_get_version(name,0))
			output("info","verified false");
		else
			output("info","verified true");
	}
	
	char **dclasses = db_get_device_classes();
	if (dclasses) {
		char **p = dclasses;
		while (*p) {
			output("info","deviceclass %s",*p);
			p++;
		}
	}

//	printf("status: working\n"); fflush(stdout);
//	add_all_files(newTree.Path(),newVersionFiles,false);
//	printf("info: file-count %d\n",newVersionFiles.count()); 
}

void revert_version_rec(void *userData)
{
	version_rec *r = (version_rec*)userData;
	write_version_record(r);
}

void generate_bootstrap(const char *templ, const char *output, const char *archive, version_rec &)
{
	status("generating bootstrap");
	if (!cfg["binder_progress"])
		error_die("config file corrupt: could not find binder_progress");
	
	char *str;
	struct stat st;
	char buf[1024];
	FILE *inTemplate = fopen(templ,"r");
	if (!inTemplate) error_die("could not find template %s",templ);
	FILE *outBootstrap = fopen(output,"w");
	if (!outBootstrap) error_die("could not write bootstrap script");
	while ((str = fgets(buf,1024,inTemplate)) && (str[0] == '/') && (str[1] == '/'))
		fprintf(outBootstrap,"%s",str);
	lstat(archive,&st);
	// fprintf(outBootstrap,"var _updtG_needReboot = %s;\n",r.reboot?"true":"false");
	// fprintf(outBootstrap,"var _updtG_needRedial = %s;\n",r.redial?"true":"false");
	// fprintf(outBootstrap,"var _updtG_needRestart = %s;\n",r.restart?"true":"false");
	// fprintf(outBootstrap,"var _updtG_needFreeze = %s;\n",r.freeze?"true":"false");
	// fprintf(outBootstrap,"var _updtG_needFlush = %s;\n",r.flush?"true":"false");
	fprintf(outBootstrap,"var _updtG_dsaParams = '/boot/%s';\n",cfg["dsaparams_location"]);
	fprintf(outBootstrap,"var _updtG_dsaKey = '/boot/%s';\n",cfg["dsakey_location"]);
	fprintf(outBootstrap,"var _updtG_progress = %s;\n",cfg["binder_progress"]);
	fprintf(outBootstrap,"var _updtG_removeList = 'updt-remove-list';\n");
	fprintf(outBootstrap,"var _updtG_archiveSize = %ld;\n",(int32)st.st_size);
	fprintf(outBootstrap,"var _updtG_excludeFiles = new Array(_updtG_removeList);\n");
	if (str) fprintf(outBootstrap,"%s",str);
	while ((str = fgets(buf,1024,inTemplate)))
		fprintf(outBootstrap,"%s",str);
	fclose(inTemplate);
	fclose(outBootstrap);
}

int do_finish(const char *newVersionName)
{
	char buf[1024];
	version_rec r;
	BPath
		oldRoot,newRoot,
		oldTree,newTree,
		oldLBXList,newLBXList,
		oldVersionArchive,newVersionArchive,
		oldDeltaData,newDeltaData,
		oldSymbolDB,newSymbolDB,
		oldSymbolMatch,newSymbolMatch,
		oldCodeDict,newCodeDict,
		oldUnpackDict,newUnpackDict,
		oldMagicSymbols,newMagicSymbols,
		oldCrushedTree,newCrushedTree,
		oldDSAParams,newDSAParams,
		oldDSAPubKey,newDSAPubKey,
		oldDSAPrivKey,newDSAPrivKey,
		newDSAKey,
		oldAllRemoveFiles,newAllRemoveFiles,
		newRemoveFiles,
		newFullRemoveFiles,
		newThisDeltaDir,
		newThisDeltaRemoveFiles,
		newDeltaBootstrap,
		newFullBootstrap,
		newDeltaDir,

		newImages,

		newVersionArchiveZip,
		newDeltaDataZip,
		newFullUpdateZip,
		newDeltaUpdateZip;

	char *oldVersionName = NULL;
	read_version_record(newVersionName,&r);
	if (r.derivedFrom[0]) oldVersionName = r.derivedFrom;
	r.finishTime = -1;
	r.checkinTime = -1;
	r.reboot = false;
	r.redial = false;
	r.restart = false;
	r.flush = false;
	r.freeze = false;
	write_version_record(&r);
	
	filelist oldVersionFiles, newVersionFiles, added, removed, changed;

	newRoot.SetTo(CACHE_PATHNAME,newVersionName);
	if (!isdir(newRoot.Path()))
		error_die("version '%s' not found",newVersionName);

	newVersionArchive.SetTo(newRoot.Path(),"versionarchive");
	newTree.SetTo(newVersionArchive.Path(),"install");
	newMagicSymbols.SetTo(newVersionArchive.Path(),"spec/magic_symbols.txt");
	if (!isdir(newVersionArchive.Path()) ||
		!isdir(newTree.Path()))
		error_die("new version '%s' does not have expected structure",newVersionName);

	newImages.SetTo(newRoot.Path(),"images");
	newDeltaData.SetTo(newRoot.Path(),"deltadata");
	newCrushedTree.SetTo(newRoot.Path(),"fullupdate");
	newFullBootstrap.SetTo(newRoot.Path(),"bootstrap.js");

	newSymbolDB.SetTo(newDeltaData.Path(),"symbol.db");
	newSymbolMatch.SetTo(newDeltaData.Path(),"symbol.match");
	newCodeDict.SetTo(newDeltaData.Path(),"pakdict");
	newUnpackDict.SetTo(newDeltaData.Path(),"unpakdict");
	newDSAKey.SetTo(newDeltaData.Path(),"dsakey");
	newDSAPubKey.SetTo(newDeltaData.Path(),"dsakey.pub");
	newDSAPrivKey.SetTo(newDeltaData.Path(),"dsakey.priv");
	newDSAParams.SetTo(newDeltaData.Path(),"dsaparams");
	newAllRemoveFiles.SetTo(newDeltaData.Path(),"updt-remove-list");
	newFullRemoveFiles.SetTo(newCrushedTree.Path(),"updt-remove-list");
	newRemoveFiles.SetTo(newRoot.Path(),"updt-remove-list");

	newFullUpdateZip.SetTo(newRoot.Path(),"fullupdate.zip");
	newVersionArchiveZip.SetTo(newRoot.Path(),"versionarchive.zip");
	newDeltaDataZip.SetTo(newRoot.Path(),"deltadata.zip");

	if (oldVersionName) {
		status("fetching old version '%s'",oldVersionName);
		if (get_version(oldVersionName,&oldRoot,AR_VERSIONARCHIVE|AR_DELTADATA|AR_FULLUPDATE) != 0)
			error_die("old version '%s' unknown",oldVersionName);
	
		oldVersionArchive.SetTo(oldRoot.Path(),"versionarchive");
		oldTree.SetTo(oldVersionArchive.Path(),"install");
		oldMagicSymbols.SetTo(oldVersionArchive.Path(),"spec/magic_symbols.txt");
		oldDeltaData.SetTo(oldRoot.Path(),"deltadata");
		oldAllRemoveFiles.SetTo(oldDeltaData.Path(),"updt-remove-list");
		oldSymbolDB.SetTo(oldDeltaData.Path(),"symbol.db");
		oldSymbolMatch.SetTo(oldDeltaData.Path(),"symbol.match");
		oldCodeDict.SetTo(oldDeltaData.Path(),"pakdict");
		oldUnpackDict.SetTo(oldDeltaData.Path(),"unpakdict");
		oldCrushedTree.SetTo(oldRoot.Path(),"fullupdate");
		oldDSAPubKey.SetTo(oldDeltaData.Path(),"dsakey.pub");
		oldDSAPrivKey.SetTo(oldDeltaData.Path(),"dsakey.priv");
		oldDSAParams.SetTo(oldDeltaData.Path(),"dsaparams");

		newDeltaDir.SetTo(newRoot.Path(),"delta");
		newThisDeltaDir.SetTo(newDeltaDir.Path(),oldVersionName);
		newDeltaUpdateZip.SetTo(newThisDeltaDir.Path(),"archive.zip");
		newDeltaBootstrap.SetTo(newThisDeltaDir.Path(),"bootstrap.js");
		rm(newDeltaDir.Path());
		mkdir(newDeltaDir.Path(),0777);
		mkdir(newThisDeltaDir.Path(),0777);
	}

	status("cleaning up old files");
	wipe_version(newVersionName,false);

	bool incremental = false;
//	bool needToCrush = changed.count_elf() || added.count_elf();
	if (oldVersionName) {
		/* We need to copy over the symbol dictionaries before starting to crush */
		status_t err = 0;
		BPath from,to;

		err = copyfile(oldSymbolDB.Path(),newSymbolDB.Path());
		if (!err) err = copyfile(oldSymbolMatch.Path(),newSymbolMatch.Path());
		if (!err) err = copyfile(oldCodeDict.Path(),newCodeDict.Path());
		if (!err) err = copyfile(oldUnpackDict.Path(),newUnpackDict.Path());
		if (!err) err = copyfile(oldDSAParams.Path(),newDSAParams.Path());
		if (!err) err = copyfile(oldDSAPubKey.Path(),newDSAPubKey.Path());
		if (!err) err = copyfile(oldDSAPrivKey.Path(),newDSAPrivKey.Path());
		if (!err) err = copyfile(oldSymbolMatch.Path(),newSymbolMatch.Path());
		if (!err) err = copyfile(oldCrushedTree.Path(),newCrushedTree.Path(),"-r");
		if (!err) incremental = true;
		else warning("could not copy over delta data from previous version");
		
		if (!incremental) {
			warning("no crushing history is available -- making an initial revision");
			rm(newCrushedTree.Path());
			rm(newSymbolDB.Path());
			rm(newSymbolMatch.Path());
			rm(newCodeDict.Path());
			rm(newUnpackDict.Path());
		}
	}

	status_t err;
	int32 count;
	arglist args;
	BPath src,dst;
	filelist realNewFiles,realOldFiles,tmp1,tmp2;
	filelist newfiles,crushfiles,finalcrushfiles;
	filelist newLBXfiles,oldLBXfiles,addedLBX,removedLBX,changedLBX;
	filelist nonLBXadded,nonLBXremoved,nonLBXchanged;
	filelist newLBXarchives,oldLBXarchives,addedArchives,removedArchives,changedArchives,toLBXify;

	if (incremental) {
		status("building '%s' as an upgrade from '%s'",newVersionName,oldVersionName);
		add_all_files(oldTree.Path(),oldVersionFiles);
		status("old version '%s' sniffed (%d files)",oldVersionName,oldVersionFiles.count());
	} else {
		status("building '%s' as an initial revision",newVersionName);
		r.reboot = true;
		r.freeze = true;
	}

	add_all_files(newTree.Path(),newVersionFiles);
	status("new version '%s' sniffed (%d files)",newVersionName,newVersionFiles.count());

	status("analyzing LBX archive composition");

	oldLBXList.SetTo(oldTree.Path(),"beos/etc/files-in-lbx.txt");
	newLBXList.SetTo(newTree.Path(),"beos/etc/files-in-lbx.txt");

	if (incremental) {
		if ((err = file_to_filelist(oldLBXList.Path(),oldLBXfiles,false)))
			error_die("could not find old list of LBX files (beos/etc/files-in-lbx.txt)");
	}

	if ((err = file_to_filelist(newLBXList.Path(),newLBXfiles,false)))
		error_die("could not find new list of LBX files (beos/etc/files-in-lbx.txt)");

	filelist::make_diffs(oldVersionFiles,newVersionFiles,nonLBXadded,nonLBXremoved,nonLBXchanged);
	filelist::subtract(oldVersionFiles,oldLBXfiles,realOldFiles);
	filelist::subtract(newVersionFiles,newLBXfiles,realNewFiles);

	status("differencing LBX archives");
	filelist::make_diffs(oldLBXfiles,newLBXfiles,addedLBX,removedLBX,changedLBX);
	changedLBX.clear();
	filelist::intersection(newLBXfiles,nonLBXchanged,changedLBX);

	nonLBXadded.clear(); nonLBXremoved.clear(); nonLBXchanged.clear();
	filelist::make_diffs(realOldFiles,realNewFiles,nonLBXadded,nonLBXremoved,nonLBXchanged);

	if (added.count()) status("%d files added (%d binaries)",nonLBXadded.count(),nonLBXadded.count_elf());
	if (removed.count()) status("%d files removed (%d binaries)",nonLBXremoved.count(),nonLBXremoved.count_elf());
	if (changed.count()) status("%d files changed (%d binaries)",nonLBXchanged.count(),nonLBXchanged.count_elf());

	filelist::lbxify(oldLBXfiles,oldLBXarchives);
	filelist::lbxify(newLBXfiles,newLBXarchives);
	filelist::make_diffs(oldLBXarchives,newLBXarchives,addedArchives,removedArchives,changedArchives);
	changedArchives.clear();
	filelist::lbxify(changedLBX,changedArchives);

	if (addedArchives.count()) status("%d LBX archives added",addedArchives.count());
	if (removedArchives.count()) status("%d LBX archives removed",removedArchives.count());
	if (changedArchives.count()) status("%d LBX archives changed",changedArchives.count());

	filelist::merge(nonLBXadded,addedArchives,false,added);
	filelist::merge(nonLBXremoved,removedArchives,false,removed);
	filelist::merge(nonLBXchanged,changedArchives,false,changed);
	filelist::merge(addedArchives,changedArchives,false,toLBXify);
	filelist::merge(nonLBXadded,nonLBXchanged,true,crushfiles);

	if (!added.count() && !changed.count() && !removed.count())
		error_die("no changes from previous version");

	status("preparing new install tree");

	if ((err = mkdir(newCrushedTree.Path(),0777))) {
		if(errno != EEXIST) {
			error_die("could not create the new directory");
		}
	}
	if ((err = chdir(newTree.Path())))
		error_die("could not cd to the new install directory");

	args.clear();
	args.add(CP_PATHNAME);
	args.add("-Pdp");
	for (int32 i=0;i<nonLBXadded.count();i++) {
		if (nonLBXadded[i].isDir()) {
			dst.SetTo(newCrushedTree.Path(),nonLBXadded[i].filename);
			mkdir(dst.Path(),0777);
		} else if (!nonLBXadded[i].isElf()) {
			args.add(nonLBXadded[i].filename);
		}
	}
//	nonLBXadded.make_args(args,filelist::ONLY_NON_ELF);
	for (int32 i=0;i<nonLBXchanged.count();i++) {
		if (nonLBXchanged[i].isDir()) {
			dst.SetTo(newCrushedTree.Path(),nonLBXchanged[i].filename);
			mkdir(dst.Path(),0777);
		} else if (!nonLBXchanged[i].isElf()) {
			args.add(nonLBXchanged[i].filename);
		}
	}
//	nonLBXchanged.make_args(args,filelist::ONLY_NON_ELF);
	args.add(newCrushedTree.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not integrate new/changed files");

	if (toLBXify.count()) {
		status("performing LBX compression (%d archives)",toLBXify.count());
	
		const char *rotateStr = cfg["rotate_lbx"];
		bool rotate = (rotateStr && !strcasecmp(rotateStr,"true"));
		for (int32 i=0;i<toLBXify.count();i++) {
			args.clear();
			args.add(MKLBX_PATHNAME);
			args.add("--output");
			dst.SetTo(newCrushedTree.Path(),toLBXify[i].filename);
			printf("dst = '%s','%s','%s'\n",newCrushedTree.Path(),toLBXify[i].filename,dst.Path());
			args.add(dst.Path());
			args.add("--verbose");
			if (rotate) args.add("--rotate");
			src.SetTo(toLBXify[i].filename);
			src.GetParent(&src);
			newLBXfiles.make_args(args,filelist::ALL,&src);
			if ((err = simple_exec(args.count(),args)))
				error_die("could not create LBX archive");
		}
	}
	
//	if ((err = chdir(newCrushedTree.Path())))
//		error_die("could not cd to the crushing directory");
	
	if (crushfiles.count()) {
		status("adding new symbols to database");
		args.clear();
		args.add(ELFHANDLER_PATHNAME);
		args.add("-addsymbols");
		crushfiles.make_args(args);
		args.add("-db");
		args.add(newSymbolDB.Path());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not add new symbols to crusher database");
	
		status("marking magic symbols");
		args.clear();
		args.add(ELFHANDLER_PATHNAME);
		args.add("-markmagicsymbols");
		args.add("-db");
		args.add(newSymbolDB.Path());
		args.add("-magic");
		args.add(newMagicSymbols.Path());
		if ((err = exec_to_filelist(args.count(),args,newfiles)))
			error_die("Could not mark magic symbols!");

		newfiles.remove_duplicates();
		if ((count=newfiles.count())) status("%ld new files added to crushing list",count);
	
		status("marking used symbols");
		args.clear();
		args.add(ELFHANDLER_PATHNAME);
		args.add("-markusedsymbols");
		crushfiles.make_args(args);
		args.add("-db");
		args.add(newSymbolDB.Path());
		if ((err = exec_to_filelist(args.count(),args,newfiles)))
			error_die("could not mark used symbols");

		newfiles.remove_duplicates();
		if (newfiles.count()-count) status("%ld new files added to crushing list",newfiles.count()-count);
/*
		if (newfiles.count()) {
			status("overlaying binaries to recrush");
			for (int32 i=0;i<newfiles.count();i++) {
				src.SetTo(newTree.Path(),newfiles[i].filename);
				dst.SetTo(newCrushedTree.Path(),newfiles[i].filename);
				if (copyfile(src.Path(),dst.Path()))
					error_die("could not overlay uncrushed binaries");
			}
		}
*/	
		filelist::merge(crushfiles,newfiles,false,finalcrushfiles);
	
		if ((err = chdir(newTree.Path())))
			error_die("Could not cd to the new install directory");
	
		status("crushing binaries (%d files)",finalcrushfiles.count());
	
		args.clear();
		args.add(ELFHANDLER_PATHNAME);
		args.add("-crushsymbols");
		finalcrushfiles.make_args(args);
		args.add("-out_tree");
		args.add(newCrushedTree.Path());
		args.add("-db");
		args.add(newSymbolDB.Path());
		args.add("-match");
		args.add(newSymbolMatch.Path());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not crush binaries");
	
		if ((err = chdir(newCrushedTree.Path())))
			error_die("could not cd to the crushing directory");
	
		if (!incremental) {
			status("building initial code compression dictionary");
			// need to build code dictionary
			args.clear();
			args.add(BEFIRST_PATHNAME);
			args.add(newCodeDict.Path());
			args.add("-merge");
			finalcrushfiles.make_args(args);
			if ((err = simple_exec(args.count(),args)))
				error_die("could not create code packing dictionary");
	
			args.clear();
			args.add(BEFIRST_PATHNAME);
			args.add(newCodeDict.Path());
			args.add("-freeze");
			args.add(newUnpackDict.Path());
			args.add("-cut");
			args.add("1");
			if ((err = simple_exec(args.count(),args)))
				error_die("could not create code unpacking dictionary");
		}
	
		if ((err = copyfile(newUnpackDict.Path(),UNPACK_DICT_PATHNAME)))
			error_die("could not copy over code unpacking dictionary");
	
		status("making final code compression pass (%d files)",finalcrushfiles.count());

		args.clear();
		args.add(ELF2CELF_PATHNAME);
		args.add("-batch");
		args.add(newCodeDict.Path());
		finalcrushfiles.make_args(args);
		if ((err = simple_exec(args.count(),args)))
			error_die("could not finish compression pass");
	}
	
	filelist diffarchive;
	{
		filelist tmplist;
		filelist::merge(added,changed,false,tmplist);
		filelist::merge(tmplist,finalcrushfiles,false,diffarchive);
	}

	BPath p,zrecover,configuredZrecover,settings;
	settings.SetTo(newVersionArchive.Path(),"spec/recovery_settings");
	zrecover.SetTo(newVersionArchive.Path(),"spec/zrecover");
	configuredZrecover.SetTo(newDeltaData.Path(),"zrecover.configured");

	status("configuring recovery image");

	if ((err = copyfile(zrecover.Path(),configuredZrecover.Path())))
		error_die("could not copy zrecover image");

	char line[1024];
	int input=-1,output=open("/dev/null",O_WRONLY);
	args.clear();
	args.add(EDITRECOVERY_PATHNAME);
	args.add(configuredZrecover.Path());
	thread_id thid = do_exec(args.count(),args,&input,&output);
	if (thid < 0) error_die("could not configure recovery image");

	FILE *inf = fopen(settings.Path(),"r");
	FILE *outf = fdopen(input,"w");
	input = -1;
	if (!inf) error_die("could not open recovery settings");
	if (!outf) error_die("could not open stdin for settings tool");

	while (fgets(line,1024,inf)) {
		printf("piping: %s",line);
		fwrite(line,strlen(line),1,outf);
	}
	fclose(inf);

	char param='X';
	int32 paramNum=0;
	settings.SetTo(newDeltaData.Path(),"dsaparams");
	inf = fopen(settings.Path(),"r");
	if (!inf) error_die("could not open DSA params file");
	while (fgets(line,1024,inf)) {
		if (line[0] == '\t') {
			printf("piping: DSA_PARAM_%c_%02ld=%s",param,paramNum,line+1);
			fprintf(outf,"DSA_PARAM_%c_%02ld=%s",param,paramNum,line+1);
			paramNum++;
		} else if (line[1] == ':') {
			param = toupper(line[0]);
			paramNum = 0;
		}
	}
	fclose(inf);

	paramNum = 0;
	settings.SetTo(newDeltaData.Path(),"dsakey.pub");
	inf = fopen(settings.Path(),"r");
	if (!inf) error_die("could not open DSA key file");
	while (fgets(line,1024,inf)) {
		if (line[0] == '\t') {
			printf("piping: DSA_PUB_KEY_%02ld=%s",paramNum,line+1);
			fprintf(outf,"DSA_PUB_KEY_%02ld=%s",paramNum,line+1);
			paramNum++;
		}
	}
	fclose(inf);
	fclose(outf);
	
	wait_for_thread(thid,&err);
	if (err) error_die("could not configure recovery image");

	status("installing DSA params and key");
	if (!cfg["dsaparams_location"] || !cfg["dsakey_location"])
		error_die("config file corrupt: connot find DSA params/key location settings");

	p.SetTo(newCrushedTree.Path(),cfg["dsaparams_location"]);
	if (copyfile(newDSAParams.Path(),p.Path()))
		error_die("could not install DSA params");

	sprintf(buf,"%s.pub",cfg["dsakey_location"]);
	p.SetTo(newCrushedTree.Path(),buf);
	if (copyfile(newDSAPubKey.Path(),p.Path()))
		error_die("could not install DSA key");
	
	/*	Compare this recovery image with the old one, and add stuff to the diff to
		update the recovery partition, if neccessary. */
	
	filelist oldAllRemoved,newAllRemoved;

	if (removed.write(newRemoveFiles.Path()))
		error_die("could not write delta update remove file list");

	if (incremental) {
		if (oldAllRemoved.read(oldAllRemoveFiles.Path()))
			error_die("could not read old comprehensive remove file list");
		filelist::merge(oldAllRemoved,removed,false,newAllRemoved);
	} else
		newAllRemoved.copy(removed);

	if (newAllRemoved.write(newAllRemoveFiles.Path()))
		error_die("could not write new comprehensive remove file list");

	removed.clear();
	filelist::subtract(newAllRemoved,realNewFiles,removed);

	if (removed.write(newFullRemoveFiles.Path()))
		error_die("could not write full update remove file list");

	// need to write the version record out before we compress the delta data
	version_rec oldRec = r;
	r.finishTime = time(NULL);
	if (installby) r.installByTime = parsedate(installby,r.finishTime);
	r.reboot = fReboot;
	r.redial = fRedial;
	r.restart = fRestart;
	r.flush = fFlush;
	r.freeze = fFreeze;
	write_version_record(&r);

	if (!cfg["version_tag"])
		error_die("config file corrupt: could not find version_tag");
	
	p.SetTo(newCrushedTree.Path(),cfg["version_tag"]);
	int outVersionTag = open(p.Path(),O_CREAT|O_TRUNC|O_WRONLY);
	write(outVersionTag,newVersionName,strlen(newVersionName));
	close(outVersionTag);
	
	death_hook _hook1(revert_version_rec,&oldRec);

	if (incremental) {
		status("creating delta package (%d files)",diffarchive.count());

		mkdir(newDeltaDir.Path(),0777);
		rm(newThisDeltaDir.Path());
		mkdir(newThisDeltaDir.Path(),0777);
	
		args.clear();
		args.add(ZIP_PATHNAME);
		args.add("-y9");
		args.add(newDeltaUpdateZip.Path());
		diffarchive.make_args(args);
		args.add(cfg["version_tag"]);
		if ((err = simple_exec(args.count(),args)))
			error_die("could not create delta update package");

		if ((err = chdir(newRoot.Path())))
			error_die("could not cd to the version root directory");
			
		args.clear();
		args.add(ZIP_PATHNAME);
		args.add("-y9");
		args.add(newDeltaUpdateZip.Path());
		args.add(newRemoveFiles.Leaf());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not add remove files to delta update package");
		rm(newRemoveFiles.Path());

		status("signing delta update package");
	
		args.clear();
		args.add(DSASIG_PATHNAME);
		args.add("sign");
		args.add(newDSAParams.Path());
		args.add(newDSAKey.Path());
		args.add(newDeltaUpdateZip.Path());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not cryptographically sign delta update package");

		generate_bootstrap(DELTA_BOOTSTRAP_PATHNAME,newDeltaBootstrap.Path(),newDeltaUpdateZip.Path(),r);
	} else
		status("skipping delta update archive, as this is an initial version");

	if ((err = chdir(newCrushedTree.Path())))
		error_die("could not cd to the version root directory");

	status("creating full update package");
	args.clear();
	args.add(ZIP_PATHNAME);
	args.add("-ry9");
	args.add(newFullUpdateZip.Path());
	args.add(".");
	if ((err = simple_exec(args.count(),args)))
		error_die("could not create full update archive");

	status("signing full update package");
	args.clear();
	args.add(DSASIG_PATHNAME);
	args.add("sign");
	args.add(newDSAParams.Path());
	args.add(newDSAKey.Path());
	args.add(newFullUpdateZip.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not cryptographically sign full update package");

	generate_bootstrap(FULL_BOOTSTRAP_PATHNAME,newFullBootstrap.Path(),newFullUpdateZip.Path(),r);

	if ((err = chdir(newVersionArchive.Path())))
		error_die("could not cd to the versioning archive directory");

	status("creating versioning archive");
	
	args.clear();
	args.add(ZIP_PATHNAME);
	args.add("-ry9");
	args.add(newVersionArchiveZip.Path());
	args.add(".");
	if ((err = simple_exec(args.count(),args)))
		error_die("could not create versioning archive");

	/*	Generate the script */
	

	return 0;
}

void undo_checkin(void *userData)
{
	char buf1[256],buf2[256];
	version_rec *r = (version_rec*)userData;
	if (doRename) {
		sprintf(buf1,CACHE_PATHNAME "/%s",doRename);
		sprintf(buf2,CACHE_PATHNAME "/%s",workingVersion);
		rename(buf1,buf2);
		strcpy(r->versionID,workingVersion);
	}
	r->checkinTime = -1;
	
	write_version_record(r);
}

int do_checkin(const char *versionName)
{
	version_rec r;
	struct version_info info;
	char installByBuf[256],checkoutBuf[256],checkinBuf[256],finishBuf[256];
	status_t err;

	int32 dieHook = add_die_hook(undo_checkin,&r);

	read_version_record(versionName,&r);
	if (doRename) {
		strcpy(r.versionID,doRename);
		sprintf(finishBuf,CACHE_PATHNAME "/%s",versionName);
		sprintf(checkoutBuf,CACHE_PATHNAME "/%s",doRename);
		rename(finishBuf,checkoutBuf);
	}
	r.checkinTime = time(NULL);
	if (installby) r.installByTime = parsedate(installby,r.checkinTime);
	write_version_record(&r);

	BPath root(CACHE_PATHNAME,r.versionID);
	BPath deltaData(root.Path(),"deltadata");
	BPath deltaDataZip(root.Path(),"deltadata.zip");

	status("packaging version info",versionName);

	strcpy(finishBuf,strtime(r.finishTime));
	strcpy(installByBuf,strtime(r.installByTime));
	strcpy(checkoutBuf,strtime(r.checkoutTime));
	strcpy(checkinBuf,strtime(r.checkinTime));
	info.versionid = r.versionID;
	info.installby = installByBuf;
	info.checkout_time = checkoutBuf;
	info.finish_time = finishBuf;
	info.checkin_time = checkinBuf;
	info.lastversionid = r.derivedFrom;

	if ((err = chdir(deltaData.Path())))
		error_die("could not cd to the delta data directory");

	rm(deltaDataZip.Path());

	arglist args;
	args.add(ZIP_PATHNAME);
	args.add("-ry9");
	args.add(deltaDataZip.Path());
	args.add(".");
	if ((err = simple_exec(args.count(),args)))
		error_die("could not create delta data archive");

	status("uploading version '%s' to server",versionName);
	if ((err = cache_put_version(&info)))
		error_die("could not upload version to server");

	if (r.derivedFrom[0]) {
		int flags;

		status("uploading default upgrade path",versionName,0);
		flags = 0;
		if (fReboot) flags |= UF_REBOOT;
		if (fFlush) flags |= UF_FLUSH;
		if (fFreeze) flags |= UF_FREEZE;
		if (fRedial) flags |= UF_REDIAL;
		if (fRestart) flags |= UF_RESTART;
		if (db_put_upgrade_path(versionName,r.derivedFrom,flags))
			error_die("could not upload version upgrade path '%s' -> '%s'",r.derivedFrom,versionName);
	}

	remove_die_hook(dieHook);

	return 0;
}

const char *bookmarkSig = "application/x-vnd.Be-bookmark";

void remove_tracker_interface(const char *name)
{
	BPath userLand("/boot/home/Desktop/");
	userLand.Append("Mastering Station");
	userLand.Append("BeIA Versions");
	userLand.Append(name);
	rm(userLand.Path());
}

void create_tracker_interface(const char *name, bool open=true)
{
	status_t err;
	char buf[1024];
	BPath p,p2,data(CACHE_PATHNAME,name);
	BPath userLand("/boot/home/Desktop/");

	userLand.Append("Mastering Station");
	mkdir(userLand.Path(),0777);

	userLand.Append("BeIA Versions");
	mkdir(userLand.Path(),0777);

	userLand.Append(name);
	if (!isdir(userLand.Path())) rm(userLand.Path());
	mkdir(userLand.Path(),0777);

	p.SetTo(data.Path(),"versionarchive/install");
	userLand.Append("Install Tree");
	rm(userLand.Path());
	symlink(p.Path(),userLand.Path());

	p.SetTo(data.Path(),"versionarchive/spec");
	userLand.GetParent(&userLand);
	userLand.Append("Build Specifications");
	rm(userLand.Path());
	symlink(p.Path(),userLand.Path());

	userLand.GetParent(&userLand);
	userLand.Append("Control Panel");
	rm(userLand.Path());
	BFile bookmark(userLand.Path(),O_CREAT|O_WRONLY);
	if (!cfg["admin_panel_url"]) error_die("admin tool panel URL not configured");
	strcpy(buf,cfg["admin_panel_url"]);
	clean_name(name,buf+strlen(buf));
	bookmark.WriteAttr("META:url",B_STRING_TYPE,0,buf,strlen(buf)+1);
	bookmark.WriteAttr("BEOS:TYPE",'MIMS',0,bookmarkSig,strlen(bookmarkSig)+1);
	bookmark.WriteAttr("BEOS:PREF_APP",B_MIME_TYPE,0,wagnerSig,strlen(wagnerSig)+1);
	bookmark.Unset();

	if (open) {
		userLand.GetParent(&userLand);
		arglist args;
		args.add("/system/Tracker");
		args.add(userLand.Path());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not point Tracker at version '%s'",name);
	}
}

void do_create(const char *name)
{
	status_t err;
	BPath p,p2,data(CACHE_PATHNAME,name);
	mkdir(data.Path(),0777);
	p.SetTo(data.Path(),"versionarchive/spec/recovery_settings");
	data.Append("deltadata");

	status("creating new initial version '%s'",name);

	wipe_version(name);

	version_rec r;
	create_version_record(&r,name);
	write_version_record(&r);

	if (copyfile(DEFAULTRECOVERYSETTINGS_PATHNAME,p.Path()))
		error_die("could not create initial recovery settings");

	arglist args;

	p2.SetTo("/boot/station/vendor");
	mkdir(p2.Path(),0777);
	p2.Append("dsaparams");
	p.SetTo(data.Path(),"dsaparams");
	if (!isfile(p2.Path())) {
		status("no DSA params found; generating some");
		args.add(DSASIG_PATHNAME);
		args.add("mkparams");
		args.add("1024");
		args.add(p2.Path());
		if ((err = simple_exec(args.count(),args)))
			error_die("could not create new DSA params");
	} else
		status("using pregenerated cryto params");

	if ((err = copyfile(p2.Path(),p.Path())))
		error_die("could not obtain DSA params");

	p2.SetTo(data.Path(),"dsakey");
	status("generating new cryto key");

	args.clear();
	args.add(DSASIG_PATHNAME);
	args.add("mkkey");
	args.add(p.Path());
	args.add(p2.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not create new DSA key");

	create_tracker_interface(name);
}

int do_cache(const char *versionName)
{
	status("checking out version '%s'",versionName);
	int32 flags = cache_get_version(versionName,AR_ALL);
	if (flags == ERR_NOVERSION)
		error_die("no version '%s' exists",versionName);
	if (flags == ERR_DUPVERSIONS)
		error_die("more than one version '%s' exists",versionName);
	if (flags < 0)
		error_die("failed to checkout version '%s'",versionName);
	if ((flags & AR_ALL) != AR_ALL)
		error_die("failed to fetch all components of version '%s'",versionName);

	create_tracker_interface(versionName,false);
	
	return 0;
}

int do_delete(const char *versionName)
{
	int err;
	status("removing version record for '%s' from database",versionName);
	if ((err = db_delete_version(versionName)))
		error_die("could not delete record from database");
	return 0;
}

int do_uncache(const char *versionName)
{
	status("removing local project '%s'",versionName);
	remove_tracker_interface(versionName);
	BPath versionPath(CACHE_PATHNAME,versionName);
	rm(versionPath.Path());
	return 0;
}

int do_checkout(const char *checkoutName, const char *versionName)
{
	do_cache(versionName);

	BPath oldVersionPath(CACHE_PATHNAME,versionName);
	BPath newVersionPath(CACHE_PATHNAME,checkoutName);
	BPath tmp1,tmp2;

	status("clearing space for new version '%s'",checkoutName);
	rm(newVersionPath.Path());
	if (mkdir(newVersionPath.Path(),0777))
		error_die("failed to create new versioning archive directory");

	oldVersionPath.Append("versionarchive");
	newVersionPath.Append("versionarchive");

	status("creating new version '%s'",checkoutName);
	if (copyfile(oldVersionPath.Path(),newVersionPath.Path(),"-r"))
		error_die("failed to create new version '%s'",checkoutName);

	version_rec r;
	create_version_record(&r,checkoutName,versionName);
	write_version_record(&r);

	create_tracker_interface(checkoutName);
	return 0;
}

int64 get_device_flash_size(const char *deviceID)
{
	int64 deviceSize=0;
	printf("get_device_flash_size '%s'\n",deviceID);
	if (!strcmp(deviceID,"_local")) {
		if (!cfg["flash_device"]) error_die("no flash device configured");
		int fd = open(cfg["flash_device"],O_RDONLY);
		if (fd < 0)
			error_die("could not open flash device");
		if (ioctl(fd,B_GET_DEVICE_SIZE,&deviceSize))
			error_die("could not query flash device for size");
		close(fd);
	} else {
		deviceSize = devclass_getimgsize(deviceID);
		if (deviceSize <= 0) error_die("unknown device class '%s'",deviceID);
	}
	return deviceSize;
}

void do_images(const char *name, const char *deviceID)
{
	status_t err;
	struct stat sb;
	BPath zbeos,image,zrecover,configuredZrecover,tree,p,root(CACHE_PATHNAME,name);
	BPath dsaParams,dsaKey;

	int32 flags = cache_get_version(name,AR_ALL);
	if (flags == ERR_NOVERSION)
		error_die("no version '%s' exists",name);
	if (flags == ERR_DUPVERSIONS)
		error_die("more than one version '%s' exists",name);
	if (flags < 0)
		error_die("failed to checkout version '%s'",name);
	if ((flags & AR_ALL) != AR_ALL)
		error_die("failed to fetch all components of version '%s'",name);

	tree.SetTo(root.Path(),"fullupdate");

	if (!isdir(tree.Path()))
		error_die("must -finish before -burn-ing or -deploy-ing");

	p.SetTo(root.Path(),"images");
	mkdir(p.Path(),0777);
	p.Append(deviceID);
	rm(p.Path());
	mkdir(p.Path(),0777);

	image.SetTo(p.Path(),"main_image");

	status("determining flash disk size");

	char ssize[64];
	int64 size = get_device_flash_size(deviceID);
	if (size % 512) {
		warning("device flash size (%Ld) is not a multiple of 512",size);
	} else {
		status("device flash size is %Ld",size);
	}

	zrecover.SetTo(root.Path(),"deltadata/zrecover.configured");
	if ((err = lstat(zrecover.Path(),&sb)))
		error_die("could not stat recovery image");

/**********/	
	int32 config_size = atol(cfg["recovery_size"]);
	config_size = ((config_size+511)/512) * 512;
	if (sb.st_size > config_size)
		error_die("recovery image is larger than reserved space");

	size -= config_size;
	size -= 3*512; // for the MBR
	sprintf(ssize,"%Ld",size);
/********/

	
	p.SetTo(root.Path(),"fullupdate");
	zbeos.SetTo(root.Path(),"versionarchive/spec/zbeos");

	if (!strcmp(deviceID,"_local"))
		status("creating CFS main partition image for local flash");
	else
		status("creating CFS main partition image for device '%s'",deviceID);

	arglist args;
	args.add(MKCFS_PATHNAME);
	args.add("--blocksize");
	args.add("2k");
	args.add("--silent");
	args.add("--boot");
	args.add(zbeos.Path());
	args.add("--clear");
	args.add("--size");
	args.add(ssize);
	args.add("--source");
	args.add(p.Path());
	args.add(image.Path());
	args.add("BeIA");
	if ((err = simple_exec(args.count(),args)))
		error_die("could not make CFS image of main partition");
}

void do_burn(const char *name, const char *deviceID)
{
	status_t err;
	struct stat sb;
	char tmp[1204];
	BPath flashImage,image,zrecover,p,root(CACHE_PATHNAME,name);

	flashImage.SetTo(root.Path(),"images");
	flashImage.Append(deviceID);
	flashImage.Append("flash_image");
	image.SetTo(root.Path(),"images");
	image.Append(deviceID);
	image.Append("main_image");

	zrecover.SetTo(root.Path(),"deltadata/zrecover.configured");

	status("creating flash disk image");

	arglist args;
	args.add(MKFLASH_PATHNAME);
	args.add(flashImage.Path());
	args.add(image.Path());
	args.add(zrecover.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not create flash disk image");

	const char *flashDevice = cfg["flash_device"];
	if (!flashDevice)
		error_die("no flash device configured");

	status("burning image onto flash");

	if ((err = lstat(flashImage.Path(),&sb)))
		error_die("couldn't stat flash image file");

	args.clear();
	args.add(DD_PATHNAME);

	strcpy(tmp,"if=");
	strcat(tmp,flashImage.Path());
	args.add(strdup(tmp),true);

	strcpy(tmp,"of=");
	strcat(tmp,flashDevice);
	args.add(strdup(tmp),true);

	args.add("bs=65536");

	sprintf(tmp,"count=%ld",(int32)(sb.st_size+65535)/65536);
	args.add(strdup(tmp),true);

	if ((err = simple_exec(args.count(),args)))
		error_die("could not burn flash");
}

void do_deploy(const char *name, const char *deviceID)
{
	status_t err;
	BPath image,signedImage,tree,p,root(CACHE_PATHNAME,name);
	BPath dsaParams,dsaKey,dsaPrivKey;

	p.SetTo(root.Path(),"images");
	p.Append(deviceID);
	image.SetTo(p.Path(),"main_image");
	signedImage.SetTo(p.Path(),"main_image.recovery");
	dsaParams.SetTo(root.Path(),"deltadata/dsaparams");
	dsaKey.SetTo(root.Path(),"deltadata/dsakey");
	dsaPrivKey.SetTo(root.Path(),"deltadata/dsakey.priv");

	if (!isfile(image.Path()))
		error_die("couldn't find main partition image");

	if (!isfile(dsaParams.Path()) ||
		!isfile(dsaPrivKey.Path()))
		error_die("couldn't find DSA files");

	status("RLE compressing image");

	arglist args;
	args.add(RLEIMAGE_PATHNAME);
	args.add(image.Path());
	args.add(signedImage.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not compress main partition image");

	status("crypto signing compressed image");

	args.clear();
	args.add(DSASIG_PATHNAME);
	args.add("sign");
	args.add(dsaParams.Path());
	args.add(dsaKey.Path());
	args.add(signedImage.Path());
	if ((err = simple_exec(args.count(),args)))
		error_die("could not cryptographically sign main partition image");
	
	status("deploying version '%s' to device '%s'",name,deviceID);

	if ((err = devclass_deploy(deviceID,name,signedImage.Path())))
		error_die("could not deploy to device");
}

int myPG=-1;
int main(int argc,char **argv,char **/*env*/)
{
	myPG = find_thread(NULL);
	setpgid(0,0);

	int stdErr = open("/boot/station/log",O_TRUNC|O_CREAT|O_WRONLY);
	dup2(stdErr,STDERR_FILENO);
	close(stdErr);

	bool burn=false;
	bool cache=false;
	bool create=false;
	bool finish=false;
	bool checkin=false;
	bool verify=false;
	bool deploy=false;
	bool info=false;
	bool uncache=false;
	bool deleteVersion=false;
	const char *versionName=NULL,*checkout=NULL,*deviceID=NULL;

	int32 i = 1;
	while (i < argc) {
		if (!strcmp(argv[i],"-v")) {
			if (i == (argc-1)) error_die("-v is last parameter");
			versionName = argv[i+1];
			workingVersion = versionName;
			i++;
		}

		if (!strcmp(argv[i],"-checkout")) {
			if (i == (argc-1)) error_die("-checkout is last parameter");
			checkout = argv[i+1];
			i++;
		}

		if (!strcmp(argv[i],"-installby")) {
			if (i == (argc-1)) error_die("-installby is last parameter");
			installby = argv[i+1];
			i++;
		}

		if (!strcmp(argv[i],"-rename")) {
			if (i == (argc-1)) error_die("-rename is last parameter");
			doRename = argv[i+1];
			i++;
		}

		if (!strcmp(argv[i],"-device")) {
			if (i == (argc-1)) error_die("-device is last parameter");
			deviceID = argv[i+1];
			i++;
		}

		if (!strcmp(argv[i],"-flashsize")) {
			if (i == (argc-1)) error_die("-flashsize is last parameter");
			flashSize = argv[i+1];
			i++;
		}

		if (!strcmp(argv[i],"-cache"))
			cache = true;

		if (!strcmp(argv[i],"-info"))
			info = true;

		if (!strcmp(argv[i],"-burn"))
			burn = true;

		if (!strcmp(argv[i],"-deploy"))
			deploy = true;

		if (!strcmp(argv[i],"-verify"))
			verify = true;

		if (!strcmp(argv[i],"-create"))
			create = true;

		if (!strcmp(argv[i],"-finish"))
			finish = true;

		if (!strcmp(argv[i],"-checkin"))
			checkin = true;

		if (!strcmp(argv[i],"-reboot"))
			fReboot = true;

		if (!strcmp(argv[i],"-redial"))
			fRedial = true;

		if (!strcmp(argv[i],"-flush"))
			fFlush = true;

		if (!strcmp(argv[i],"-freeze"))
			fFreeze = true;

		if (!strcmp(argv[i],"-restart"))
			fRestart = true;

		if (!strcmp(argv[i],"-delete"))
			deleteVersion = true;

		if (!strcmp(argv[i],"-uncache"))
			uncache = true;

		i++;
	}

	start_md5_thread();

	if (!versionName)
		error_die("must specify version ID (-v)");
	if (!deleteVersion && !uncache && !cache && !info && !create && !finish && !checkin && !burn && !deploy)
		error_die("nothing to do");
	if ((deleteVersion || uncache) && (create || checkout || checkin || burn || deploy || finish))
		error_die("cannot specify -delete or -uncache with any other options");
	if (checkout && !create)
		error_die("can only specify -checkout with -create");
	if (deploy && !deviceID)
		error_die("must specify -device with -deploy");

	if (!finish) {
		if (installby) warning("not finishing, ignoring -installby");
		if (fReboot) warning("not finishing, ignoring -reboot");
		if (fRedial) warning("not finishing, ignoring -redial");
		if (fRestart) warning("not finishing, ignoring -restart");
		if (fFlush) warning("not finishing, ignoring -flush");
		if (fFreeze) warning("not finishing, ignoring -freeze");
	}
	if (!checkin) {
		if (doRename) warning("not checking in, ignoring -rename");
	}
	
	if (uncache) do_uncache(versionName);
	if (deleteVersion) do_delete(versionName);
	if (cache) do_cache(versionName);
	if (info) do_info(versionName);
	if (create) {
		if (checkout)	do_checkout(versionName,checkout);
		else			do_create(versionName);
	}
	if (finish) do_finish(versionName);
	if (checkin) do_checkin(versionName);
	if (burn || deploy) do_images(versionName,deviceID?deviceID:"_local");
	if (burn) do_burn(versionName,deviceID?deviceID:"_local");
	if (deploy) do_deploy(versionName,deviceID);

	status("done");

	/* note: don't stop this thread until the last log has been sent */
	stop_md5_thread();

}
