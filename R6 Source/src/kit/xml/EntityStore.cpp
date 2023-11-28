"Can't initialize memory manager: %d (0x%08lx)\n",
			retval, retval);
	printf ("Allocated %d for workspace.\n", TESTAREASIZE);

	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);

	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, complex mask (8 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		care = random () % 0xFF;
		state = random () & care;
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocates: %d\n", total);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&size));
	printf ("Freenodes remaining: %d\n\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	return (0);
}


int32
checkfreelist (int32 *nnodes)
{
	MemNode	*mn;
	int32	total = 0;
	
	*nnodes = 0;
	for (mn = (MemNode *) FIRSTNODE (&gml.ml_List);
	     NEXTNODE (mn);
	     mn = (MemNode *) NEXTNODE (mn))
	{
		total += mn->mem_Size;
		if (nnodes)	(*nnodes)++;
	}

	return (total);
}

void
freeseqandcheck (void **ptrs, int32 *sizes, int32 n)
{
	while (--n >= 0) {
		if (*ptrs)
			testfree (*ptrs, *sizes);
		*ptrs++ = NULL;
		*sizes++ = 0;
	}
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (&n));
	printf ("Freenodes remaining: %d\n", n);
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
				while (!sizes[--i])
					if (i < 0)		i = nentries;