interface */
/* inform the bus manager of our intent to support a set of devices */
status_t register_driver(const char *driver_name,
							const usb_support_descriptor *descriptors, 
							size_t count,
							const char *optional_republish_driver_name);								

/* request notification from the bus manager for add/remove of devices we
   support */
status_t install_notify(const char *driver_name, 
						   const usb_notify_hooks *hooks);
status_t uninstall_notify(const char *driver_name);

/* get the device descriptor */
const usb_device_descriptor* get_device_descriptor(usb_device device);

/* get the nth supported configuration */	
const usb_configuration_info* get_nth_configuration(usb_device device, uint index);

/* get the active configuration */
const usb_configuration_info* get_configuration(usb_device device);

/* set the active configuration */	
status_t set_configuration(usb_device device,
							  const usb_configuration_info *configuration); 

status_t set_alt_interface(usb_device device,
							  const usb_interface_info *ifc);

/* standard device requests -- convenience functions        */
/* handle may be a usb_device, usb_pipe, or usb_interface   */
status_t set_feature(usb_id handle, uint16 selector);
status_t clear_feature(usb_id handle, uint16 selector);
status_t get_status(usb_id handle, uint16 *status);

status_t get_descriptor(usb_device device, 
						   uint8 type, uint8 index, uint16 lang,
						   void *data, size_t len, size_t *actual_len);

/* generic device request function */	
status_t send_request(usb_device device, 
						 uint8 request_type, uint8 request,
						 uint16 value, uint16 index, uint16 length,
						 void *data, size_t *actual_len);

status_t send_request_safe(usb_device device, 
						 uint8 request_type, uint8 request,
						 uint16 value, uint16 index, uint16 length,
						 void *data, size_t *actual_len);


/* async request queueing */
status_t queue_interrupt(usb_pipe pipe, 
							void *data, size_t len,
							usb_callback_func notify, void *cookie);

status_t queue_bulk(usb_pipe pipe, 
					   void *data, size_t len,
					   usb_callback_func notify, void *cookie);
							
status_t queue_bulk_v(usb_pipe pipe, 
					   iovec *vec, size_t count,
					   usb_callback_func notify, void *cookie);

status_t queue_isochronous(usb_pipe pipe, 
							  void *data, size_t len,
							  rlea* rle_array, uint16 buffer_duration_ms,
							  usb_callback_func notify, void *cookie);

status_t queue_request(usb_device device, 
						  uint8 request_type, uint8 request,
						  uint16 value, uint16 index, uint16 length,
						  void *data, usb_callback_func notify, void *cookie);

status_t set_pipe_policy(usb_pipe pipe, uint8 max_num_queued_packets,
							uint16 max_buffer_duration_ms, uint16 sample_size);
						 
/* cancel pending async requests to an endpoint */
status_t cancel_queued_transfers(usb_pipe pipe); 

/* tuning, timeouts, etc */
status_t usb_ioctl(uint32 opcode, void* buf, size_t buf_size);


/* Version 2.0 interface support */
status_t queue_request_v2(usb_device device, 
					   uint8 request_type, uint8 request,
					   uint16 value, uint16 index, uint16 length,
					   void *data, size_t data_len,
					   usb_callback_func notify, void *cookie);

status_t send_request_safe_v2(usb_device device, 
						   uint8 request_type, uint8 request,
						   uint16 value, uint16 index, uint16 length,
						   void *data, size_t data_len, size_t *actual_len);


#define SIG_PIPE		'PipE'
#define SIG_BAD			'CRAP'
#define SIG_DEVICE		'DevC'
#define SIG_INTERFACE	'IntF'

#define SIG_SET_PIPE(pipe) (pipe->signature = SIG_PIPE)
#define SIG_SET_DEVICE(dev) (dev->signature = SIG_DEVICE)
#define SIG_SET_INTERFACE(ifc) (ifc->signature = SIG_INTERFACE)

#define SIG_CLR_PIPE(pipe) (pipe->signature = SIG_BAD)
#define SIG_CLR_DEVICE(dev) (dev->signature = SIG_BAD)
#define SIG_CLR_INTERFACE(ifc) (ifc->signature = SIG_BAD)

#define SIG_CHECK_PIPE(pipe) ((!pipe) || (pipe->signature != SIG_PIPE))
#define SIG_CHECK_DEVICE(dev) ((!dev) || (dev->signature != SIG_DEVICE))
#define SIG_CHECK_INTERFACE(ifc) ((!ifc) || (ifc->signature != SIG_INTERFACE))

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #include "usbd.h"

/* Version 2.0 interface support */
status_t queue_request_v2(const usb_id id, 
					   uint8 request_type, uint8 request,
					   uint16 value, uint16 index, uint16 length,
					   void *data, size_t data_len,
					   usb_callback_func notify, void *cookie)
					   
{
	return queue_request(id, request_type, request, value, index,
						length, data, notify, cookie);
}

status_t send_request_safe_v2(const usb_id id, 
						   uint8 request_type, uint8 request,
						   uint16 value, uint16 index, uint16 length,
						   void *data, size_t data_len, size_t *actual_len)
{
	return send_request_safe(id,request_type,request,value,index,length,
						data,actual_len);
}
                                                                                                                                                                                                                                                                                                                                            ���i                 ��������                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               ������������������������ ! ...GNUmakefileMakefileaic5800ohci        ! �     �     e&     �     �     !�                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         #! /bin/sh
# Generated automatically by configure.
# Run this file to recreate the current configuration.
# This directory was configured as follows,
# on host gnube:
#
# ./configure  -v --prefix=/ --cache-file=config.cache i586-beos
#
# Compiler output produced by configure, useful for debugging
# configure, is in ./config.log if it exists.

ac_cs_usage="Usage: ./config.status [--recheck] [--version] [--help]"
for ac_option
do
  case "$ac_option" in
  -recheck | --recheck | --rechec | --reche | --rech | --rec | --re | --r)
    echo "running ${CONFIG_SHELL-/bin/sh} ./configure  -v --prefix=/ --cache-file=config.cache i586-beos --no-create --no-recursion"
    exec ${CONFIG_SHELL-/bin/sh} ./configure  -v --prefix=/ --cache-file=config.cache i586-beos --no-create --no-recursion ;;
  -version | --version | --versio | --versi | --vers | --ver | --ve | --v)
    echo "./config.status generated by autoconf version 2.13"
    exit 0 ;;
  -help | --help | --hel | --he | --h)
    echo "$ac_cs_usage"; exit 0 ;;
  *) echo "$ac_cs_usage"; exit 1 ;;
  esac
done

ac_given_srcdir=.
ac_given_INSTALL="/bin/install -c"

trap 'rm -fr Makefile doc/Makefile lib/Makefile src/Makefile checks/Makefile examples/Makefile config.h conftest*; exit 1' 1 2 15

# Protect against being on the right side of a sed subst in config.status.
sed 's/%@/@@/; s/@%/@@/; s/%g$/@g/; /@g$/s/[\\&%]/\\&/g;
 s/@@/%@/; s/@@/@%/; s/@g$/%g/' > conftest.subs <<\CEOF
/^[ 	]*VPATH[ 	]*=[^:]*$/d

s%@SHELL@%/bin/sh%g
s%@CFLAGS@%-g -O2%g
s%@CPPFLAGS@%%g
s%@CXXFLAGS@%%g
s%@FFLAGS@%%g
s%@DEFS@%-DHAVE_CONFIG_H%g
s%@LDFLAGS@%%g
s%@LIBS@%%g
s%@exec_prefix@%${prefix}%g
s%@prefix@%/%g
s%@program_transform_name@%s,x,x,%g
s%@bindir@%${exec_prefix}/bin%g
s%@sbindir@%${exec_prefix}/sbin%g
s%@libexecdir@%${exec_prefix}/libexec%g
s%@datadir@%${prefix}/share%g
s%@sysconfdir@%${prefix}/etc%g
s%@sharedstatedir@%${prefix}/com%g
s%@localstatedir@%${prefix}/var%g
s%@libdir@%${exec_prefix}/lib%g
s%@includedir@%${prefix}/include%g
s%@oldincludedir@%/usr/include%g
s%