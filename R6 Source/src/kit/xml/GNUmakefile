entries)	i = -1;
			}
		}
		if (sizes[i] > 0)
			testfree (ptrs[i], sizes[i]);
		ptrs[i] = NULL;
		sizes[i] = 0;
	}
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (&nentries));
	printf ("Freenodes remaining: %d\n", nentries);
}


/*****************************************************************************
 * Allocator front end and support.
 */
void *
testalloc (int32 size, uint32 carebits, uint32 statebits)
{
	BMemSpec	ms;
	
	ms.ms_Size		= size;
	ms.ms_AddrCareBits	= carebits;
	ms.ms_AddrStateBits	= statebits;
	ms.ms_AllocFlags	= 0;

	if (AllocMem (&ms, &gml) < 0)
		return (NULL);
	else
		return ((void *) ms.ms_BaseAddr);
}

void
testfree (void *ptr, int32 size)
{
	BMemSpec	ms;
	
	ms.ms_Size	= size;
	ms.ms_BaseAddr	= (uint32) ptr;
	FreeMem (&ms, &gml);
}


void *
allocnode (void)
{
	register int	i;
	
	for (i = gfirstfree;  i < NNODES;  i++)
		if (ISORPHANNODE (gnodes + i)) {
			gfirstfree = i + 1;
			return (gnodes + i);
		}
	return (NULL);
}

void
freenode (void *node)
{
	register int32	idx;

	if ((idx = (MemNode *) node - gnodes) < gfirstfree)
		gfirstfree = idx;
}

status_t
initmem (uint32 size)
{
	register int	i;

	for (i = NNODES;  --i >= 0; )
		INITNODE (gnodes + i);

	if (!(gml.ml_BaseAddr = (uint32) malloc (size)))
		return (B_NO_MEMORY);

	gml.ml_Size = size;
	gml.ml_Name = "Test region"