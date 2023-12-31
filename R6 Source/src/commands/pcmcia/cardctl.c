/*======================================================================

    PCMCIA device control program

    cardctl.c 1.44 1999/07/20 16:02:21

    The contents of this file are subject to the Mozilla Public
    License Version 1.1 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.

======================================================================*/

#ifndef __linux__
#include <pcmcia/u_compat.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <pcmcia/version.h>
#include <pcmcia/config.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

/*====================================================================*/

#ifdef ETC
static char *configpath = ETC;
#else
static char *configpath = "/etc/pcmcia";
#endif

static char *scheme = "/var/run/pcmcia-scheme";

static char *stabfile = "/var/run/stab";

/*====================================================================*/

#ifdef __linux__

static int major = 0;

static int lookup_dev(char *name)
{
    FILE *f;
    int n;
    char s[32], t[32];
    
    f = fopen("/proc/devices", "r");
    if (f == NULL)
	return -errno;
    while (fgets(s, 32, f) != NULL) {
	if (sscanf(s, "%d %s", &n, t) == 2)
	    if (strcmp(name, t) == 0)
		break;
    }
    fclose(f);
    if (strcmp(name, t) == 0)
	return n;
    else
	return -ENODEV;
} /* lookup_dev */

#endif /* __linux__ */

/*====================================================================*/

static int open_sock(int sock)
{
#ifdef __linux__
    int fd;
    char *fn;
    dev_t dev = (major<<8) + sock;
    if ((fn = tmpnam(NULL)) == NULL)
	return -1;
    if (mknod(fn, (S_IFCHR|S_IREAD|S_IWRITE), dev) != 0)
	return -1;
    fd = open(fn, O_RDONLY);
    unlink(fn);
    return fd;
#endif
#ifdef __BEOS__
    char fn[B_OS_NAME_LENGTH];
    sprintf(fn, "/dev/bus/pcmcia/sock/%d", sock);
    return open(fn, O_RDONLY);
#endif
} /* open_sock */

/*====================================================================*/

static void print_status(cs_status_t *status)
{
    char *v = "5";
    if (status->Function == 0) {
	printf("  ");
	if (status->CardState & CS_EVENT_3VCARD)
	    v = "3.3";
	else if (status->CardState & CS_EVENT_XVCARD)
	    v = "X.X";
	if (status->CardState & CS_EVENT_CB_DETECT)
	    printf("%sV cardbus card present", v);
	else if (status->CardState & CS_EVENT_CARD_DETECT)
	    printf("%sV 16-bit card present", v);
	else
	    printf("no card");
	if (status->CardState & CS_EVENT_PM_SUSPEND)
	    printf(", suspended");
	printf("\n");
    }
    if ((status->CardState & CS_EVENT_PM_SUSPEND) ||
	!(status->CardState & CS_EVENT_CARD_DETECT))
	return;
    printf("  Function %d: ", status->Function);
    printf("%s", (status->CardState & CS_EVENT_READY_CHANGE)
	   ? "ready" : "busy");
    if (status->CardState & CS_EVENT_WRITE_PROTECT)
	printf(", write protect");
    if (status->CardState & CS_EVENT_BATTERY_DEAD)
	printf(", battery dead");
    if (status->CardState & CS_EVENT_BATTERY_LOW)
	printf(", battery low");
    if (status->CardState & CS_EVENT_REQUEST_ATTENTION)
	printf(", request attention");
    printf("\n");
} /* print_status */

/*====================================================================*/

static void print_config(config_info_t *config)
{
    if (config->Function == 0) {
	printf("  Vcc = %.1f, Vpp1 = %.1f, Vpp2 = %.1f\n",
	       config->Vcc/10.0, config->Vpp1/10.0, config->Vpp2/10.0);
	if (!(config->Attributes & CONF_VALID_CLIENT))
	    return;
	printf("  Interface type is ");
	switch (config->IntType) {
	case INT_MEMORY:
	    printf("memory-only\n"); break;
	case INT_MEMORY_AND_IO:
	    printf("memory and I/O\n"); break;
	case INT_CARDBUS:
	    printf("cardbus\n"); break;
	}
	if (config->AssignedIRQ != 0) {
	    printf("  IRQ %d is ", config->AssignedIRQ);
	    switch (config->IRQAttributes & IRQ_TYPE) {
	    case IRQ_TYPE_EXCLUSIVE:
		printf("exclusive"); break;
	    case IRQ_TYPE_TIME:
		printf("time-multiplexed"); break;
	    case IRQ_TYPE_DYNAMIC_SHARING:
		printf("dynamic shared"); break;
	    }
	    if (config->IRQAttributes & IRQ_PULSE_ALLOCATED)
		printf(", pulse mode");
	    else
		printf(", level mode");
	    printf(", %s\n", (config->Attributes & CONF_ENABLE_IRQ) ?
		   "enabled" : "disabled");
	}
	if (config->Attributes & CONF_ENABLE_DMA)
	    printf("  DMA mode is enabled\n");
	if (config->Attributes & CONF_ENABLE_SPKR)
	    printf("  Speaker output is enabled\n");
    
    }

    if (!(config->Attributes & CONF_VALID_CLIENT))
	return;
    
    printf("  Function %d:\n", config->Function);

    if (config->CardValues) {
	printf("    Config register base = %#06x\n",
	       config->ConfigBase);
	printf("    ");
	if (config->CardValues & CV_OPTION_VALUE)
	    printf("  Option = %#04x", config->Option);
	if (config->CardValues & CV_STATUS_VALUE)
	    printf(", status = %#04x", config->Status);
	if (config->CardValues & CV_PIN_REPLACEMENT)
	    printf(", pin = %#04x", config->Pin);
	if (config->CardValues & CV_COPY_VALUE)
	    printf(", copy = %#04x", config->Copy);
	if (config->CardValues & CV_EXT_STATUS)
	    printf(", ext = %#04x", config->ExtStatus);
	printf("\n");
    }

    if (config->NumPorts1 > 0) {
	printf("    I/O window 1: %#06x to %#06x", config->BasePort1,
	       config->BasePort1 + config->NumPorts1 - 1);
	if (config->IntType == INT_CARDBUS) {
	    printf(", 32 bit\n");
	} else {
	    if (config->Attributes1 & IO_SHARED)
		printf(", shared");
	    if (config->Attributes1 & IO_FORCE_ALIAS_ACCESS)
		printf(", force alias");
	    switch (config->Attributes1 & IO_DATA_PATH_WIDTH) {
	    case IO_DATA_PATH_WIDTH_8:
		printf(", 8 bit\n"); break;
	    case IO_DATA_PATH_WIDTH_16:
		printf(", 16 bit\n"); break;
	    case IO_DATA_PATH_WIDTH_AUTO:
		printf(", auto sized\n"); break;
	    }
	}
    }
    if (config->NumPorts2 > 0) {
	printf("    I/O window 2: %#06x to %#06x", config->BasePort2,
	       config->BasePort2 + config->NumPorts2 - 1);
	if (config->Attributes2 & IO_SHARED)
	    printf(", shared");
	if (config->Attributes2 & IO_FORCE_ALIAS_ACCESS)
	    printf(", force alias");
	switch (config->Attributes2 & IO_DATA_PATH_WIDTH) {
	case IO_DATA_PATH_WIDTH_8:
	    printf(", 8 bit\n"); break;
	case IO_DATA_PATH_WIDTH_16:
	    printf(", 16 bit\n"); break;
	case IO_DATA_PATH_WIDTH_AUTO:
	    printf(", auto sized\n"); break;
	}
    }
} /* print_config */

/*====================================================================*/

static int get_tuple(int fd, cisdata_t code, ds_ioctl_arg_t *arg)
{
    arg->tuple.DesiredTuple = code;
    arg->tuple.Attributes = TUPLE_RETURN_COMMON;
    arg->tuple.TupleOffset = 0;
    if ((ioctl(fd, DS_GET_FIRST_TUPLE, arg) == 0) &&
	(ioctl(fd, DS_GET_TUPLE_DATA, arg) == 0) &&
	(ioctl(fd, DS_PARSE_TUPLE, arg) == 0))
	return 0;
    else
	return -1;
}

static void print_ident(int fd)
{
    ds_ioctl_arg_t arg;
    cistpl_vers_1_t *vers = &arg.tuple_parse.parse.version_1;
    cistpl_manfid_t *manfid = &arg.tuple_parse.parse.manfid;
    cistpl_funcid_t *funcid = &arg.tuple_parse.parse.funcid;
    int i;
    static char *fn[] = {
	"multifunction", "memory", "serial", "parallel",
	"fixed disk", "video", "network", "AIMS", "SCSI"
    };
    
    if (get_tuple(fd, CISTPL_VERS_1, &arg) == 0) {
	printf("  product info: ");
	for (i = 0; i < vers->ns; i++)
	    printf("%s\"%s\"", (i>0) ? ", " : "",
		   vers->str+vers->ofs[i]);
	printf("\n");
    } else {
	printf("  no product info available\n");
    }
    if (get_tuple(fd, CISTPL_MANFID, &arg) == 0)
	printf("  manfid: 0x%04x, 0x%04x\n",
	       manfid->manf, manfid->card);
    if (get_tuple(fd, CISTPL_FUNCID, &arg) == 0)
	printf("  function: %d (%s)\n", funcid->func,
	       fn[funcid->func]);
}

/*====================================================================*/

typedef enum cmd_t {
    C_STATUS, C_CONFIG, C_IDENT, C_SUSPEND,
    C_RESUME, C_RESET, C_EJECT, C_INSERT
} cmd_t;

static char *cmdname[] = {
    "status", "config", "ident", "suspend",
    "resume", "reset", "eject", "insert"
};

#define NCMD (sizeof(cmdname)/sizeof(char *))

static int do_cmd(int fd, int cmd)
{
    int i, ret;
    cs_status_t status;
    config_info_t config;

    ret = 0;
    switch (cmd) {
	
    case C_STATUS:
	for (i = 0; i < 4; i++) {
	    status.Function = i;
	    if (ioctl(fd, DS_GET_STATUS, &status) == 0)
		print_status(&status);
	    else {
		if (i == 0) {
		    if (errno == ENODEV)
			printf("  no card\n");
		    else
			perror("ioctl()");
		}
		break;
	    }
	}
	break;
	
    case C_CONFIG:
	for (i = 0; i < 4; i++) {
	    config.Function = i;
	    if (ioctl(fd, DS_GET_CONFIGURATION_INFO, &config) == 0)
		print_config(&config);
	    else {
		if (i == 0) printf("  not configured\n");
		break;
	    }
	}
	break;

    case C_IDENT:
	print_ident(fd);
	break;
	
    case C_SUSPEND:
	ret = ioctl(fd, DS_SUSPEND_CARD);
	break;
	
    case C_RESUME:
	ret = ioctl(fd, DS_RESUME_CARD);
	break;

    case C_RESET:
	ret = ioctl(fd, DS_RESET_CARD);
	break;

    case C_EJECT:
	ret = ioctl(fd, DS_EJECT_CARD);
	break;
	
    case C_INSERT:
	ret = ioctl(fd, DS_INSERT_CARD);
	break;
    }
    return ret;
}

/*======================================================================

    A utility function to scan /var/run/stab and apply a specified action
    to each device, in turn.  If any command returns a non-zero exit
    code, execute() returns -1.
    
======================================================================*/

typedef struct stab_t {
    int		status;
    char	class[33];
    char	dev[33];
} stab_t;

static stab_t stab[256];
static int nstab;

static int fetch_stab(void)
{
    char s[133];
    FILE *f;

    f = fopen("/var/run/stab", "r");
    if (f == NULL)
	return -1;
    for (nstab = 0; fgets(s, 132, f); ) {
	if (s[0] != 'S') {
	    sscanf(s, "%*d\t%s\t%*s\t%*d\t%s",
		   stab[nstab].class, stab[nstab].dev);
	    stab[nstab].status = 0;
	    nstab++;
	}
    }
    fclose(f);
    return 0;
}

static int execute(stab_t *s, char *action, char *scheme)
{
    int ret;
    char cmd[133];
    
    if (scheme)
	sprintf(cmd, "./%s %s %s %s", s->class, action, s->dev, scheme);
    else
	sprintf(cmd, "./%s %s %s", s->class, action, s->dev);
    ret = system(cmd);
    if (!WIFEXITED(ret) || WEXITSTATUS(ret))
	return -1;
    return 0;
}

static int stop_scheme(char *new)
{
    int i;
    
    fprintf(stderr, "checking:");
    for (i = 0; i < nstab; i++) {
	fprintf(stderr, " %s", stab[i].dev);
	stab[i].status = execute(stab+i, "cksum", new);
	if (stab[i].status &&
	    (execute(stab+i, "check", NULL) != 0)) break;
    }
    fprintf(stderr, "\n");
    if (i < nstab) {
	fprintf(stderr, "Device '%s' busy: scheme unchanged.\n",
		stab[i].dev);
	return -1;
    }
    for (i = 0; i < nstab; i++)
	if (stab[i].status) execute(stab+i, "stop", NULL);
    return 0;
}

static int start_scheme(void)
{
    int i, j = 0;
    
    for (i = 0; i < nstab; i++)
	if (stab[i].status) j |= execute(stab+i, "start", NULL);
    return j;
}

/*======================================================================

    do_scheme() is in charge of checking and updating the current 
    PCMCIA configuration scheme.  The current scheme is kept in a
    file, /var/run/pcmcia-scheme.  When updating the scheme, we first
    stop all PCMCIA devices, then update the scheme, then restart.
    
======================================================================*/

static int do_scheme(char *new)
{
    FILE *f;
    char old[33];
    int i;
    
    f = fopen(scheme, "r");
    if (f && fgets(old, 32, f))
	old[strlen(old)-1] = '\0';
    else
	old[0] = '\0';
    if (f) fclose(f);
    
    if (new) {

#ifndef __BEOS__
#ifndef UNSAFE_TOOLS
	if (getuid() != 0) {
	    fprintf(stderr, "Only root can select a new scheme.\n");
	    return -1;
	}
#endif
#endif
	
	/* Sanity checks... */
	for (i = 0; i < strlen(new); i++)
	    if (!isalnum(new[i])) break;
	if ((i != strlen(new)) || (strlen(new) < 1) ||
	    (strlen(new) > 32)) {
	    fprintf(stderr, "Bad scheme name.\n");
	    return -1;
	}
	if (strcmp(old, new) == 0) {
	    fprintf(stderr, "Scheme unchanged.\n");
	    return 0;
	}

	if (chdir(configpath) != 0) {
	    fprintf(stderr, "Could not change to %s.\n", configpath);
	    return -1;
	}
	
	/* Shut down devices in old scheme */
	if ((fetch_stab() == 0) && (stop_scheme(new) != 0))
	    return -1;

	/* Update scheme state */
	if (old[0])
	    printf("Changing scheme from '%s' to '%s'...\n", old, new);
	else
	    printf("Changing scheme to '%s'...\n", new);
	
	umask(022);
	f = fopen(scheme, "w");
	if (f) {
	    fprintf(f, "%s\n", new);
	    fclose(f);
	} else
	    perror("Could not set scheme.");

	/* Start up devices in new scheme */
	if (start_scheme() != 0)
	    fprintf(stderr, "Some devices did not start cleanly.\n");
	
    } else {
	if (old[0])
	    printf("Current scheme: '%s'.\n", old);
	else
	    printf("Current scheme: 'default'.\n");
    }
    return 0;
}

/*====================================================================*/

void usage(char *name)
{
    int i;
    fprintf(stderr, "usage: %s command [socket #]\n", name);
    fprintf(stderr, "    or %s [-c configpath] [-f scheme]"
	    " [-s stab] scheme [name]\n", name);
    fprintf(stderr, "    commands:");
    for (i = 0; i < NCMD; i++)
	fprintf(stderr, " %s", cmdname[i]);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/*====================================================================*/

#define MAX_SOCKS 8

int main(int argc, char *argv[])
{
    int cmd, fd[MAX_SOCKS], ns, ret, i;
    int optch, errflg;
    char *opts = (getuid() == 0) ? "Vc:f:s:" : "V";

    errflg = 0;
    while ((optch = getopt(argc, argv, opts)) != -1) {
	switch (optch) {
	case 'V':
	    fprintf(stderr, "cardctl version " CS_RELEASE "\n");
	    return 0;
	    break;
	case 'c':
	    configpath = strdup(optarg); break;
	case 'f':
	    scheme = strdup(optarg); break;
	case 's':
	    stabfile = strdup(optarg); break;
	default:
	    errflg = 1; break;
	}
    }
    
    if (errflg || (argc == optind) || (argc > optind+2))
	usage(argv[0]);
#ifndef __BEOS__
    if (geteuid() != 0) {
        fprintf(stderr, "cardctl must be setuid root\n");
	exit(EXIT_FAILURE);
    }
#endif

#ifdef __linux__
    major = lookup_dev("pcmcia");
    if (major < 0) {
	if (major == -ENODEV)
	    fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	else
	    perror("could not open /proc/devices");
	exit(EXIT_FAILURE);
    }
#endif

    if (strcmp(argv[optind], "scheme") == 0) {
#ifndef UNSAFE_TOOLS
	setuid(getuid());
#endif
	if (do_scheme((argc == optind+1) ? NULL : argv[optind+1]) == 0)
	    exit(EXIT_SUCCESS);
	else
	    exit(EXIT_FAILURE);
    }
    
    for (cmd = 0; cmd < NCMD; cmd++)
	if (strcmp(argv[optind], cmdname[cmd]) == 0) break;
    if (cmd == NCMD)
	usage(argv[0]);

    ret = 0;
    if (argc == optind+2) {
	ns = atoi(argv[optind+1]);
	fd[0] = open_sock(ns);
	if (fd[0] < 0) {
	    perror("open_sock()");
	    exit(EXIT_FAILURE);
	}
#ifndef UNSAFE_TOOLS
	setuid(getuid());
#endif
	ret = do_cmd(fd[0], cmd);
	if (ret != 0)
	    perror("ioctl()");
    } else {
	for (ns = 0; ns < MAX_SOCKS; ns++) {
	    fd[ns] = open_sock(ns);
	    if (fd[ns] < 0) break;
	}
#ifndef UNSAFE_TOOLS
	setuid(getuid());
#endif
	if (ns == 0) {
	    perror("open_sock()");
	    exit(EXIT_FAILURE);
	}
	for (ns = 0; (ns < MAX_SOCKS) && (fd[ns] >= 0); ns++) {
	    if (cmd <= C_IDENT)
		printf("Socket %d:\n", ns);
	    i = do_cmd(fd[ns], cmd);
	    if ((i != 0) && (errno != ENODEV)) {
		perror("ioctl()");
		ret = i;
	    }
	}
    }
    if (ret != 0)
	exit(EXIT_FAILURE);
    return 0;
}
