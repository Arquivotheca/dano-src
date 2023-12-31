t32 *sizes, int32 n)
{
	while (--n >= 0) {
		if (*ptrs)
			testfree (*ptrs, *sizes);
		*ptrs++ = NULL;
		*sizes++ = 0;
	}
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (gpoolptr, &n));
	printf ("Nodes remaining on free list: %d\n", n);
}

void
freerndandcheck (void **ptrs, int32 *sizes, int32 nentries)
{
	register int	i, n;

	n = nentries;
	while (--n >= 0) {
		i = random () % nentries;
		if (!sizes[i]) {
			if (i & 1) {
				do {
					if (i <= 0)		i = nentries;
				} while (!sizes[--i]);
			} else {
				do {
					if (i >= nentries - 1)	i = -1;
				} while (!sizes[++i]);
			}
		}
		if (sizes[i] > 0)
			testfree (ptrs[i], sizes[i]);
		ptrs[i] = NULL;
		sizes[i] = 0;
	}
gDEBUG = TRUE;
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (gpoolptr, &nentries));
	printf ("Nodes remaining on free list: %d\n", nentries);
gDEBUG = FALSE;
}


/*****************************************************************************
 * Allocation logic.
 */
void *
testalloc (int32 size, uint32 carebits, uint32 statebits)
{
	BMemSpec	ms;
	status_t	retval;

//printf ("Attempting allocation of %d bytes (0x%08lx : 0x%08lx)\n",
// size, carebits, statebits);
	memset (&ms, 0, sizeof (ms));
	ms.ms_MR.mr_Size	= size;
	ms.ms_MR.mr_Owner	= 0;
	ms.ms_PoolID		= gpoolptr->pi_PoolID;
	ms.ms_AddrCareBits	= carebits;
	ms.ms_AddrStateBits	= statebits;
	ms.ms_AllocFlags	= 0;
	if ((retval = (gpm->gpm_AllocByMemSpec) (&ms)) < 0) {
//printf ("AllocByMemSpec() failed with %d (0x%08lx)\n",
// retval, retval);
		return (NULL);
	} else
		return ((void *) ((uint32) gpoolptr->pi_Pool +
				  ms.ms_MR.mr_Offset));
}

void
testfree (void *ptr, int32 size)
{
	BMemSpec	ms;
	status_t	retval;

	memset (&ms, 0, sizeof (ms));
	ms.ms_MR.mr_Offset	= (uint32) ptr - (uint32) gpoolptr->pi_Pool;
	ms.ms_PoolID		= gpoolptr->pi_PoolID;
	if ((retval = (gpm->gpm_FreeByMemSpec) (&ms)) < 0)
		printf ("FreeByMemSpec() failed: %d (0x%08lx)\n",
			retval, retval);
}




status_t
initmem (uint32 size)
{
	register int	i;
	status_t	retval;
	void		*poolmem;

	gpm = modules[0];
printf ("Initializing genpool module...\n");
	if ((retval = (gpm->gpm_ModuleInfo.std_ops) (B_MODULE_INIT)) < 0)
		return (retval);

	size += B_PAGE_SIZE - 1;
	size &= ~(B_PAGE_SIZE - 1);
printf ("Creating pool area...\n");
	if ((retval = create_area ("test pool area",
				   &poolmem,
				   B_ANY_ADDRESS,
				   size,
				   B_NO_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err0;	/*  Look down  */
	pool_aid = retval;
	
	if (!(gpoolptr = (gpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err1;	/*  Look down  */
	}

	gpoolptr->pi_Pool_AID	= pool_aid;
	gpoolptr->pi_Pool	= poolmem;
	gpoolptr->pi_Size	= size;
	strcpy (gpoolptr->pi_Name, "test pool");
printf ("Creating genpool for area 0x%08lx...\n", pool_aid);
	if ((retval = (gpm->gpm_CreatePool) (gpoolptr, 2048, 0)) < 0) {
err2:		(gpm->gpm_FreePoolInfo) (gpoolptr);
err1:		delete_area (pool_aid);
err0:		(gpm->gpm_ModuleInfo.std_ops) (B_MODULE_UNINIT);
	} else {
		retval = B_OK;

		printf ("Genpool %d successfully created:\n",
			gpoolptr->pi_PoolID);
		printf ("        pi_Pool: %p (area 0x%08lx)\n",
			gpoolptr->pi_Pool, gpoolptr->pi_Pool_AID);
		printf ("  pi_RangeLists: %p (area 0x%08lx)\n",
			gpoolptr->pi_RangeLists, gpoolptr->pi_RangeLists_AID);
		printf ("  pi_RangeLocks: %p (area 0x%08lx)\n",
			gpoolptr->pi_RangeLocks, gpoolptr->pi_RangeLocks_AID);
		printf ("    pi_PoolLock: 0x%08lx\n", gpoolptr->pi_PoolLock);
		printf ("        pi_Size: %d (%dM)\n",
			gpoolptr->pi_Size, gpoolptr->pi_Size >> 20);
		printf ("       pi_Flags: 0x%08lx\n", gpoolptr->pi_Flags);
		printf ("        pi_Name: %s\n", gpoolptr->pi_Name);
	}

	return (retval);
}

void
shutdownmem (void)
{
	(gpm->gpm_DeletePool) (gpoolptr->pi_PoolID);
	(gpm->gpm_FreePoolInfo) (gpoolptr);  gpoolptr = NULL;
	delete_area (pool_aid);
	(gpm->gpm_ModuleInfo.std_ops) (B_MODULE_UNINIT);
}
                                                                                                                                                                 ���i                 ��������                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               ������������������������	 9 ...GNUmakefileMakefileincludemem.cmem.hmempool.ctestmem.c�      " ' 0 9 <     �     '     =     /�     >     ?     @     A     �8    �%
     ��     w!     �X      \    ��
     $    ��y     �8    � �8    ��8    � \    ��t     �8    � \    ��!     �8    �$    �$    �l     $    �`�     �8    �$    �$    �T     *
           P8      T      �<      �      �S      �      �d      �/             4Q      	      V      �            ,L          ��     �     �     �)    �hR    ��       � �    � �    � 0    � �    � �    � l    � �    � x    � p    � h    � �    �               �C     q     ��     �O                                    F     �B     �E                                            �     J     �M                                     ������������������������ � ...BaxterBeOS Cool StuffBemail CrashEddieFeb12.xlsFeb19.xlsFeb26-30Feb26-30.xlsOperaProductive_2.0_x86.pkgSDKTimeTrack.xlsTr@set EDITION 1.4
@set UPDATED November 1994
@set VERSION 1.4
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   Thank you for trying this port of UNZIP for VM/CMS and MVS!

                        Using under MVS:
                    -------------------------

1. To use the Info-ZIP's UNZIP under MVS you need:

   - C/370 ver 2.1 compiler or another compatible compiler supporting
     long names for function/variable names.

2. To compile the program under MVS do :

   - unzip all the files from unz54vm.zip file. They are stored as
     ASCII format so you have to unzip them first on PC or other
     system that already have UNZIP, and then upload them to the
     mainframe with ASCII to EBCDIC conversion.

   - Copy all the .C files in the PDS called USERID.UNZIP.C

   - Copy all the .H files in the PDS called USERID.UNZIP.H

   - adjust the job UNZMVSC.JOB to work on your site. Change USERID
     to your userid.  You might need to change the CEE dataset names
     to match your OS/390 system.

   - Preallocate PDS datasets named: USERID.UNZIP.OBJ and
     USERID.UNZIP.LOAD

   - execute the job UNZMVSC to compile and link all the sources.

   - if everything is ok you will get an UNZIP MODULE

3. Using UNZIP

   - Just read the UNZIP.DOC

   - A few exceptions concerning MVS

     3.0. There are different ways to invoke UNZIP.

        - allocating UNZIP.LOAD dataset to your ISPLLIB if you
          want to invoke UNZIP under ISPF.
          Then just type UNZIP ...parms... to get it work

        - You can also call it directly with :
             TSO CALL 'userid.UNZIP.LOAD(UNZIP)' '...parms...'
             (notice to quotes!)

        - You can even call it from a batch job like:

          //MYZIP    JOB  (account)
          //STEP1    EXEC PGM=UNZIP,PARM='-l mytestz.zip *.c'
          //STEPLIB  DD DSN=userid.UNZIP.LOAD,DISP=SHR
          //SYSPRINT DD SYSOUT=*

          This will list all the .c files from the zip file mytestz.zip

     3.1. If the ZIP file has been zipped on an ASCII based system
          it will be automatically translated to EBCDIC
          ( I hope I got all those translation tables OK :-).
	  You can force ASCII to EBCDIC conversion with the -a flag.

     3.2. The date/time of the output files is set to the
          current system date/time - not according the date/time in
          the zip file.

     3.3. You can even unzip using TSO/E PIPELINES
          so unzip can be used as pipeline filter:

          'pipe cms unzip -p test.zip george.test | count lines | cons'
          ( we do also a lot of pipethinking here ;-)

     3.4. If you got also the ZIP program (see ZIP21VM.ZIP) you can
          do zipping and unzipping without translating to ASCII
          the ZIP also preserves the file informations (LRECL,BLKSIZE..)
          So when you UNZIP a file zipped with ZIP under MVS it
          restores the file info.

          There currently some problems with file with RECFM=V*
          I don't save the length of each record yet :-)

     3.5. No wildcards are supported in the input zip name you have
          to give the real name (.zip is not necessary)

          So you CAN'T use things like: unzip -t *.zip

     3.6. But you CAN use wildcards as filename selection like:
          unzip -t myzip *.c  - OK or even
          unzip -t myzip *.c -x z*.c  - to exclude all files matching
                                        z*.c

     3.7. You can unzip to a PDS using the -d parameter,
          for example:

           unzip -dmyzip myzip *.c

          This will unzip all .c files that are in the zip file in a
          PDS directory called MYZIP.C

          BE AWARE that the extension of every files is being placed as
          last identifier on the PDS name, so if you have a file in the
          zipfile called 'testp.doc' and you use '-d mypds' the PDS
          name will become 'mypds.doc(testp)'

          Depending on which options IBM chose for C this week, unzip
          may or may not prefix output files with your userid and/or
          TSO prefix.  To prevent this, quote the filename to -d, for
          example

		//UNZIP   EXEC PGM=UNZIP,
		// PARM='/-a -o ''userid.zip'' -d ''hlq.test'' *'
		//STEPLIB  DD  DSN=USERID.UNZIP.LOAD,DISP=SHR
		//SYSPRINT DD  SYSOUT=*
		//SYSOUT   DD  SYSOUT=*

          The above JCL converts from ASCII to EBCDIC (-a), always
          overwrites existing members (-o), extracts from 'userid.zip',
          writes to files starting with 'hlq.test', all members (*).
          Note the double quotes because PARM= requires single quotes.

     3.8. The rules for output DCBs are a little messy.  If the output
          file already exists (remember the -d option) then unzip uses
          the existing DCB and space values.

          If the output file does not exist and the input zip came from
          MVS then unzip makes its best attempt at preserving the
          original DCB.  However there is not enough information stored
          in the zip file to do this correctly for all file types, some
          file types may be corrupted.

          If the output file does not exist and the input zip does not
          contain MVS DCB information then unzip uses RECFM=U,
          LRECL=32760 for binary data, RECFM=V, LRECL=133 for text.
          Text includes ASCII to EBCDIC conversion.  As soon as the
          output file is created, unzip uses the same output DCB for
          all following members, even if the input is a mixture of text
          and binary.

          In all cases, unzip has no built in parameters for space.
          For a preallocated file this is not a problem.  If unzip
          creates an output file you get a default space allocation
          which is site dependent.

          It is far better to preallocate the output files with the
          correct space and DCB values then use the -d option to point
          to those files.

     3.9. All '+','_' or '-' signs are skipped from the filenames


Please repport all bugs and problems to :
     Zip-Bugs@lists.wku.edu

That's all for now.

Have fun!


George Petrov
e-mail: c888090@nlevdpsb.snads.philips.nl
tel: +31-40-781155

Philips C&P
Eindhoven
The Netherlands

Updated by:

Keith Owens <kaos@ocs.com.au>
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                c��.nZY�p�򪻑+U!��xWe�V�nU�L�-�x��.\V*�k�q^ң�\fv9�sz���"VEM1_}#��:���҂4�H<-fB&G��"���p���+7�34�˅r'D��_$�v�Ns�5^p����_p3rSwZ���4+.wv!V���bT}߼9`�c�l�?�g�nVb����c�5L��X��!�a."S�S��\
�-o���9W� .z(!����t�s��	���;�h�>�aq��4�cħц�ڻHxl?���u�OGm%�T~A^�z���4 �����3�r�Y���8�\J��}���w-��7��:���+�y�-��J7 %�(�b^X��!���fwR�t~�[�\b�)5Y�$3g�~�KۮH���f01>�ʅY.���gi�x*�V_5�&��'���b�䙃}���}���X�?@J����'�����z���=�	�Ⱥ��ֆ��6]� 4�K��2$~�M$7�K�.�;����^q��%4��$%�n+�K�U�XC?��ƕ�8�S�b�ny{�I�m��¼�ַ�=*��_r�һ�0�f�b��B�`n���x�l�mX�B�F��"�{�F���zJ	T�f��F0�#�+j.��P+�)oTLl2C�J(�-�4�%���ʸ��X(��m�?�i~{���t�4�v�xj]0�ɉ'MEਡ�!��a����nF����j�%�#/S��1��y3�HR.dHM�-��L�}5�@ȵ_1�cI����\�s��)�,�֔/tfuB�^SB`
P���v�n�V69:�v*���ٺ�I�o>PD��jK_,�n��[A����لrY�