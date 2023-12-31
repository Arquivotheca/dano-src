nt		vsid, api, hv;

	/* compute the hashed value as well as the abreviated page index for
	the page */

	vsid = vatovsid(va, asr);
	hv = hashval(va, vsid);
	api = vatoapi(va);

	/* scan the primary and the secondary HPTEG. report PTE address if
	found, NULL otherwise */

	ptep = &hptegvbase[(hv % nhpteg)*8];
	for (i=0; i<8; i++, ptep++)
		if ((ptep->vsid == vsid) && (ptep->api == api) &&
		(ptep->h == 0) && ptep->v)
			return (ptep);

	ptep = &hptegvbase[(~hv % nhpteg)*8];
	for (i=0; i<8; i++, ptep++)
		if ((ptep->vsid == vsid) && (ptep->api == api) &&
		(ptep->h == 1) && ptep->v)
			return (ptep);

	/* the PTE has not been found in both the primary and the secondary
	hashed table groups. we must look for it in the auxilliary
	hash table. */

	if (info_cpu.software_table_walk) {
		axptegp = axhptegvbase[hv % nhpteg];
		while (axptegp != NULL) {

		/* the allocated slots are linked by physical addresses */

			vaxptegp = (axpt_group *) patova(axptegp);
			ptep = &vaxptegp->pte[0];
			for (i=0; i<7; i++, ptep++)
				if ((ptep->vsid == vsid) && (ptep->api == api) &&
				ptep->v)
					return (ptep);
			axptegp = vaxptegp->next;
		}
	}

	return (NULL);
}

/*
 * find_free_pte returns a pointer to a PTE which can be used to map
 * a particular page, or NULL if no can be found. it sets the bit h, so
 * that the caller does not have to care about the hash class.
 */

pt_entry *
find_free_pte(char *va, as_rec *asr)
{
	pt_entry	*ptep;
	axpt_group	*vaxptegp, *vnaxptegp;
	void		*axptegp, *naxptegp;
	int			i;
	uint		vsid, hv;

	vsid = vatovsid(va, asr);
	hv = hashval(va, vsid);

	/* scan the primary and secondary hashed page table group. if
	an available entry is found, return it. */

	ptep = &hptegvbase[(hv % nhpteg)*8];
	for(i=0; i<8; i++, ptep++)
		if (!ptep->v) {
			ptep->h = 0;
			return (ptep);
		}
	
	ptep = &hptegvbase[((~hv) % nhpteg)*8];
	for(i=0; i<8; i++, ptep++)
		if (!ptep->v) {
			ptep->h = 1;
			return (ptep);
		}

	/* no valid entry was found in both the primary and the secondary
	hashed table groups. we must allocate a slot in the auxilliary
	hash table. the h bit is clear in this table. */

	if (!info_cpu.software_table_walk) {
		panic("BAAAAAAD: no more PTE space!!!\n");
		return NULL;
	}

	axptegp = axhptegvbase[hv % nhpteg];
	while (axptegp != NULL) {

	/* the allocated slots are linked by physical addresses */

		vaxptegp = (axpt_group *) patova(axptegp);
		ptep = &vaxptegp->pte[0];
		for (i=0; i<7; i++, ptep++)
			if (!ptep->v)
				return ptep;
		axptegp = vaxptegp->next;
	}
	
	/* no slot was found in the auxilliary table. we must allocate
	one. */

	if (axnextavail == NULL) {
		dprintf("ALERT : find_free_pte : no more PTE (should not happen)\n");
		return (NULL);
	}

	naxptegp = axnextavail;
	axnextavail = ((axpt_group *)patova(naxptegp))->next;
	naxavail--;

	vnaxptegp = (axpt_group *) patova(naxptegp);
	vnaxptegp->next = NULL;

	/* sync is necessary here to ensure that other CPUs see that next is
	NULL as soon as the node is inserted in the list.*/
	do_sync();
	
	axptegp = axhptegvbase[hv % nhpteg];
	if (axptegp == NULL)
		axhptegvbase[hv % nhpteg] = naxptegp;
	else {
		vaxptegp = (axpt_group *) patova(axptegp);

	/* linked by physical addresses... */

		while (vaxptegp->next != NULL) {
			axptegp = vaxptegp->next;
			vaxptegp = (axpt_group *) patova(axptegp);
		}
		vaxptegp->next = naxptegp;
	}
	return (&vnaxptegp->pte[0]);
}

/*
 * compact_aux_hpt tries to compact the auxilliary hash table after a
 * PTE has been removed.
 */

void
compact_aux_hpt(char *va, as_rec *asr)
{
	void		*axptegp, *paxptegp;
	axpt_group	*vaxptegp, *vpaxptegp;
	pt_entry	*ptep;
	uint		vsid, hv;
	int		i;

	if (!info_cpu.software_table_walk)
		return;

	vsid = vatovsid(va, asr);
	hv = hashval(va, vsid);

	axptegp = axhptegvbase[hv % nhpteg];
	if (axptegp == NULL)
		return;

	/* find empty axpt_groups and remove then from the list.*/

	paxptegp = NULL;
	do {
		vaxptegp = (axpt_group *) patova(axptegp);
		ptep = &vaxptegp->pte[0];
		for (i=0; i<7; i++, ptep++)
			if (ptep->v)
				break;
		if (i==7) {
			if (paxptegp == NULL)
				axhptegvbase[hv % nhpteg] = vaxptegp->next;
			else
				vpaxptegp->next = vaxptegp->next;	

	/* before removing the last link (vaxptegp->next), we must make
	sure that all the other CPUs are done with any started table search.
	this is simply achieved by sending a synchronous intercpu int to
	all the CPUs. */
			do_sync();
			glb_sync();
			vaxptegp->next = axnextavail;

			axnextavail = axptegp;
			naxavail++;

	/* at most one slot can be freed each time */
			break;
		}
		paxptegp = axptegp;
		vpaxptegp = vaxptegp;
		axptegp = vaxptegp->next;
	} while (axptegp != NULL);
}

/*
 * clear_pte_r carefully clears the reference bit a pte
 */

void
clear_pte_r(pt_entry *ptep)
{
	char	*p;

	p = (char *)ptep + 6;
	*p &= 0xfe;
}

/*
 * set_pte_c carefully set the change bit a pte
 */

void
set_pte_c(pt_entry *ptep)
{
	char	*p;

	p = (char *)ptep + 7;
	*p |= 0x80;
}


/*
 * instruction_storage_interrupt and data_storage_interrupt handle
 * memory fault exceptions.
 */

int
instruction_storage_interrupt(char *pc, uint msr, iframe *f)
{
	analyser (0x90, (long) pc);
	set_msr(get_msr() | msr_ee);	/* reenable ints */

	return vm_flt_handler(pc, INST_ACC, msr, pc);
}

int
data_storage_interrupt(char *pc, uint msr, char *fault, uint dsisr, iframe *f)
{
	acc_type	op;

	analyser (0x94, (long) fault);
	set_msr(get_msr() | msr_ee);	/* reenable ints */

	/* determine type of access */
	op = ((dsisr >> 25) & 1) ? WRITE_ACC : READ_ACC;
	return vm_flt_handler(fault, op, msr, pc);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               ���i                 ��������                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               ������������������������ C ...bkpt.cbkpt.hboot.hbootdata.ccpu.cksh.hmem.hram.hsystime.ctimer.c   	    $ ) . 3 < C      �                                                                       RTSC     cpu       �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                `include test file.'
define()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
  Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* vmmvs.h:  include file for both VM/CMS and MVS ports of UnZip */
#ifndef __vmmvs_h               /* prevent multiple inclusions */
#define __vmmvs_h

#ifndef NULL
#  define NULL (zvoid *)0
#endif

#ifdef MVS
#  define _POSIX_SOURCE    /* tell MVS we want full definitions */
#  define NO_STRNICMP      /* MVS has no strnicmp() */
#  include <features.h>
   /* MVS complains if a function has the same name as a csect. */
#  if defined(__UNZIP_C)
#    pragma csect(STATIC,"unzip_s")
#  elif defined(__CRC32_C)
#    pragma csect(STATIC,"crc32_s")
#  elif defined(__ENVARGS_C)
#    pragma csect(STATIC,"envarg_s")
#  elif defined(__EXPLODE_C)
#    pragma csect(STATIC,"explod_s")
#  elif defined(__INFLATE_C)
#    pragma csect(STATIC,"inflat_s")
#  elif defined(__MATCH_C)
#    pragma csect(STATIC,"match_s")
#  elif defined(__UNREDUCE_C)
#    pragma csect(STATIC,"unredu_s")
#  elif defined(__UNSHRINK_C)
#    pragma csect(STATIC,"unshri_s")
#  elif defined(__ZIPINFO_C)
#    pragma csect(STATIC,"zipinf_s")
#  endif
#endif /* MVS */

#include <time.h>               /* the usual non-BSD time functions */
#ifdef VM_CMS
#  include "vmstat.h"
#endif
#ifdef MVS
#  include <sys/stat.h>
#endif

#define PASSWD_FROM_STDIN
                  /* Kludge until we know how to open a non-echo tty channel */

#define EBCDIC
#define __EBCDIC 2              /* treat EBCDIC as binary! */
/* In the context of Info-ZIP, a portable "text" mode file implies the use of
   an ASCII-compatible (ISO 8859-1, or other extended ASCII) code page. */

#ifdef MORE
#  undef MORE
#endif

/* Workarounds for missing RTL functionality */
#define isatty(t) 1

#ifdef UNZIP                    /* definitions for UNZIP */

#define INBUFSIZ 8192

#define USE_STRM_INPUT
#define USE_FWRITE

#define PATH_MAX 128

#ifndef QUERY_TRNEWLN
#  define QUERY_TRNEWLN         /* terminate interaction queries with '\n' */
#endif

#ifndef DATE_FORMAT
#  define DATE_FORMAT DF_MDY
#endif
#define lenEOL        1
/* The use of "ebcdic[LF]" is not reliable; VM/CMS C/370 uses the
 * EBCDIC specific "NL" ('NewLine') control character (and not the EBCDIC
 * equivalent of the ASCII "LF" ('LineFeed')) as line terminator!
 * To work around this problem, we explicitely emit the C compiler's native
 * '\n' line terminator.
 */
#if 0
#define PutNativeEOL  *q++ = native(LF);
#else
#define PutNativeEOL  *q++ = '\n';
#endif

#endif /* UNZIP */

#endif /* !__vmmvs_h */
                                                                                                                                                                                                                                                                                                            $(MY_PREFIX)/src/servers/app/decor/DecorStack.o: $(wildcard \
 $(BUILDHOME)/src/servers/app/decor/DecorStack.cpp \
 $(BUILDHOME)/headers/posix/stdio.h \
 $(BUILDHOME)/headers/posix/be_setup.h \
 $(BUILDHOME)/headers/BeBuild.h \
 $(BUILDHOME)/headers/posix/size_t.h \
 $(BUILDHOME)/headers/posix/stddef.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/stddef.h \
 $(BUILDHOME)/headers/posix/null.h \
 $(BUILDHOME)/headers/posix/va_list.h \
 $(BUILDHOME)/headers/posix/stdarg.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/stdarg.h \
 $(BUILDHOME)/headers/posix/wchar_t.h \
 $(BUILDHOME)/headers/posix/gnu_stdio.h \
 $(BUILDHOME)/headers/posix/sys/types.h \
 $(BUILDHOME)/headers/posix/time.h \
 $(BUILDHOME)/headers/posix/_G_config.h \
 $(BUILDHOME)/headers/posix/libio.h \
 $(BUILDHOME)/src/servers/app/as_support.h \
 $(BUILDHOME)/headers/posix/memory.h \
 $(BUILDHOME)/headers/posix/string.h \
 $(BUILDHOME)/headers/posix/malloc.h \
 $(BUILDHOME)/headers/posix/unistd.h \
 $(BUILDHOME)/src/servers/app/as_debug.h \
 $(BUILDHOME)/src/servers/app/regions.h \
 $(BUILDHOME)/headers/support/SupportDefs.h \
 $(BUILDHOME)/headers/support/Errors.h \
 $(BUILDHOME)/headers/posix/limits.h \
 $(BUILDHOME)/headers/posix/float.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/float.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/limits.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/syslimits.h \
 $(BUILDHOME)/src/servers/app/basic_types.h \
 $(BUILDHOME)/src/servers/app/BArray.h \
 $(BUILDHOME)/src/servers/app/rect.h \
 $(BUILDHOME)/src/servers/app/decor/DecorDef.h \
 $(BUILDHOME)/src/servers/app/decor/DecorTypes.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/typeinfo \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/exception \
 $(BUILDHOME)/src/servers/app/parcel.h \
 $(BUILDHOME)/headers/app/AppDefs.h \
 $(BUILDHOME)/headers/app/Message.h \
 $(BUILDHOME)/headers/kernel/OS.h \
 $(BUILDHOME)/headers/nustorage/StorageDefs.h \
 $(BUILDHOME)/headers/posix/fcntl.h \
 $(BUILDHOME)/headers/posix/sys/stat.h \
 $(BUILDHOME)/headers/posix/sys/param.h \
 $(BUILDHOME)/headers/interface/Rect.h \
 $(BUILDHOME)/headers/interface/Insets.h \
 $(BUILDHOME)/headers/interface/Point.h \
 $(BUILDHOME)/headers/posix/math.h \
 $(BUILDHOME)/headers/support/DataIO.h \
 $(BUILDHOME)/headers/support/Flattenable.h \
 $(BUILDHOME)/headers/support/TypeConstants.h \
 $(BUILDHOME)/headers/interface/GraphicsDefs.h \
 $(BUILDHOME)/src/servers/app/atom.h \
 $(BUILDHOME)/headers/support/Atom.h \
 $(BUILDHOME)/headers/support/Gehnaphore.h \
 $(BUILDHOME)/src/servers/app/gr_types.h \
 $(BUILDHOME)/headers/interface/Font.h \
 $(BUILDHOME)/headers/interface/InterfaceDefs.h \
 $(BUILDHOME)/src/inc/app_server_p/messages.h \
 $(BUILDHOME)/src/servers/app/font_defs.h \
 $(BUILDHOME)/src/servers/app/proto.h \
 $(BUILDHOME)/headers/posix/stdlib.h \
 $(BUILDHOME)/headers/posix/div_t.h \
 $(BUILDHOME)/headers/posix/sys/wait.h \
 $(BUILDHOME)/headers/posix/alloca.h \
 $(BUILDHOME)/src/servers/app/render/render.h \
 $(BUILDHOME)/src/servers/app/font.h \
 $(BUILDHOME)/headers/support/Locker.h \
 $(BUILDHOME)/src/inc/app_server_p/shared_fonts.h \
 $(BUILDHOME)/headers/support/SmartArray.h \
 $(BUILDHOME)/gnupro/lib/gcc-lib/i586-pc-beos/2.9-beos-000224/include/new \
 $(BUILDHOME)/headers/posix/assert.h \
 $(BUILDHOME)/src/servers/app/render/renderdefs.h \
 $(BUILDHOME)/headers/support/ByteOrder.h \
 $(BUILDHOME)/headers/posix/endian.h \
 $(BUILDHOME)/src/servers/app/render/renderContext.h \
 $(BUILDHOME)/src/servers/app/render/renderTypes.h \
 $(BUILDHOME)/src/servers/app/render/renderPort.h \
 $(BUILDHOME)/headers/add-ons/graphics/Accelerant.h \
 $(BUILDHOME)/src/servers/app/render/renderCanvas.h \
 $(BUILDHOME)/src/servers/app/render/accelPackage.h \
 $(BUILDHOME)/src/servers/app/render/renderFuncs.h \
 $(BUILDHOME)/src/servers/app/system.h \
 $(BUILDHOME)/headers/support/TLS.h \
 $(BUILDHOME)/headers/kernel/image.h \
 $(BUILDHOME)/src/servers/app/decor/DecorStream.h \
 $(BUILDHOME)/src/servers/app/decor/DecorExecutor.h \
 $(BUILDHOME)/src/servers/app/decor/DecorStack.h \
 $(BUILDHOME)/src/servers/app/decor/DecorPart.h \
 $(BUILDHOME)/src/servers/app/decor/DecorAtom.h \
 $(BUILDHOME)/src/servers/app/decor/DecorState.h \
 $(BUILDHOME)/src/servers/app/decor/DecorAtomInclude.h \
 $(BUILDHOME)/src/servers/app/decor/DecorBehavior.h \
 $(BUILDHOME)/src/servers/app/decor/DecorSharedAtom.h \
 $(BUILDHOME)/src/servers/app/decor/DecorActionDef.h \
)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ��z�&�䩞]<��;�G�>��{<��4	W@�s��˪e�%1e�.��53�~ƒk�x�@v�w���	'~���BHͅ\YË|���p%n�M���ى+�����)�S̪��m�U�Gy:m�mv�Q�i�+�#�S�<������_�K��~�b��)��An���[�	A҃��᩿��{��YMI{@z�i���z�{a{�Ə��&�~��<�w����P��9�������j=�H�m�C���6\��q��+��9ݤ�1@0��<�[�ˠ������U����78 ��gV}�V��w&��q���n^=B;kh�O�ٲxf����VtqD�X9i��.!�@����׿d�� �3� �0�J�WӜp���լ�{.���B,\��[q'�i��v��D��!�,Y&f�'Jt/C����SJ)ɲy�� _��>��&$��,o�)�G&]�ha�c�����>aIF#��|ˍW;?">���wg}
�i�]7C�{�Ao�}Ӄ�=ᡦ�A�S0+H�/��惢��y"��E��nì	�;���x�`�Z�W'�	za�q�?սw�ˀ8��E�'�b`ץ����o��~������&���ЈG�'���倽��t�����K�g���izp4��s�8Ù&�q���o������t���=���8 ~�7�_�Ms�	'~vD@�gC�\x�wK�3m3�1=��҈�f�����orӢ;)�T�*?K����I�A>5�]Z\�� ȕ��2?"/'d�G�~WN¼��KƇP^�\�5�_������V����Ƿ<��}�O����/3�,.U|���w���̮�;L0g���p7c��X�J���B���(?���:��,ȿ�q������J��
�w�ꍁ���k��#�����������m�Y^���WYǸA)=���x~���x�m��w��µSi�0)�!�?�w�������ߝp�w�p�w>,��X�du3�I�wޖ���Ͱ(�X��|OYg��
ʨ,N�Ji�峤\#�&����|����]�8!/���E���0�w������KrEt)��	��tfDjEytTl�{�|O��,��!e;dL�����9x�_	�^�����V�]���w�o��g7�*�Jz1z�)��ұͤ_9��RHĻz���#e���ѝD�`��.)mw\\�|�RQ�[��>ą�I�`�L�<�<Wr�:{=&�.��z��6���sE��� ��H�~lI�]^er�1�%Ɩ2.)�+�����N���� ww!�-DqS)a4�������j�T�Q��<��I�!y��dÜ�kQ=�T�+dH�J{M2(���zc��<�ߏ�i�<������Aa������yE�ǚ��4��u��j�ܳ
�<�O���^W��l�ӓR�'l���4�*+���n���Hxt��rD)���H�	t�>�{���Qc�T�Q�F�.�7}��Э��~��=���_�	/{I�~���<���]������&_�=$Շ-P�l�a�}���K� kuTП*
��vN�]з����u�6�2� �-x~Oe���e���Hy�
��^R�,uRVfu����9�?;��v�����'B����67�ZB-����*�h��5afxݤi�����p㼘$�2m6�Y��&�~\|�!�_zd~��/v���� ��!�e�|_U���OUٱ@6'���Nqiҫ3��p������;��ll L����y ~�S5�f�ߎ�����.Z����G9z`О��%�/�� ���ʩ�߯C�u�\��u��C�J�r��Z�#���3R�����DV�yS�����s�a��n�vp��S�<��^es`�᳃���a�̇E��ݲ�u:,�'���=���a,���粣ߝpⷋL!p� ��Q_���o�����3Yn[Y�p 1�����,Qe��@M�g�>�p�_�w�Տ�ˮ,���qG/}FFF�!����gV�_�9���P�r7P�-!�#g���c:p�H���;��;4Q;G��=��p��2�ǌ7������Wس�|�M~�6�{�8�i��׶�m��21ͮ�!A�(�@���8��o�.�� ֋�5.Nm�]]T+9���SMf�-��
�3�4n��9m뎲�S[�d��w{h/A����3?�ιc�CE8R�s�wS8��SC�n�:���ﯾ�g��>sa��U��O��9>�Mo����m؋�o���z>���0��7W�G/�3�:Gl��U^v�-V(�s���/�y���_a��~��^F�]�tl9k��Z�5��5N���G�(6�-=k�QJ�r��˰���'M�C�ozrfӎ�:)�Fh:�{]r�2������~���d���=�%߹o�k����`-��㭟�����1E�{���y�����xg|����0bO:
���D%�(/�k��7V��Z�\���@Z�F���h/G�!�6�.�s̏�s֨i���*���i���Z���?r1�tᐢb��q@�l�O��`�m
B&؟��.�؇c����W�L�/�Q�5��xv�r���Mv�Ɨm�Nzա�ߦ�{e�v�3��╶�u�j/��۵I�6ꘃ��;J�	A�����o�O�u2���ȘyJ�2�	��a��̀���U�k�-���9!G6�w�;�ϬP}>_����w^����V%/��B����&/�/��{�[���0�4D�8J.�?�ӷ�q�Ns�f�>kWf)���� ��ю�����gv��������'��́E^��g��K�L�	|���8ߙ~���M�K�>z:Z&#�	_ߢ?6���8���+.�/(�K�m���m�wr��b��)��O��(��_;���n�i]Im5��Nd��Q����yOiu�fN�.r{��f��.mYt�S����UKHm܎p�A��G�t��F@%(�l@5�cLl��_}c�)����[K�Xo�QՅ;�.����X�*�f�Wl��e��#��+6�=��}��Y��!^���6�9`ٳ��P�fڬglt�Y��zxc��zh'A�6��Ɏ&�6j�wjU�ʁ\�����y/4�,�7-�N����\���*|�闈~M�ߡ>����1`���)z:������v�nr\�e�����a�Q�����tś���6�À���?ie�QƯ����J�.'��b������v�,�[]!�Fv��=G^�|{6�L��(�:���b��|�&��C�1>��1�_�����ɶ%̪���ﰿ���?#�V��4�����B�F��z���d��S4G�g�Br�U��H	������cS�[�+�0�T����N�x:D�!���v��,���~?n��yމU=P��G�4�3L��U��,�;]�w§[�����{���)n�vx���Ȱ^�z:�p�!L6��8��eo�%����t|�^�1�L6e,���������QF�:��m�IM�M9b��%��N��#&]���#{�l��{!��Z�i�[������B`���I��GC<�O��v�!�MЗ�=23�5]��t]�kk����i����T!V>�*$��|ƕ,�&/�d��dڔ�'W��Ll�8��%�Fv2V)��b}ՄC�:Fx��+�2D�4q��h����TȮ&5j�w\c|�߹����
'Uz
�}Y�y5�Z��[���'~��OM���K�w��|��r<G��8�����9�h6A�|��	�~�^wA�_e���Bȳ;7z���7l̈́��J�u�O��(��_w���,9�ɡ%���ZW���9����z52���c�N�x:�6�]���5O|�5y(�Q����W���N��
�H�CV��*��T�&�U��] �ǂ��o��Bđ]���@<)��p5 �D��uڐ���;�L�
�)dE��*��P�=�y�A��ׅn"4�q�����C[ڰ��l�tW�����}��]z[�̵�1��\��&������߅B����
��`~U���;X��g��/��{���������S�X��������*jr|�������C��t��lʷ)$��)�W�(�7����&�Ú4��W�X��� ���.�;�M��Ğ��q���j]�����٪��7p��QE-�v���������ކxR'c��j��.��rg���u 5�d^�^dI�����{$0�.�����/�.��J��4|��u0.�m��=D�6��h�F��ǳ6�9hf���w��q����}Y��|?f�W�C��~Z~�;�����Ԭ�;�C�kL/S��<n��9��0�~p��ς�o�c�#{�W�]����97��ُ*�}��]�r�������KC�?�I+��2~u�,%�)�)5�iI���跰�fdGv��6rC�\�W���W,u�3���P��B\�4M88�p�FY��ho��3��၉Ge
����K����c��^��,�����_�^�z�W|��.��ޟJd ���~����߻�4��-��5�cd:h��c�|�𬉧5<UɺL��®Z�[֙�ͫ�EޫTߏ�¡$�p�C���?��y�ws�Y��.|�D�{ �M�m�}|��=3�~����;������ǌ�����p��ۮp��^�)*3w�p�e�n��M&@�o�a���%���VFe��HӺ[
����[RӖ�����M�݆,Ļo�"��=0�~��y�;�]#~�L��(�C|����Գq�J�\�O����V����"Q��'�/�,��K�K��@��w���B�7%K$��P�k��1c�vڒ�R"��m��0A����$@�:��d��u�lHtG�Y֩�W�Y��}I:T�Y8�|?֠�Vm�o�O�������޻�1	�M!�~[C���=��(���?�F���}��|��
�m�c�:�7�U������0�ξ ٯ�ʋ�W)֗��2����3[m�JRWbK�RhK�_	"�'�o!?����#�M���ԯ��)���^X�9����F<�߫$���ŭ ��P��n��6��A'����g���4 {[�@qFZ�Xx�Gʫ���RTc�f��Z�b� ���J����0�����	�X�����f���Fzu2���G�&Z��j�T�D%S�Θ~��U����w�dd�+��
#����{�I�c��O��^c&�|g�}��OM&��܅O!�����.���x&<��<]>_t�����8���&�_A�ʕ;S�"�;��)���Jʝ���"S�/X�����2~����Q��%i(Q]�l)��Y	B�e�QS]1�Ű.�جIL��m?���袭.�{h���[n�8i�V�]<��I8߅��kO_�ph�L�J���_�����_6�$r�}Ȁo�2�|��M���;���P�ׅ���^XH*�o�c���A�dda��*a���"�+�U�T�=M=�'P�1w+��{���W|���Xeʝ��ߡ_8�3���)��Fx�a|��C��a=B���I���������O����%߷з	�.��s
�9�M�K��Ɨi��+����/	�<L��|/���.Gc|O8�c�:r�+���M9䤩�X�r�H�#��U������Qm��.:�uTIT#q'6J ����WA�3���|�*���hCFR����;?������&��$Y�ͼd���E���P_���T�=*�S�W�~�v��e�}��z&Յ�EG&����!N5�.o��.�Ƽn����Zݫ����R��Ȯѹ���i�{I��K����;}��Ka����o��þ�����������������iD�xp�O�@�E�;?��Sv��I^�K��%�ɮӒ�e���ſ;��v����J�=_�6�^S�(T�lm��Mu�O��ꛦyЬΣڪ_��ϸ:/�$0I�_'I�<b��n�	C|U�����M������]��|=�T����˗d�BM��(�g^�W���L������*��S�NqE�w_��T�C\feh
���Z��Z�Ĕ�M���m��ݚ���M��;͹aߥʞ ���P������
g�����
��������|ЗD�ǎ�����{�77??7����>4P2�~; +�b�	&�RI
��Ӌ���%a�^~NJ��Q�o�νjۭ���9z�Շ���8hk�6��H݌��Hߍ��P�{�q�X'�u��'MӯV\�z�ڬҠF�����NF�b�U�NMa�6X֘~�W�JnTں�RK��������`����\�@<�AQ~Q�H&���ۃ��q_𽥐�l?�wa�!�\k(�>��o,c˪��k����Y��;w��^ݗߥ��w(����S�	|�.�g
�󓛠_�w���!���䷝�����7��7_o��OG�?���p�Ȝ�L,��3�s���B��|~�ﭒ�e����e��r�;��՘<���@�սH=���e６�X�4\U���hxPt"A4��(��H��c�c�K���>�\neu5���\EP
E(~de��_����a�<.�G�yRuO*�Q��&vd�A�w�z��+9�G��)�Wmҳ���d�T��liŒ���kf	>��*���S�'��C�Y�V�9�}I�R�X��0܃��
_�~U(�Y$ܸ��;iP�k﫮��k��)4�w�w?{N����`���?��adyW�����@��+	d?(f��|�
L��#�Ϩ�s�1��~�DO��������������hv�B}�	��Xn6���X�p/�>��p9ܿPO�Uȿ��6�Y�⯎V�d���5�������ۑ�)�X=HՃ�vT�G�6����)E|f{%4}]t���5���7Hd��&�M�i;d�$�&ܯvt��s]V߭�bn5)١g���O�ږ&���\ͻ��?JE��x������k<�\�|����"���Q�e��eҔI��N����]��)�8r���Ck#�6{'t�Bo���w�?�|�Q2���� �q |�d��|>�zd@	��;�O��*[�G�E���|X��_����,^��������4��L�3L��w�>Q��w�g�JS����,f�;F�Y�������V�~��{�6�i��v��ԏ�Qj��~l����	�L�s�Q���� �IbW��A�̒�f��z:WqI�`7��O-e�]b�N%���"��Pz�Le���	k�O��SM�~oɤ�3<���µ�DkY*�&�g@�V\y�S)�7"}32?V?��!���c�.�#���'���}Qܓ�-+�\�}|����-�@��د}s�wV�y�@�+������}�7Ȼ�������l��.�_$��~�\������3�	��3Y���L��b�a|.;zji4���r�eW_���@���@�yj�Hۏ��H��02b{/2�}=�D��=O��]'�I*Y��n��.Y4�02���.tt�{[j
���э����ӟ�xxu<�Zfğ�w���L��T��~�
�/�ܼ̚��sU������_��H�N�����A`}gH��M�YV_�]��;�He��!��ߝ�+,h��{������������G����o?�.�֍�-�y�<���ɛg��c����:�q��B��~��Y��7T����h�m��P�O_�����<����{ʆ�l��MO���>U�����@��2��¹*̸�&4�r��W�5~Q���eU��o1Ù�����~��L^L�_!��xgU�R��YqF�~J�]��6�D2�S��dN���/Pjh\��ݓ:�؍�^­���T�Gʞ�\}�1�]{�#?>��]Q������N����3:���XP�g\��r�_D�����gz�������Ǐ��w?_�����?_�8�|�菫�~�/����c��i2!_$���=X�d���'`Y������6�Y�⯎�-9zb뱥��Qq��c.�f��\���K����K�D���'��-wu;4����~�w��Y���=�H� �E�,%�!��~_ք�!@}F�|��e��v����]��%����X)��0ɔ)���Ʀd�YPy��E\hZ��^�&|�|S�~1;1��r�^/��=_�s��%9���O��д��wv�*@�L���;wP����X�&�����	�{Q��w|��`��zX^F��x��ϧ?>�����vN�m!�`N��7�i���M��s��ý�܀�����k3��Y�ⷋ��xz�%�eϪyV�h�z�U���f(�l�b�j��t���ڌ��gD��i��s�A|�I�rȾ�/KP�=O����L�k�=����b��^��`�>L��ߵNJ1%)�+���<'����=cm3P�+y!��ϫ ��
dK;�j� OA-�s����X�d\��{U�^G�U�9A�>޵��)M�4�ϓ��q�e�/Q�.��A�R����U��L�N����2������J��
~젷���.y^�G��<�|}|{�{{���P��-���}֨�0��b�js��uj���r�\�� +�_�3��b�]�W�3(��yVٳ+�Uu͊�e�\
�R�g)��\���6�Yb7��c����%�����A��!��$&0wS'W�\�=��~�:���_�����i�q�$��C&�U9w�W&|�)'�\U��:돜������B�>S�N�|����b(��B��w�x��֛h{�T�N����@�O�2IO]�@U�ei �<�d���%�R~(C#���1��{�0��,[�����%�{�{�r��싯k������' ����:������xX�N�`NP�l�5��b������]��>�Y�~��^!��]aY��Y�B%M�(�a���b$������u�,�39
w/�^��US��C�x�����rŮI��S	�р��L�O|
��|�����q鼸Y����y����U�ҼIU<\���j�]�V$��h�~4��/��r"vJB��Q��G[hw��wH���x�Q|����瑮i*��P��l�6� .��+8�Ϝy�E ���U"^���,I7e��!���;��.���o�k/w/Ϭ��������W�5���q���ݪ�5���o�M9��X�Gf(L�tєq�KC�H���Y�bu�k\�1K��[	�Z��B���TI�\PC^�U�g(�I���ځщ��-�6���K�[F^5�h@�{��iR*]��mW�Yº�wY#���O�����LU���!�9�ֱ�gY�ΆW�?A���`f����Y�W�ѿ^�Ϸ,a1�;��M���_��6�\B�m4��=�����sN��ض��L��}dh#S�����ڙ���ꉩ�9�'�;ʹ�\E��D��_b庤\g�u�|M��x]������=���y���+%;�����������v�?�i�|�C��7�m����(�Va����q�/O�E{^����N��,f�WGVr��1�t��=�j��H�Z�,gR\�^���m ���>Na��W"�~6���č�SFv�4�D�����;��m�w~��m�g��UhEW��|?�/e�ɠ�d�ij��W��+�
�������y����NزM�u���ʲ�|��}u5�{��)6�xsm���6�0&}���2��g��5d�W��P��uP�#];��sG?w�׸
�/�q�W��%־&�H�
�r���kGۯM��{�B�e���[�?^���~<�=m����C�s�m���M���d��G0|�V����	�/4�Y}f������{�Я�^#qk%#�Đ�}�-d�#����=��y�X
�c���y؉�]�fYM�j�Q�.���|�q���n
��C�^�9(���m:�6�Ƌ��|���^��.�|�^�3A��� ۛ�E�M���j����������zF/��+dcm��=��y<L��M:v�� :6��$�����8d%�}I*���P�<6��%��N�����r��� �{@d��.��u��ϔ���N3z:�z�{x�ݶl�"������P�f�*���"'g�rFs6�D�D�f�L�e�\��"��}����_�cF�;v�:%߯�A=��[M�,��
q ws�sT�C�=r�9�X(�M�\[)2kX���PC#�(�~���5�}��w��'C`�[R���7`9��\"A�Rq���x��c�������{w|�~;)Ѓ�g6���}4�,+"��$�ϒ���MMn��kulk�U���D[Kx���sx�]��U��vF^thX����Cd���G��ĮXaWx��-��1��]x�l��s�S�?��Vhc�2�����!���nW׾ϥ�m��́?���m]����k��\�3[E2�_�j�=�4S|���dV��,~���x��c�-�A%�AP�Zl�3-*sn�Md��E%|y�]�w��]Ӷ7�V��2�Ȯ�v�T�M?��{
���r}�_7����|_`Ύ������S��ɯ���}<�I�x�蕢��u�p��^g�lgح��"��|S�ۖ��ث��u�O�YE�[ho4�~��,�w�������~���]�,�`|g����G���,�yFy�w>�{n|��ou|��,
���_z���&|��=���֒=|������븶�M2f��=�❀xgy�� ���N��,f�W���P���=ja���y�
�us��hjȍQ��>��NٷlG�"l���@V�X3tZ�����~XG]L�������E��ʵ�	v>�`��lJ'cMS�3Ss�9���>1������j�PV�}J��3��$G<�����w�|��Bn#ş�h��9�6�W�#�kS��M�]z_�4�;t��E�8�yl�[$}h�9+V8�{��z������������ ���y�*���yc�ש����p&���y��,)��g|��,~�8O)�m+u��s� ��Q��Wc��=.	A��dA�Sy�&(����)��
^�����P��儜d�v	����[ŗ52��MK���ƀ��;��mx5�̔>��rx�U.�)��3��͘N���|��܄�N�wu���}������0�|'�M��|�Ҙ��4ߏ�ƾG �>��)�6��3�S4o��l��O*�{`Z��Go���y%��dY�LP@���(x��轷2eHI�(u��DL��u63��}���YU �g���@ $�P����ɓ�b9��8?�6�'~�����!�Y,�<�k��5��U�����pՌ;�����,Q�X"ɍ����ϙ1zd�R�[]߰ɉx�/9o�B��+��6����}[x�q�s�ԑ*��b-��	�N�*	/��	��dA�cE\g���<�w�� �� s�d�gr���@.���Z6�W���Me�ll��-w��w����P����\��ߑ�M�w~?�����7�xm�6�w�W)�oC@���m�SX��O��&���\�M�����AȽW�#?B RX�{�y��c^M"����j�����?���8F�Y���D��c3O;i�J�V���c��v�%�4����C�Nʻ#�!���+���Ô��M7�/���\<w�}C�R��u��Z�ݪ���fwS�ˤ��TnL�Y�XZl�6aE��5���զ|�K�������ֲ�������]<�@�_���~����տ�3xsu~����;�o��8\��/$<�x��#�X���}��}���<���?ϸ_'�_�L�}�ʁ��̔���ܥ�N\�J���.�k�^�z��RG�\#sͮo�Cw�П��85���g�<����xw»S���i"�D�,���%������4U���6������r��|��/}���v5����2��e�U�Z�>�g��v�`h]������kV�x�L$�FL�=�x����:R�MG������pߓɸ�}�26,}�R6yS�vU����{�U��qK�*��I���ժx���s��n0O��숕�x�և�Ղ�3����b�s?/@�S�[��P�=+���y/�.`8�xy�
�|�8bEʿ���z����H��kQ��B��MC{���T�%�]�e�;�(ߝ���r��"���Q&�9�M��K�<2V=�6�W��MH�0�_x�-�]C?����K���"�o-�<�H�p~}g:�*(����L����Eؓ��ڊ����{�LI2���<������l��&��Ҟ%P�_�����V��$#Y8�߄��9<�;�'����V������w���#�w��G��9�ܗ ���q�3��=�'��^%K7F �G�g�7��:��^�z���9s�v�� �#������ �|o�C9�q*:c�^��Y�u��<�î��(���Hu�<����m�x ��!�V�=��mR�k���?S��O|�[�+Iɾ��W���.�jJ�>��,~�N�{����-�3r��dʽB|ޚ��M���.� ��M�2�s����������L!�/ �-cX_�3)"��w����3�Rp<����E�s��bNS���c�ȤJ˔n;�����w���w���|�W�^�z�9Fꘙk����(�"�'�5��^����od	qG�ww��]�����d{�a6U�r��}3l��ɗ	�}����ҡEvu�g�����?C�ʟA[f�@5��aO�a(�B��>���PtA����ȧ���n����]f����}���֊��!߷P�����e&\��S����׎١|���~�O�Ϛ0��#b�$����w����yC�B��� n�>�q?N��${��������{��2�C�]�#�+[�.��MT��|�W�^������s
&�3��~��Q�B�{gșK�=⓮��hˑՍu?o ߧ��1M�nh���7��0�ׁ�__�r�ߥ��@$�����U���-NQ�WdgOfL�+���������;X41�6i���\�3��G6`�[N��0��竛��et���T��C<O����3�wE;�M�È���lP�Z4��T��A�f�w�
�ߥ���mt��S�;#��+ؔ�p�i�=�@�9���4��|�˫�yg7W�7O�~��|�W�^�����K	�ٽ���$�(�G��a��{�=%o |�F\��~_��,ֽ\�'b�,�F*�����Jo���ބk���q�w�����{����-��w�:b����W0����)�U��5��!��s���n�y~F.�3e?0<R�O�J���;Fk[���#��%�5����{���М��p�2gF�G��Z��:���#>�]��WU���W�F���_gP���',��n��e�}~�	$<4����g�U�����bSKl3����ħ�w�!�=[y �f/lx�>�b໱�i��9q��`I�:��J��������~o�dW+�W2���r�v���"��o[���Wį�l��P|+����3��̢�J��O����}�Y�S�}�ҷu�N�urd��rg�p�n]/𝝯��M�Yr��3W��	P�� zwv�	�	����;\���}��~���S�ׁ�m�AQ	�!{V.�씕U�$,<S� ߱��{�����j`��_�\7����}��]����ԕ{���N����2f�8�Ǿ�����D���w��;��n���#��������e��80$��u���	��|sd��_UȚB�{�W��KEO����pOU!�X�Y4Uj>���H��U���xЊ�;�}V�}Ǣ|�w4B����D��zȝ��k�~����v"5�q��I���kb�$�w�,y�B�Z�n�"%��8��.����8��^���S���g|?סf,�?�}�?3�g�p/,������\Z�k���Yv�]�	�����S��rfˉ�tr�����Bc��Q���h�xkL�L�=�!��X���e�����/s��xf7�t�ߤ�3����p'	ҧ-涋�;d�C�Ti[�����=����m6 ���w)-g�V��o|g�sKD���ښYں|ߵ����HC���m����	�(�Q�?��|���ݐ����;����U���\_���c���?���=|YFh~�_B_�x*����B�d���V�d$�{q������]�h��o�{�)�+�V+v�v�w/&�@�v��f$�#ޞ�(�$�Ym�;�4�J��C��S�w��2�8���v_&�m���N��B�;wT�s�O6;E���qM�am3PKP��O�[�GvK�������M�w��}K�w�ƾF5r���i��&��r��M�{$<�3�~���g�u	Ļ��=���&7ߏ *�cO��!R�?n��*��}���\�3�{�ɳ���L�c�}w��	�������[yHŻ���e�����ό��]_��v��;�����X�||'Ɛ��$L��Q#�ja�u@>y
��k�������m���%��&�3����uE�T�f'{:8��:9�)�E��Mpo��'��l��{T�T��ꂒ�B���cYQ>��.���m��1�mM�j�"�=�o�ܙ)��W���#�;�f�wR��0܃�G�Rq�j�7�������;�gv�o�Ô|H�Ä�y�^��@�t��_�3�B>��3<��� �M��z��E�Y��zÛ��s{�7��{��ߎ�Vl����ycca,Cc�q��11��D��:jl��l�;6����¿�HΎ����9��L�mp��;"���={H�cC:2�C��k�J�;����~�;K��ʽ��E���{�?3W�̺P����)�ms[����&�v�#�\&�M�^��W�~��Ñ�~G��-�-c0�I�-ֲ"����=�4/ˁw���P�ܗ�b�/<�@y�S�ס@���+W���\]h	N/ܲ�{PH�ڟ�W�^�ꄆ
��8��7y��u�x����,PW��=�%g��s.��~G��ם��ܾ�������x��c��lA>����i������6�|OdB9K_�+��P�_��Đ��������� 3�sO�"�{�܋N���U�/��u�r��[�w�c�wCޣ�1�x���)�_=5�~o]1��Kͪ���G�6g�y�����-b?A�ϰ�{~g�="��7�[��������!mP��I�k��	O���W���9Sf��ջb���ٮ�^�z�ȵ�����V�?C�>���L�+L���d�>3゠1�g�����1!^����ߡ|�ބ�~���ܗW����G���Jp������&z�,�H�~bߏt�@'{hѬu {���'r�.X(g}�-�f5�c>���;��c|�w�g,c�7���*�n�w��Nu�X����eG��X��E�̼?�#^���F�<��~S�?�[�g����n�����e��t�y��)ܟv�7��W�J����g���
�B�|��_���z���4�טEc%�{����5��iQU���#X_�:���-�a$�1�~��5�h3�?,ܿ�BJ�+���^�LX?pq�*����[������ҙA�L�$<���F��J��L��&��rZ����"$�NS��=��"��>V[3C�D�o#�� �#��e��J���ueX��[���0������������	DH������A�/�E������[��tp>��_WJ���^<�,���r,_y�z
W5��U�����/�Xfl;��g�Z}���:m��T4��B�w�c�];�u�i�!�D�ߴC�3;k��6Vpl�.Dhn3�N�:�Yi�%�!�N(��2��|�ߥ|?�ȉ.��U|��Y�fU�[0*o-����#���{5�+
���� �N��
�3��c(,����>�sWw�"���T�3���tÛP�c�X�����ȿ3�}��e��K�mظ������_{߷��W�@��?���e��<bi0n(��/�h����סx]�׮W�^��ꫪ���4#��"��=R�@	\%��vWІ���@��VϦ|�|K7-���/�����c���w�p?�ms߶��U�G[P	�b�6��j�$#��[d��]+��	�}r�|��
D$���]8�\��I�t^+&Və�<�΋�}p��3��Ά�m����1���9���w��T��E?p�O���v�?s�G������>�|����������2���)��Y?��i����3����@�;�}�G5��U���8OS=$<D�m��C����F7����3Yn#I�pFD���X�� HqI�7p)I%Q"EJ*���b}����Ǜ�{d&@u����t���������o��Z�Κz�%�P�;$�X��(�<?�Ô�=�԰o��`�a�/���+�~���Jbf����y&�z.�w,4�]l�z�\=�U�����-��7����m�/q�R�����g(��D<�}`���o�����,2b�0VN�t�'W}i�I�����Ԗ�~��O[��0�B���w����+�75��*�S��M�k��PW�n�?w���'ӧp��O�x��.�;�Z��J����B?�Qp�3���A����7�������$�|�-�(�Ѱ�jI����z��Zf�"}�{�&Q"g�э�^�~�� a�
�V�`h�=��~ZU���]�#}Y'���M Ñ������M���q��/a�=O}�pW���/�;���U�#��ҭ�bD��̇g&�;��}-O��Y���@����8���Ğ5��;�IW���󄼱P���ʟQE����~�������2�������=H��ؔo[槎����|���.xKp�����	sF*%�m�ޖ? �?⡏��g�@��w� W���7���������{��㠓��V<\I:������wԨG�!��RРa�d��+��A$M����G���yC��O����m�_m��@����m����U�������wP��g�r�� %g(�9�!� ΃(b|^�8-+�v���{)/�h��|��P�j{=��z��H9��D��HWr�7&;Ѱ����h��&-�;��\�9�5���.�	`5Z�7���M
ᑯa��a�-��C�q��q��8�c¿X'l��ȧ%۽$��r�U�5���/uvS���J�WUՋ����~O�����R+a��~c�}#�����#�Ƃ@I=�"�/p�0P�u�����k֫5���tľ�������f��m+"X����0C�k�|�����U���P���Fh@��<���8\��)m�;� �������̰"~ ����:(�&[��VK�_5f�њv<������t=�����7����"�5q���
�㖓��e�߯�"� z�?���������]���j=�u�S�2�_�Db�o;K3��;M��T\��3��~���W��*�WUՋ���{N�9)N����������64���X�J����(��@�<��|�^���(cA���j�i�o�w���2G�H�����l�,#���~Sǐ�y2��'g �<��U�8���$<�N���z ��15�Y��k"օ��eE��T�g�:�x_��������b�#e�����t�)ͷA�_�a��X���#K��W|7|�'>��jq�	Ol�3B����.��c��X��/#.��o���o��~,�v��Mx����;ͯ���}��_28�q��yJϒ��UU��j��g�<�8���4�~��������dB�)	�$�Kq�&�J�3�?H�Z�E��j��Q�x%ر�v�w��ӡ�t����U����;����1�MHu�_���D=��y(_��uĮ�#���P}�p�+�*ڳk��s_�h)+���;-��)4�w1?t�4&[-�7��k��:y3�N7Q����T:�鱦�h����������2��p��-�W����_@�C�;������ ��?���q?v�G��H�����c����q�^��>����j�8�I� �J�eB/zW|���W�hN��p���μN�G�[����lL�	�WI�#YS�R5	�F`�R�E��K\#�$QӦX���w��m�$�4�am�O����͜�L�T[ppudʂ�{�3�~ɜM7�|��X>��]G>��=G��\�O0E�U\�n�𛕥�&;`]�}`p9ϊAx\�O"�{�����D�|?ې.ץˎt�C>R9ߍ���Փ����Ϝ��j��O�,�#����}��>��*��O �u�/C�~h����!�3$p�	�#	�M����5��5���1M��;�)��|O��~����W��v݇W㈏�v���N�l��F�eC�I�'Y��jj�\�s�s��1�DZZ��6�����Yc�����|7��Z�8f.5n����	P��,f��6��c[U��&f�\�_q��kW���]����5VX9�E�L�	L����Hm�_�������d�Aw;�`(�Jo���T��yFN����������;�w���t&��j�97U�ߴ�;83�/���3࿿��#H�|�}�q]�л�������tH�����v����5K���%X��_Rz��8�ӊ�UU��?A��?S�zd7Sw���fkE�wXڣi��P�DK5vՆo�F����j�(	��m��;c�jD��Ǟ|�����;pS{�x�4H{K|��>re�;X4�� ��yĹ/�꾫���uؐb��,b�Z%�q�)_a�������"�Y'���ndd�%���р�H�}�NN|zl2�#.�5���&�����bb�	�仕�8�E�/�h
���������A�i�"�g0g ��ǉ�ۡ�u�|JW�J��?��bK|��M���;�����z�����d���Vꚩo�"��Z��U�뵦�6�255��8�"W���8@��m3���tYc5��!�:�v����E]��٪#O���a1.65`A���e<��v,e&�gD5d��E@|Xn����-S��w�r�	2���_J~�%<gz~�)�_��t�&롴U��Z��tܖ�3r�#����|S���O5-����.�ܗ��+�]�M��&�/����mW�������k�!q�~�
G����ߎsi���h��Y���Y-��R�_&t�T�LUU��J;���"�
l3v�ķ��L"#��(U�D�B-���Q=KuM5��Nt�(�|�4]C�Mͷ4�U�@B%��D^�F�F��C�}lp�����y �H�n��b�ăͦ���$ϟ���6G������H��p/"
���w4mV�8I�I�ٌ��L�ϤY"��С�:;ԔCE����t��z�.����y/��J���*�_}6?#�;�����
Y=3z@��+��?�����4�n{0���ￊ�����<���|�����|?��^UU/�c�w�?+�gr=9F��G��S=��ok�w-A�w���g��	�-���:Jb�w� G����
�'|��|��h��/�|�CD�.���U��v(&F�[�/�-�]�w��$���;�����}ױI_�t�'�}��}��Lv��3�'�/�gN��;�S.LZ��2/�;X�89���-�3�s(h��#��~4�,���z��w��߁$h����T�28�L�M�U�߹x?��I5?SUU/��Ա���Q�G�!�cG�-r��E���BK�B7U�r��.��8�|�4=C� �Hy =|���J�B��9�����������ꁋ|�.�y�\���o0"��#N����Er$m/5Z�\�;�]�H��>it��u�n�tצ{�7�L�Ty��9S����|/���z��ȟ���R��uP���E�L��>��&n9I?N�
����ܒ��^,1�=�;���=��Ȕ��]����UU�2�|�d�@<�������?[�pL�a8���a؋������g��7A��n*]K������/9�;:�G����9*V��!X4g���ǡ8�a1��:Z�{;?���V-_qZ�ߴ0���+N�r�O8��i���6���@Uf
��C��/�6ʮ��'2�Ý�ib�?���&[���U0g1	��y>A�}l�Uկ`�s�?������8%�q�����g�3(�a22wi�ta��7�r�EJNӊ�UU��꿁�v�w4gP��r�O������|�"�=c����Jd)m[��� �e��\��Ӎ�"?r�����������x�ɇ8�d�C���:�=��HyZ|.�
+�#i�w4g0����|7�Q��s�~Z�39�̌���/J��b9߹r��{q|U~���kp�鶍|߄��7�ӹx��B���#g��͂���#���_g8���U�{F������QUU/����]Hxtf̘���c�H|����y\« [�]|�E	�k�����.�\��QZ�����>1EU���� �{�70Wr"
X>B2D<�yh�����xa<���˞j.���ߋ⸇H&�i�WD�����ʤ[�5�I6�J���~�7���4g4�w�c}n������{~�����u�~�p����W������|5�N����Hwx�,��e���&�e߫�����q�ϠE��rg�#��Gld������ǝ �� �-��]�9ߡ먾�Ʈ�|7!�`m�	
�U�;�{�{l��$�?�c��s_���8�ws�Φ��U�wa����+�}9� i��byMy��jtj��n���Cl2��! ^��;�|/��r����r��`ј��_�g��6V�xS�/ր�ǻ��|/��4'p�s!L��������tv��ty�ȟ�*&/��9��UƮ�e��{UU����Ա��L�p���oƁ�O�/�8�E�{��S��a��gX�x���U�t��S:��s����������M��[���¢Q���4/S��}�cl�8��_��>SLC."�5��/�#��H�����w�{*	�ݠ�l���݀�d�#�.�/�=�t��J��~��?s��0/��"_�����.��2�g�0��}W�1C�����%��p9��	�m�|�|�-�j����9ߑ�%�Q�_�䫚|�������VUUU�����&�oJ8���g&��fi�'�4�����j�=��A�|?�,������-��|P��xj#TV#e5PG���(뎺�@|�ϴp����"�y��g��y��8��+?��j��b2_q*,��S��3D~$�k�	@����:��p$�	�^Lf19�ț�����)G��w໦��M w��|�s�����X��8���܅������c?q ���y�&?8�O�}�0����3[j#�p�S�����JH��j�`@H ��b1H�1W�s;3�s9'OU��22*������w֟޶K��� �?�G��LÙb>s�g?�0b��~����W�	���*P��B[5t�ȕCW*o�K��$hU����y�e9�j+2 ����D"�������s{��˦@���!�iBa�\V�TS�N8Q�#�Hx~� ��������5�l��[���[�L&}Xn����+}8�˳���:g*GM��]�D�{h7B!s����m8�z(qo�g�?R���|�����#�7�Ζ�w���w�3�|f
����9C�^gN���ɗ�h��i��#i1��T��*���k�[�17I"�G1w�^ퟩ���W�Y�{h����4e,vը�CZ
l�7K#�[�K����m���R�[������ `�����E���S��|��]�g������aJ��CV�z��c�7�=^���O�L���Ml���̢)�g*��ߡ��?�@$<���x���C����Q�G̱��&�)4_�����z$Go�]�~,�
����ϰ����2d��]�i��#���٠1,���聾f��[��"y��ɻx���!M)��� ��'!,����պ��|%�|7�����c[O=v��QB���)A<Hx�[�~�#˶`>#۪lk��˖~��%��YȍC�������ʯ����o|Ϩ���tDp�E���Qa �O)O�����b�L �vv��1��~���/w�ߗdv�`�<����2�Ϙ�s��5�'�������j��*�[V�/^�B�d�ߤ������^-7Q	_�GB�l��ߏ��Qc6��/���/C\4D�����-U|o�Y�q�RP�#���ϰ��#���q����ZW]�����@�Ó�݈l#�t��� w��)��Y2A<�� ��=?��~/�n�pk��ߵ��˟G�����A�έi����ojQ�^�hVT�LY�H��:K�;������ȷ�L���%�aJS�w�d���30�/�����#���Lv��{1>HQ/e3��i����H�(�e:�OU��X�� ��m�PFh��*���'�'��b�?r��g�����<8�Mo����d:l���1�@@�t����
k�?���ȓ�=�F{죁��<t��|���OW��^�el"ۍ�c�.i�ᨉ�āFb�A ��7��Һ��	�ȱ]]u�D�칲�K^ ����Û��w�q	���-G�0�u��4�5]��q�*�	�T��­���C�� jfG���]�,F>l�em��ʶ6�ۭ�hL���qS��CN߬��Uwe����T!ݬ#��&�����~�{)d�Ɍrf�d�1s�c����$�����G�~,��9R<�������
4B���O+�Үޯ�xg/#�gZ���������������nv�0;;��W��$Ґ���t�S���ߜ�@y�< �~�'&|��W�������ﱥ��]#��̳����Fhi�&��|��Dc)�a��苁���k{�F�O�Fr�HACh7�����d�����ʹa$|w�mG�f�L�E�/�܂���Ƶ5�A�n	;&A<�X�wM�=��N$��~3�����9������\��W��.�D��3-j��.���@���YU�+�fpۮ�#�R�8�N��Y��;����Z��p��g�z�{���±F�r�ʧ�r�)c]>7�3]:7�K�!N,��'�0q�KO�
���'��H���?cx�4���������C��~�m�������_���>�u���I�_Nٿ���~���]<��}�$�޶��&w��e�ś�t��S�!Lbnsg1;���$����iu�U��u�{�w-2t��8VóS�J}3	�8R����R�%�D�;�Y���.�Ahێ�9��z�	N�96O٣����Kh��~��Q��X�7C�ԅUF4+t�)���M�o�էu�ߤ^�m��<}BW�V�����A��ƥ~/2��%<8#�2��������]��胊g�4D���l�/��Q$b�4Eg-4��E��#|js'�@h~�Wa01͡"�Ȥ�SUi�H'�ǆ|��<���[�	�]���/<�ː�
�눿�y��������4�r��������uN������p��>�W����:���ۜ�C�&{�A�J�_�+��@� ��;����Dx�:����>_�WP�	��صS��;�FdD�L�n7i'CN�:�d�rh�m��=߷��-[q�Jx3�fu����~-2w+����Q���&L`�|����$�!q���Y�4���6ժ�KȌ��xz�)��3U80�E�߫�|R�LP���
�P}Y$tE�c�uq�V����{�@hx�@��9K�Q����/�I[t������p,�E�;uF�����!}�Z��G��*׆�;7	����n��/������)?��f�������l�����|ҟ�a�κ�ܯ07�*����?Sx��g`�>�ɑ��apy��C�u~d]u}�����&ổxN���I#;�͸Q�[9C�m��	�y�{`;��ۮbG���F�o^��n���:s��n��8�w�����������{�a��K����'zЃ�{���)��[tIV��9�I�4��P�{�f���˫܆&�Xڞ���ڡ-�=�y�iʌ	�Cfh���U|��^�g����\�o���H�͍��w��;G�|��f����=_���g�����O���Ӗ���H���ѯ&���.������ME��(����P/@������>_M	��Bc���m�n��idƉꧼC��#3g@�Ǽ�ɡ�瞗{A`��l_6�xob�{�WRv�b���� ~�����%m�~�|W�?C��J�^�a�AS���@����"5;�b���e����߶X�p�λx���U~C��[���Xzϖ�\�����8bN�W���#��������f��$x���]��������6�;}�����-x���a>���� ���aF������B�����-#���	�5eM�%Z>�����3������x���5�����E�$|Ol+�ܖ����,��X�M9�x���9����5<���c;������&kf����c�<a	��6���h����c�������o�|%ދ&�抣}KtVC�*��eD�G����&��ŋeDA��4�w�GV|'r~I�S�ty���m����~������y<4P��,�mg��#M��~�~'ϒ�EJ��a�����;�r��� ���+C����<����������������}a B2G�-��l�7-�W��ħ��Q�����,��!:��;:�k��Uק��጑�v������N3�|�A"�	o���r^ Şށ��~�:��;��d,ờJa�l����C�n]���#~7�-�_��U��E.�VXӂ�"tJ/r9��,Pַ��&��`�(o��L��Y��6�����?����}���P
!O7WE�,��Ԗ��YƁm�l�o+[8�i�'fB$|��5�X��m��F�{yy�X��b w�L�,�#Y8�M��5���.|�^���a4��lC?���������qN��'�i[!d����6���M�o�f�9���	�A�g �)��E�������?�Q?F���{]u}���� ���u2�~��۾ی	��8ТH���c�����b]�#|�}�4'�k�?������I��}A��`1�~]f|ϕvMq[�7T�;����g
���� ��+ _6���[� ����Z~Y��tS8g趂�`��.I���Ӡ1�K2���Ol���8t�#G8*���N=BIt�d&:��E�8Υ��/5;=�!CEV7�H������h��n�	g=B��tT�}��}����̮�_^���W��������6�f�Fӂ�-��6��79����W����$F��|����W^`��~�|�~\���P�=5"�Ȅ�v99���;�{�9��^.E-'�W����r���Ut��\��}K��Uj�Qa����)��¥T��9V�k�p����l�b:�+���["�M�����xQ)�^�#K���z�1����׸�=��U�.bfh�|���3���D��|�N��7����w����-n���x��k���k ��.�"<���_�3���=��`����%f���Os&3M�����F����L#��"ԋj��U�g,)��ހ��{@�ކW�V��W�5�d���^@n�oDf7��׋4'�Gm�նI��j��'|�f~o3k����������!���A��Q#|�4��B��)���*���{~�{���+��_�;����,R�/����w\�]��G��}S�k����Ct�3�)sj�a����as���0�a��L)���U�;�w�����M�	�	��{D�������eF��yz|�[}\a�6��f�%|/�^��)�q�U�������8*^�����a��z���>]��F��&����N�������j�+^���0�q2�ݐ�=t�X�����Ͷ�Ҷ�V��Z�����G��7�@�o`�_+,4�|�3�5��.e��
Yc�{�r%IU�%�Ǜ��U��3�gp^���j���J�CV��?�y��}�_�s�1���=��s�J{jW{�̮VZ]HH+��v��T���*��T�vȁr'��8���lcc.[����0�HpB $$� �$$���^��h!��Ⱦ����ޖj�}�ǯ��D\��h���w�i"ߛ���
��dO>ໃ���4?CΨ��*��Y�pא�~~&֕�q��L�cj��f��V�!��i;z��fi\��ٛ!��+(�e�E\�D����v��*��<��	��8r�#I�w.����q-c�;�K�E��IzUϭ$�R�*�f>�-�i����D�&$�J>g62�j�˦7�9eɩ��J|�b�*�a7�+�,��+�de���+�M74��갩�^
�BxM�ūQ�����26�c��A2��k��)E��dc
t�jcv�;3��VE����#>���7J��+���m�8�m4)ߛ����EG�S�Ȣ%.h>�w��|��|�n�M��hʶ ��wʎ.�����$}�C`������E��c��ǎZ=�%���w\4��N��G�kr�|�_��Y5dg�&Jvi	n���g���Z~�Ha�#��\\O�2��Y�̻V1���<�ͫ)�'/zf��K	����B8Q����B��ͺ�t2�q���葒�V���^�|7weԀ��^R�(��Lu�T�u�GSK:�|�P7غ`�P�Ly�TFLe�������Q���A���������hޠ�װ���H}4~5�S4~�����6����,iw�w���F�3~���o�FՀ��&-�W���)/Y�xɧ<����ߛ����^\��;��5��Lsu��յC�ˇ.�]Vg�ڌxtT<ԇ����4�aH�'q �=����vg�ID<Ͽsqm8�<�=k�9'�w�R��ג��w� ��]���\ԭ����R, ��d.����#���޲=XJV��-��S��7#ߛ����w��I�:�� ����՚���!��C7�I�Yl�D�C��RMCp�~�^<'���]�{��1��|�Q|�}���}SD�ԭ���w>ߕ�Z�+�s�ޮ����Lk=�i�HM�c]��������%�Y�7_=���#����9���v��<:.o�k����P��� ���6��V�A�w
wڜ@ZpŖK߱9�+�s�sqm<�u���k�!~Ozu��d��!�{Z%���WQ���e��Fڃ��- ��l5�[���D1ioq�>߷`㔀xx�N�t��}�k2�#ԠA�����T��T\��+c18CK�5	�^�wYKX1)�3���� ��tXe$+�a�g�z�<�1��u�7�M�n��2�p������v �����7����Z���S�.��w�,�߃��4��x,ץ�}�#���weH:>+�צ�T�5���|�yo��́S�R����34~�s�iW���;�;׆��9K��z>4��մ[O%j����I���+��p�JNKU�D]�*r�h���ެW�9^1��(���W��)��ŷ���z��Ƒ�~~f�
���彆R�����I_,ԏ9uk<4W���t\�@���Ў�1��0	����v�;�
v{���5�;�T��]l���Ƭm6-����i������'G�w���v%�������#ֱE��R��w%��;|?P!Gh�����O5��5޴�q	�~�)�N���Ʌ=��yS׶�?�g|��}���i����ŵq�s9�nP�[��Z�m��zک��=I��4��\���3U�E�+��M��=�s�Eͭ����(:��Q�{�P��V�] ���w���+UC)#���Wc!�{L��BӖ:Wf,���z�;.P
�^���Tً�훫�;`]���U��Z���ZY���<��[�|��|� ~7����iaw.��ر�N|��h[�P�m��,�d����������u�3߅�%\�
d_�$k��訰2��ޏ�Jǧ��͋k3x��e��>s�ζ�,�|O����!~��\l*)L��H.���4~G�<����@��-�{1i�P�׀[�[��B�/�4҉\��
�ӣ�f��({vэ�߳����}�3�Q�o�e���c�$����^]4�-1u,�N��Y[���YK�(~Ԕ�)��w�D2��?��k��;���b�;U��&g ~��|�!��h�ތQ��)����hh�2��آe|�SY�F������t�_U�K��}QXn��q�:��m��	�c�}�n�:B�kt�j��]�p�w�ٶ�4?#-v��tgm�|��3\\O��q-kc����%���j)���g<��}	��12=�DU��
��d����3�\լ䝒g��=Zn�cN��e����Lu�Pq]���w,��hر`�PFq�&u*��١�n�*���1eА�d��v����y��֣ʒ��|��v��a�3��i�]�o�Ȕ��b�ݏߛ���,~�
�,�U�ǵ1c!��i����,9Ӣ�3~}�N���� �%�?M����qA8X��v��ڌp|NX��!��
�M,����Ar�B����8(~G���J�,�w,~��.L{d�x~��k�g3�Yt��ߓn�Z%���k4z%m�+�DU�ʡ|ά�힔�M[.�=����|"��'6���آ|���U仟Wi����ߣ4�|7��[L���E�n�0 ���a���.W���:���A	M����;F�`�*�]S�v�ߛ1�i`�LS���m�_4"�f{ΰ�M!�22�{W�;��c
�%�����=%C��{A8PV�0`_�����-�W���z��{E�l��6��$�Y�{'�����#=�׿sqm@��������HZ?SI9��x�5�	��1Ji3Y�8�-��Y���v+�����p��,�ل��cc��+:Pb�	�ߝ����X��_�|א�(.n*`�\ъ|7��1��U����Y�T\	��J�u�(K�"���&���"�v&ht���(+�ĝC����t'?�A��b��1Z?��w��6ܥ/�ڊ5ì���'����v~&�����O��֯2��q�=�w�e����5���xs;������|��wq��,n�|8߹�6���8�{�.yn����)�M���c2��NP�g2F!ϻ����]�x=F.�V�6G��<P����d����:X����g�����{D)S��ʨ�7T�,u�V�m�i�s�����K�j��~���-�rt�`[l��2���h�����w3Ԣ뛶ŕ]9Z�ZaC��	̊#C�we�+?��w��{"��= ���`}�(.׀︃�ڬ��$k���g���s5'�&�wv/^�Hˣ��L'�dR���W��6��~��o�I뛒���F���r�W	%��W���&n���%����:=�jf��w��M��w�C똟i�}.���]|Wjz��(�u��ue�T0x��y����bXBSA|#�ۥ��s�^�
�����o�+g ~Ǜ�>�G|���8�=��#m�P�7�ȝ�bX~&��O@#z�����k��,x������e��y�@YB���ƅ�i�;Y��p��wl=6@����M,��K$������v�ٞ��6�a|'��\\Pf&�e,��'���)�=���ͬ��5ټ�V�DE�
�d�H9f�6���e4�u�F��<'c�F��<��0�ɮlh�w�E_p��a����"��i@S���,�=���St��&C�h�a���w�lU.�	ز������]Ŋmgp ����}<�{��wC������D������ζm�UN~��|g�;����}G\a�gv�pY�ڙ ��˩���rCX��3���8�}}�$�d�=	h�+��l��n��w�D�\LB�'�Ns��0�&S�x��k��q��K��?�QਂC�.�u�����8��`� �`Ͽ.�48΂s�<� .��p�_�g�z:��׳>���ӧ�g~D|W�>�����>�;��x����$D�Gzi��ٟ�ѿ�u�/������w�y�G���� x��q��?�x���L�w�u�~��|���t��eo,����� k�8����Sp�	�s�{�\��	�C��_����%:����+t~��_�����u:��ߤ���m:��ߥ����}:?
s ��I>�|�|:��?~�?~���|�?�CAv��< �|�|:9M>��!��$������>�~�o��?@>���;�)�?L����c��y�	>e
U"a8!Q�5x"1�l�9�K�X�&ąϫJ�$E�p̐,��1O
��)�8VH���X'�l���l"���O��w�|������o�������M�o��;����~��������[��#��?��'�?�~�����������>�0�#���?��q�'�����������|�~��ς?>�<��A�C�/��������_?�*�k௃��&�[�o���.�{��� ����>~��?�!���~�7��O������~��?~�O�� ~��_�/�_�+����~Uf߹���m�^9��5�ƽ���i����@�A�7�������t�_ �OE���pqqqq�T������������.���؋�<���[Q��o �"/9���[Q��o �"_9���[Q�0������Ϟ=��3�<��CW_}u6�]ܕ��;��8y�d�X��뮹�'N��$I�kø��˯���k�2�MDP���������8��5{��&�+���]�������^������=����z+�����է�~z�o~�၁��5�^z��/����#�<r��w�����UU��
����?�>�|��p�*��o�l�fT"5�w^v�e�+_z饥���n���{���;�����z�bzꩧ^����q�����ǿ �u�]w1��r�'�|�}��/���d���F�S{� H�p�̙3�+O�>
�Z����t7�g��\\\\K�7�x#���_<y��'^x��5�\r	\s�UW�ϼ�꫷�v�7���O�O��|��������̣��R����϶����+_3���7qqqq�i1�>��c��{�X��9uꔦi��zk��ٳg��<\v�-��O�!�Av�