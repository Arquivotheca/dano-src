

class MMallocIO : public BPositionIO {
public:
					MMallocIO(char *data = NULL, size_t size = 0);
virtual				~MMallocIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;

		void		SetBlockSize(size_t blocksize);

		void		*Buffer() const;
		void		SetDispose(bool dispose);
		
		off_t		Size() const;

private:

//virtual	void		_ReservedMallocIO1();
//virtual	void		_ReservedMallocIO2();

					MMallocIO(const MMallocIO &);
		MMallocIO	&operator=(const MMallocIO &);

		size_t		fBlockSize;
		size_t		fPosition;
		
		void		*fPtr;
		size_t		fPhysSize;
		size_t		fLogSize;
		bool		fDispose;
		
		int32		_reserved[2];
};
