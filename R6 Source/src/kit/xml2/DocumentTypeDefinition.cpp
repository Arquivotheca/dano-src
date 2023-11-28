race((stderr, "buildpath now = [%s]\n", buildpath));
        return 0;  /* could check for existence here, prompt for new name... */
    }

/*---------------------------------------------------------------------------
    INIT:  allocate and initialize buffer space for the file currently being
    extracted.  If file was renamed with an absolute path, don't prepend the
    extract-to path.
  ---------------------------------------------------------------------------*/

/* GRR:  for VMS and TOPS-20, add up to 13 to strlen */

    if (FUNCTION == INIT) {
        Trace((stderr, "initializing buildpath to "));
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+1)) ==
            (char *)NULL)
            return 10;
        if ((rootlen > 0) && !renamed_fullpath) {
            strcpy(buildpath, rootpath);
            end = buildpath + rootlen;
        } else {
            *buildpath = '\0';
            end = buildpath;
        }
        Trace((stderr, "[%s]\n", buildpath));
        return 0;
    }

/*---------------------------------------------------------------------------
    ROOT:  if appropriate, store the path in rootpath and create it if neces-
    sary; else assume it's a zipfile member and return.  This path segment
    gets used in extracting all members from every zipfile specified on the
    command line.
  ---------------------------------------------------------------------------*/

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (FUNCTION == ROOT) {
        Trace((stderr, "initializing root path to [%s]\n", pathcomp));
        if (pathcomp == (char *)NULL) {
            rootlen = 0;
            return 0;
        }
        if (rootlen > 0)        /* rootpath was already set, nothing to do */
            return 0;
        if ((rootlen = strlen(pathcomp)) > 0) {
            char *tmproot;

            if ((tmproot = (char *)malloc(rootlen+2)) == (char *)NULL) {
                rootlen = 0;
                return 10;
            }
            strcpy(tmproot, pathcomp);
            if (tmproot[rootlen-1] == '.') {    /****** was '/' ********/
                tmproot[--rootlen] = '\0';
            }
            if (rootlen > 0 && (stat(tmproot, &G.statbuf) ||
                                !S_ISDIR(G.statbuf.st_mode)))
            {   /* path does not exist */
                if (!G.create_dirs /* || isshexp(tmproot) */ ) {
                    free(tmproot);
                    rootlen = 0;
                    return 2;   /* skip (or treat as stored file) */
                }
                /* create the directory (could add loop here scanning tmproot
                 * to create more than one level, but why really necessary?) */
                if (mkdir(tmproot, 0777) == -1) {
                    Info(slide, 1, ((char *)slide,
                      "checkdir:  cannot create extraction directory: %s\n",
                      tmproot));
                    free(tmproot);
                    rootlen = 0;  /* path didn't exist, tried to create, and */
                    return 3; /* failed:  file exists, or 2+ levels required */
                }
            }
            tmproot[rootlen++] = '.';   /*********** was '/' *************/
            tmproot[rootlen] = '\0';
            if ((rootpath = (char *)realloc(tmproot, rootlen+1)) == NULL) {
                free(tmproot);
                rootlen = 0;
                return 10;
            }
            Trace((stderr, "rootpath now = [%s]\n", rootpath));
        }
        return 0;
    }
#endif /* !SFX || SFX_EXDIR */

/*---------------------------------------------------------------------------
    END:  free rootpath, immediately prior to program exit.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == END) {
        Trace((stderr, "freeing rootpath\n"));
        if (rootlen > 0) {
            free(rootpath);
            rootlen = 0;
        }
        return 0;
    }

    return 99;  /* should never reach */

} /* end function checkdir() */





/********************/
/* Function mkdir() */
/********************/

int mkdir(path, mode)
    const char *path;
    int mode;   /* ignored */
/*
 * returns:   0 - successful
 *           -1 - failed (errno not set, however)
 */
{
    return (SWI_OS_File_8((char *)path) == NULL)? 0 : -1;
}




/*********************************/
/* extra_field-related functions */
/*********************************/

int isRISCOSexfield(void *extra_field)
{
  if (extra_field!=NULL) {
    extra_block *block=(extra_block *)extra_field;
    return (block->ID==EF_SPARK && (block->size==24 || block->size==20) &&
            block->ID_2==SPARKID_2);
  } else
    return FALSE;
}

void setRISCOSexfield(char *path, void *extra_field)
{
  if (extra_field!=NULL) {
    extra_block *block=(extra_block *)extra_field;
    SWI_OS_File_1(path,block->loadaddr,block->execaddr,block->attr);
  }
}

void printRISCOSexfield(int isdir, void *extra_field)
{
 extra_block *block=(extra_block *)extra_field;
 printf("\n  This file has RISC OS file informations in the local extra field.\n");

 if (isdir) {
/*   I prefer not to print this string... should change later... */
/*   printf("  The file is a directory.\n");*/
 } else if ((block->loadaddr & 0xFFF00000) != 0xFFF00000) {
   printf("  Load address: %.8X\n",block->loadaddr);
   printf("  Exec address: %.8X\n",block->execaddr);
 } else {
   /************* should change this to use OS_