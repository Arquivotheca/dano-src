#ifndef _PRIV_SYSCALLS_H
#define _PRIV_SYSCALLS_H

#include <BeBuild.h>
#include <sys/types.h>
#include <signal.h>
#include <iovec.h>
#include <fs_info.h>
#include <SupportDefs.h>
#include <OS.h>
#include <PCI.h>
#include <image.h>
#include <debugger.h>
#include <bootscreen.h>
#include <rtc_info.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dirent;
struct attr_info;
struct index_info;

extern int		_kset_fd_limit_(int num);
extern int		_kset_mon_limit_(int num);


extern int		_kmount_(const char *filesystem, int fd, const char *where,
								const char *devname, ulong flags, void *parms, size_t len);
extern int		_kunmount_(int fd, const char *path);
extern int		_kstatfs_(dev_t dev, long *pos, int fd,
								const char *path, struct fs_info *fs);
extern int		_kwfsstat_(dev_t dev, fs_info *fs, ulong mask);
extern int		_kopen_(int fd, const char *path, int omode,
								int perms, bool coe);
extern int		_kopen_vn_(dev_t dev, ino_t ino, const char *path, int omode, bool coe);
extern int		_kclose_(int fd);
extern ssize_t	_kread_(int fd, void *buf, size_t count,
								int *err_return);
extern ssize_t	_kwrite_(int fd, const void *buf, size_t count,
								int *err_return);
extern ssize_t	_kread_pos_(int fd, off_t pos, void *buf,
								size_t count, int *err_return);
extern ssize_t	_kwrite_pos_(int fd, off_t pos, const void *buf,
								size_t count, int *err_return);
extern ssize_t	_kreadv_pos_(int fd, off_t pos, const iovec *vec, size_t count,
								int *err_return);
extern ssize_t	_kwritev_pos_(int fd, off_t pos, const iovec *vec, size_t count,
								int *err_return);
extern off_t	_klseek_(int fd, off_t pos, int mode);
extern int		_kioctl_(int fd, int cmd, void *arg, size_t len);
extern int		_kfcntl_(int fd, int cmd, long arg);
extern int		_kdup2_(int fd, int nfd, bool coe);
extern int		_krstat_(int fd, const char *path, struct stat *st,
								bool eatlink);
extern int		_kwstat_(int fd, const char *path, struct stat *st,
								long mask, bool eatlink);
extern int		_klink_(int ofd, const char *oldpath, int nfd,
								const char *newpath);
extern int		_krmdir_(int fd, const char *path);
extern int		_kopendir_(int fd, const char *path, bool coe);
extern int		_kclosedir_(int dfd);
extern int		_krewinddir_(int dfd);
extern int		_kreaddir_(int fd, struct dirent *buf,
								size_t bufsize, long num);
extern int		_kmkdir_(int fd, const char *path, mode_t perms);
extern int		_krename_(int ofd, const char *oldpath, int nfd,
								const char *newpath);
extern int		_kunlink_(int fd, const char *path);
extern int		_ksymlink_(const char *oldpath, int nfd,
								const char *newpath);
extern ssize_t	_kreadlink_(int fd, const char *path, char *buf,
								size_t bufsize);
extern int		_kchdir_(int fd, const char *path);
extern int		_kaccess_(int fd, const char *path, int mode);
extern ssize_t	_kread_attr_(int fd, const char *attr, int type,
								off_t pos, void *buf, size_t len);
extern ssize_t	_kwrite_attr_(int fd, const char *attr, int type,
								off_t pos, const void *buf, size_t len);
extern int		_kremove_attr_(int fd, const char *attr);
extern int		_krename_attr_(int fd, const char *oldname,
								const char *newname);
extern int		_kstat_attr_(int fd, const char *attr,
								struct attr_info *buf);
extern int		_kopen_attr_dir_(int fd, const char *path,
								bool coe);
extern int		_kclose_attr_dir_(int fd);
extern int		_kread_attr_dir_(int fd, struct dirent *buf,
								size_t bufsize, long num);
extern int		_krewind_attr_dir_(int fd);
extern int		_kstat_index_(dev_t dev, const char *index,
								struct index_info *buf);
extern int		_kopen_index_dir_(dev_t dev, bool coe);
extern int		_kclose_index_dir_(int fd);
extern int		_kread_index_dir_(int fd, struct dirent *buf,
								size_t bufsize, long num);
extern int		_krewind_index_dir_(int fd);
extern int		_kcreate_index_(dev_t dev, const char *index,
								int type, uint flags);
extern int		_kremove_index_(dev_t dev, const char *index);
extern int		_krename_index_(dev_t dev, const char *oldindex,
								const char *newindex);
extern int		_kopen_query_(dev_t dev, const char *query,
								uint flags, port_id port, long token, bool coe);
extern int		_kclose_query_(int fd);
extern int		_kread_query_(int fd, struct dirent *buf,
								size_t bufsize, long num);
extern int		_kstart_watching_vnode_(dev_t dev, ino_t ino,
								ulong flags, port_id port, long token);
extern int		_kstop_watching_vnode_(dev_t dev, ino_t ino,
								port_id port, long token);
extern int		_kstop_notifying_(port_id port, long token);
extern int		_klock_node_(int fd);
extern int		_kunlock_node_(int fd);

extern thread_id	_kspawn_thread_(thread_entry function_name, 
			  						const char *thread_name, long priority, 
			  						void *arg);
extern thread_id _kfork_();
extern team_id	_kwait_for_team_(team_id tmid, uint32 options,
								thread_id *thid, int32 *reason, int32 *result);
extern status_t _kexit_thread_(int retval);
extern status_t _kexit_team_(int retval);
extern status_t _kset_thread_name_(const char *name);
extern status_t _kget_thread_stacks_(thread_id thread, char **sps);
extern status_t _kget_thread_registers_(thread_id thread, cpu_state *f);
extern status_t _kget_thread_register_flag(thread_id thread, bool flag);

extern int		_ksigactionvec_(int, struct sigaction *,
								struct sigaction *);

extern status_t _klock_memory_(void *addr, size_t size, ulong flags);
extern status_t _kunlock_memory_(void *addr, size_t size, ulong flags);
extern status_t _kfree_memory_(void *addr, size_t size);
extern status_t _kmap_pages_(void *addr, size_t size);
extern area_id	_kcopy_area_(const char *name, void **dest_addr, 
						   				uint32 addr_spec, uint32 protection, 
						   				area_id source);
extern area_id	_kmap_physical_memory_(	const char	*area_name,
										void		*physical_address,
										size_t		size,
										uint32		address_spec,
										uint32		protection,
										void		**mapped_address);

extern status_t _kexec_image_(int argc, char **argv, int envc, char **envp);
extern status_t _kload_image_(int argc, char **argv, int envc, char **envp, char *buf, int bufsize);
extern status_t _kload_add_on_(const char *path, const char *apath,
								const char *lpath);	
extern status_t _kunload_add_on_(image_id id);
extern status_t _kunload_library_(image_id id);
extern status_t get_nth_image_dependence(image_id id, int32 n);

extern status_t _kshutdown_(bool reboot);
extern status_t _kset_cpu_state_(int cpu, bool enabled);
extern bool	 _kget_cpu_state_(int cpu);
extern status_t _kset_dprintf_enabled_(bool enabled);
extern void	_kdprintf_(const char *str, ...);
extern int		_ksyslog_initialize_();


extern status_t _kget_default_screen_info_(screen *s);

extern int 	_kset_tzspecs_(long offset, int dst_observed);
extern int		_kset_tzfilename_(char *tzname, int len, int rtc_type);
extern int		_kget_tzfilename_(char *tzname);
extern status_t _kget_rtc_info_(rtc_info *info);

/* TO DO:
extern int		_kget_system_time_parms_();
*/


extern status_t	_kget_safemode_option_(const char *key,
												char *value, uint32 *len);
extern status_t	_kgeneric_syscall_(uint32 opcode, void* buf,
												size_t buf_len);			

extern status_t	waiton(uint32 max, uint32 *rfds, uint32 *wfds,
												uint32 *efds, bigtime_t t);


#if __INTEL__
extern status_t	_kapm_control_(uint32 op);
#endif

#if __POWERPC__
extern void **		_kget_tls_base_(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
