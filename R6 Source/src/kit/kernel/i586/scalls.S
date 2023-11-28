
#include <asm.h>
#include "fsc.h"

ENTRY_EXPORT(_kopen_)
	movl $0, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kclose_)
	movl $1, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_)
	movl $2, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwrite_)
	movl $3, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kioctl_)
	movl $4, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_klseek_)
	movl $5, %eax
	int  $0x25
	ret

ENTRY_EXPORT(copy_sighandlers)
	movl $6, %eax
	int  $0x25
	ret

ENTRY_EXPORT(real_time_clock)
	movl $7, %eax
	int  $0x25
	ret

ENTRY_EXPORT(resize_area)
	movl $8, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_real_time_clock)
	movl $9, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kload_add_on_)
	movl $10, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kfcntl_)
	movl $11, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kopendir_)
	movl $12, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kexit_thread_)
	movl $13, %eax
	int  $0x25
	ret

ENTRY_EXPORT(snooze)
	movl $14, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kclosedir_)
	movl $15, %eax
	int  $0x25
	ret

ENTRY_EXPORT(kill_thread)
	movl $16, %eax
	int  $0x25
	ret

ENTRY_EXPORT(resume_thread)
	movl $17, %eax
	int  $0x25
	ret

ENTRY_EXPORT(acquire_sem)
	movl $18, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kunload_add_on_)
	movl $19, %eax
	int  $0x25
	ret

ENTRY_EXPORT(create_area)
	movl $20, %eax
	int  $0x25
	ret

ENTRY_EXPORT(delete_area)
	movl $21, %eax
	int  $0x25
	ret

ENTRY_EXPORT(get_image_symbol)
	movl $22, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krewinddir_)
	movl $23, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kshutdown_)
	movl $24, %eax
	int  $0x25
	ret

ENTRY_EXPORT(find_area)
	movl $25, %eax
	int  $0x25
	ret

ENTRY_EXPORT(rename_thread)
	movl $26, %eax
	int  $0x25
	ret

ENTRY_EXPORT(get_nth_image_symbol)
	movl $27, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kreaddir_)
	movl $28, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_thread_info)
	movl $29, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kmkdir_)
	movl $30, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_ksymlink_)
	movl $31, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_cpu_state_)
	movl $32, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_cpu_state_)
	movl $33, %eax
	int  $0x25
	ret

ENTRY_EXPORT(wait_for_thread)
	movl $34, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kreadlink_)
	movl $35, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_dprintf_enabled_)
	movl $36, %eax
	int  $0x25
	ret

ENTRY_EXPORT(read_config_item)
	movl $37, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krename_)
	movl $38, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kunlink_)
	movl $39, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_port_info)
	movl $40, %eax
	int  $0x25
	ret

ENTRY_EXPORT(has_data)
	movl $41, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_sem_info)
	movl $42, %eax
	int  $0x25
	ret

ENTRY_EXPORT(release_sem)
	movl $43, %eax
	int  $0x25
	ret

ENTRY_EXPORT(delete_sem)
	movl $44, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kspawn_thread_)
	movl $45, %eax
	int  $0x25
	ret

ENTRY_EXPORT(install_default_debugger)
	movl $46, %eax
	int  $0x25
	ret

/*
 * find_thread. it is still defined here for backward compatibility.
 * new code uses the static inline defined in OS.h.
 */

ENTRY_EXPORT(find_thread)
	cmpl	$0, 4(%esp)
	jnz		real
	
	pushl	%ebx
	COMPUTE_GOT(find_thread)

	movl	EXTERN_GOT(__supported_fsc_types), %eax
	movl	(%eax), %eax
	test	%eax, %eax	/* Does the CPU support any FSC ? */
	jz		fake	
	
	test	$FSC_AMD, %eax 
	jnz		fsc_amd
	
fsc_intel:        
	movl	$1, %eax 	/* FSC # */
	movl	EXTERN_GOT(fsci_r), %edx	/* address of return from kernel mode. We could return directly to the calling function but it'll ruin the branch cache. */ 
	mov		%esp, %ecx 
	sysenter 
ENTRY(fsci_r)
	popl	%ebx
 	ret 			

fsc_amd:
	movl	$1, %eax 	/* FSC # */
	syscall
	popl	%ebx
	ret

fake:
	int		$0x26
	popl	%ebx
	ret

real:

ENTRY_EXPORT(_kfind_thread_)
	movl	$47, %eax
	int		$0x25
	ret

ENTRY_EXPORT(_krstat_)
	movl $48, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kmount_)
	movl $49, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_sem_owner)
	movl $50, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_port_owner)
	movl $51, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kload_image_)
	movl $52, %eax
	int  $0x25
	ret

ENTRY_EXPORT(install_team_debugger)
	movl $53, %eax
	int  $0x25
	ret

ENTRY_EXPORT(remove_team_debugger)
	movl $54, %eax
	int  $0x25
	ret

ENTRY_EXPORT(suspend_thread)
	movl $55, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_debugger)
	movl $56, %eax
	int  $0x25
	ret

ENTRY_EXPORT(debug_thread)
	movl $57, %eax
	int  $0x25
	ret

ENTRY_EXPORT(kill_team)
	movl $58, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_team_info)
	movl $59, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_image_info)
	movl $60, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_image_info)
	movl $61, %eax
	int  $0x25
	ret

ENTRY_EXPORT(write_config_item)
	movl $62, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kexit_team_)
	movl $63, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kunmount_)
	movl $64, %eax
	int  $0x25
	ret

ENTRY_EXPORT(create_port)
	movl $65, %eax
	int  $0x25
	ret

ENTRY_EXPORT(write_port)
	movl $66, %eax
	int  $0x25
	ret

ENTRY_EXPORT(read_port)
	movl $67, %eax
	int  $0x25
	ret

ENTRY_EXPORT(port_count)
	movl $68, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kfree_memory_)
	movl $69, %eax
	int  $0x25
	ret

ENTRY_EXPORT(start_event_log)
	movl $70, %eax
	int  $0x25
	ret

ENTRY_EXPORT(stop_event_log)
	movl $71, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_thread_priority)
	movl $72, %eax
	int  $0x25
	ret

ENTRY_EXPORT(delete_port)
	movl $73, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kdup2_)
	movl $74, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwstat_)
	movl $75, %eax
	int  $0x25
	ret

ENTRY_EXPORT(read_pci_config)
	movl $76, %eax
	int  $0x25
	ret

ENTRY_EXPORT(write_pci_config)
	movl $77, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_area_protection)
	movl $78, %eax
	int  $0x25
	ret

ENTRY_EXPORT(clone_area)
	movl $79, %eax
	int  $0x25
	ret

ENTRY_EXPORT(get_nth_pci_info)
	movl $80, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_team_info)
	movl $81, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_area_info)
	movl $82, %eax
	int  $0x25
	ret

ENTRY_EXPORT(read_port_etc)
	movl $83, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_area_info)
	movl $84, %eax
	int  $0x25
	ret

ENTRY_EXPORT(area_for)
	movl $85, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_system_info)
	movl $86, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kchdir_)
	movl $87, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kaccess_)
	movl $88, %eax
	int  $0x25
	ret

ENTRY_EXPORT(send_data)
	movl $89, %eax
	int  $0x25
	ret

ENTRY_EXPORT(receive_data)
	movl $90, %eax
	int  $0x25
	ret

ENTRY_EXPORT(release_sem_etc)
	movl $91, %eax
	int  $0x25
	ret

ENTRY_EXPORT(get_sem_count)
	movl $92, %eax
	int  $0x25
	ret

ENTRY_EXPORT(create_sem)
	movl $93, %eax
	int  $0x25
	ret

ENTRY_EXPORT(write_port_etc)
	movl $94, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstatfs_)
	movl $95, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krmdir_)
	movl $96, %eax
	int  $0x25
	ret

ENTRY_EXPORT(is_computer_on)
	movl $97, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kunload_library_)
	movl $98, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kcopy_area_)
	movl $99, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_thread_info)
	movl $100, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_port_info)
	movl $101, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_next_sem_info)
	movl $102, %eax
	int  $0x25
	ret

ENTRY_EXPORT(find_port)
	movl $103, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kdprintf_)
	movl $104, %eax
	int  $0x25
	ret

ENTRY_EXPORT(port_buffer_size)
	movl $105, %eax
	int  $0x25
	ret

ENTRY_EXPORT(acquire_sem_etc)
	movl $106, %eax
	int  $0x25
	ret

ENTRY_EXPORT(port_buffer_size_etc)
	movl $107, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kopen_vn_)
	movl $108, %eax
	int  $0x25
	ret

ENTRY_EXPORT(get_nth_image_dependence)
	movl $109, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_klink_)
	movl $110, %eax
	int  $0x25
	ret

ENTRY_EXPORT(sync)
	movl $111, %eax
	int  $0x25
	ret

ENTRY_EXPORT(__sigaction)
	movl $112, %eax
	int  $0x25
	ret
	.globl sigaction
sigaction = __sigaction
	.weak sigaction

ENTRY_EXPORT(sigpending)
	movl $113, %eax
	int  $0x25
	ret

ENTRY_EXPORT(__sigsuspend)
	movl $114, %eax
	int  $0x25
	ret
	.globl sigsuspend
sigsuspend = __sigsuspend
	.weak sigsuspend

ENTRY_EXPORT(sigprocmask)
	movl $115, %eax
	int  $0x25
	ret

ENTRY(send_signal)
	movl $116, %eax
	int  $0x25
	ret
	.globl __kill
__kill = send_signal
	.globl kill
kill = send_signal
	.weak kill

ENTRY_EXPORT(__getegid)
	movl $117, %eax
	int  $0x25
	ret
	.globl getegid
getegid = __getegid
	.weak getegid

ENTRY_EXPORT(__geteuid)
	movl $118, %eax
	int  $0x25
	ret
	.globl geteuid
	geteuid = __geteuid
	.weak __geteuid

ENTRY_EXPORT(__getgid)
	movl $119, %eax
	int  $0x25
	ret
	.globl getgid
getgid = __getgid
	.weak getgid

ENTRY_EXPORT(getpgrp)
	movl $120, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_alarm)
	movl $121, %eax
	int  $0x25
	ret

ENTRY_EXPORT(getppid)
	movl $122, %eax
	int  $0x25
	ret

ENTRY_EXPORT(__getuid)
	movl $123, %eax
	int  $0x25
	ret
	.globl getuid
getuid = __getuid
	.weak getuid

ENTRY_EXPORT(setgid)
	movl $124, %eax
	int  $0x25
	ret

ENTRY_EXPORT(setpgid)
	movl $125, %eax
	int  $0x25
	ret

ENTRY_EXPORT(setsid)
	movl $126, %eax
	int  $0x25
	ret

ENTRY_EXPORT(setuid)
	movl $127, %eax
	int  $0x25
	ret

ENTRY_EXPORT(tcgetpgrp)
	movl $128, %eax
	int  $0x25
	ret

ENTRY_EXPORT(tcsetpgrp)
	movl $129, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_parent)
	movl $130, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_ksyslog_initialize_)
	movl $131, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_thread_stacks_)
	movl $132, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_attr_)
	movl $133, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwrite_attr_)
	movl $134, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstart_watching_vnode_)
	movl $135, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstop_watching_vnode_)
	movl $136, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_pos_)
	movl $137, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwrite_pos_)
	movl $138, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_default_screen_info_)
	movl $139, %eax
	int  $0x25
	ret

ENTRY_EXPORT(fsync)
	movl $140, %eax
	int  $0x25
	ret

ENTRY_EXPORT(is_computer_on_fire)
	movl $141, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_klock_node_)
	movl $142, %eax
	int  $0x25
	ret

ENTRY(_kunlock_node_)
	movl $143, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kremove_attr_)
	movl $144, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstat_attr_)
	movl $145, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_attr_dir_)
	movl $146, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krewind_attr_dir_)
	movl $147, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kopen_attr_dir_)
	movl $148, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kclose_attr_dir_)
	movl $149, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstop_notifying_)
	movl $150, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kopen_index_dir_)
	movl $151, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kclose_index_dir_)
	movl $152, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_index_dir_)
	movl $153, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krewind_index_dir_)
	movl $154, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kcreate_index_)
	movl $155, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kremove_index_)
	movl $156, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstat_index_)
	movl $157, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kopen_query_)
	movl $158, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kclose_query_)
	movl $159, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kread_query_)
	movl $160, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kfork_)
	movl $161, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kexec_image_)
	movl $162, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwait_for_team_)
	movl $163, %eax
	int  $0x25
	ret

ENTRY_EXPORT(clear_caches)
	movl $164, %eax
	int  $0x25
	ret

ENTRY_EXPORT(real_time_clock_usecs)
	movl $166, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_thread_name_)
	movl $167, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_ksigactionvec_)
	movl $168, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwfsstat_)
	movl $169, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kmap_pages_)
	movl $170, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krename_attr_)
	movl $171, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_krename_index_)
	movl $172, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_tzspecs_)
	movl $173, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_tzfilename_)
	movl $174, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_tzfilename_)
	movl $175, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_rtc_info_)
	movl $176, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_thread_registers_)
	movl $177, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_thread_register_flag_)
	movl $178, %eax
	int  $0x25
	ret

ENTRY_EXPORT(read_isa_io)
	movl $179, %eax
	int  $0x25
	ret

ENTRY_EXPORT(write_isa_io)
	movl $180, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_klock_memory_)
	movl $181, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kunlock_memory_)
	movl $182, %eax
	int  $0x25
	ret

ENTRY_EXPORT(set_signal_stack)
	movl $183, %eax
	int  $0x25
	ret

ENTRY_EXPORT(disable_debugger)
	movl $184, %eax
	int  $0x25
	ret

ENTRY_EXPORT(snooze_until)
	movl $185, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_fd_limit_)
	movl $186, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kset_mon_limit_)
	movl $187, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kmap_physical_memory_)
	movl $188, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kapm_control_)
	movl $189, %eax
	int  $0x25
	ret

ENTRY_EXPORT(waiton)
	movl $190, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kreadv_pos_)
	movl $191, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kwritev_pos_)
	movl $192, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kget_safemode_option_)
	movl $193, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_get_team_usage_info)
	movl $194, %eax
	int  $0x25
	ret

ENTRY_EXPORT(close_port)
	movl $195, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kgeneric_syscall_)
	movl $197, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kstrace_init_)
	movl $198, %eax
	int	 $0x25
	ret			

ENTRY_EXPORT(_kgrant_)
	movl $199, %eax
	int  $0x25
	ret

ENTRY_EXPORT(_kmap_kernel_export_data_)
	movl $201, %eax
	int  $0x25
	ret

ENTRY_EXPORT(snooze_etc)
	movl $202, %eax
	int  $0x25
	ret
