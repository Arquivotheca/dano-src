/*--------------------------------------------------------------------*\
  File:      Translator.h
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Header file describing the generic Translator classes.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include <DataIO.h>
#include <Message.h>
#include <TranslatorAddOn.h>

#include <unistd.h>


#ifndef TRANSLATOR_H
#define TRANSLATOR_H


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#ifndef DECL_SPEC
#if defined __BEOS__ && defined EXPORT_SYMBOLS
#define DECL_SPEC _EXPORT
#elif defined __BEOS__ && defined IMPORT_SYMBOLS
#define DECL_SPEC _IMPORT
#else
#define DECL_SPEC
#endif
#endif


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Class name:       Translator
  Inherits from:    none
  New data members: protected static const size_t mk_buf_size - for
                        the convenience of derived classes to use as a
                        constant for an arbitrary buffer size.
                    private status_t m_status - the status of the
                        object.
                    private BPositionIO *m_src - the source of the
                        data to be translated.
                    private BPositionIO *m_dest - the destination for
                        the translated data.
  Description: Abstract base class to represent a translator. The idea
      behind this class is that it is not meant to stick around for a
      long time. In other words, its function after instantiation
      should be a one-time translation.
\*--------------------------------------------------------------------*/

class DECL_SPEC Translator
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: Translator
  Member of:     public Translator
  Defined in:    Translator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		Translator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~Translator
  Member of:     public Translator
  Defined in:    Translator.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~Translator(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public Translator
  Defined in:    Translator.cpp
  Arguments:     status_t * const a_err - a placeholder for the error
                     code (this will be the same value that is
                     returned). Default: NULL.
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Classes who wish to override this
      function, should always call this function first to insure that
      the underlying structure is valid. Possible return values are as
      follows:
      
      B_NO_ERROR - the object was created successfully.
      B_NOT_ALLOWED - the translation has already been performed.
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(status_t * const a_err = NULL) const;
		
/*--------------------------------------------------------------------*\
  Function name: virtual Translate
  Member of:     public Translator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to make some checks and then call the
      PerformTranslation() hook function.
\*--------------------------------------------------------------------*/
		
		status_t Translate(void);
	
	protected:
	
		// Protected static const data members
		static const size_t mk_buf_size;
	
		// Protected member functions
		
/*--------------------------------------------------------------------*\
  Function name: virtual PerformTranslation
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Hook function called to perform the translation.
\*--------------------------------------------------------------------*/
		
		virtual status_t PerformTranslation(void) = NULL;
		
/*--------------------------------------------------------------------*\
  Function name: PositionDestination const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       off_t - the current position.
  Throws:        none
  Description: Function to get the current position in the destination
      data area.
\*--------------------------------------------------------------------*/
		
		off_t PositionDestination(void) const;
		
/*--------------------------------------------------------------------*\
  Function name: PositionSource
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       off_t - the current position.
  Throws:        none
  Description: Function to get the current position in the source data
      area.
\*--------------------------------------------------------------------*/
		
		off_t PositionSource(void) const;
		
/*--------------------------------------------------------------------*\
  Function name: ReadAtDestination const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_pos - the position from which to begin
                     reading.
                 void * const a_buf - the buffer into which to read.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes read on success, an
                     error code on failure.
  Throws:        none
  Description: Function to read from a given position from the
      destination data area.
\*--------------------------------------------------------------------*/
		
		ssize_t ReadAtDestination(const off_t a_pos, void * const a_buf, const size_t a_buf_len) const;
		
/*--------------------------------------------------------------------*\
  Function name: ReadAtSource const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_pos - the position from which to begin
                     reading.
                 void * const a_buf - the buffer into which to read.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes read on success, an
                     error code on failure.
  Throws:        none
  Description: Function to read from a given position from the source
      data area.
\*--------------------------------------------------------------------*/
		
		ssize_t ReadAtSource(const off_t a_pos, void * const a_buf, const size_t a_buf_len) const;
		
/*--------------------------------------------------------------------*\
  Function name: ReadDestination const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     void * const a_buf - the buffer into which to read.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes read on success, an
                     error code on failure.
  Throws:        none
  Description: Function to read from the current position from the
      destination data area.
\*--------------------------------------------------------------------*/
		
		ssize_t ReadDestination(void * const a_buf, const size_t a_buf_len) const;
		
/*--------------------------------------------------------------------*\
  Function name: ReadSource const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     void * const a_buf - the buffer into which to read.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes read on success, an
                     error code on failure.
  Throws:        none
  Description: Function to read from the current position from the
      source data area.
\*--------------------------------------------------------------------*/
		
		ssize_t ReadSource(void * const a_buf, const size_t a_buf_len) const;
		
/*--------------------------------------------------------------------*\
  Function name: SeekDestination const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_pos - the offset.
                 const int32 a_mod - the mode from which to calculate
                     the offset. Default: SEEK_SET.
  Returns:       off_t - the new position.
  Throws:        none
  Description: Function to set the reading/writing position for the
      destination data area.
\*--------------------------------------------------------------------*/
		
		off_t SeekDestination(const off_t a_pos, const int32 a_mod = SEEK_SET) const;
		
/*--------------------------------------------------------------------*\
  Function name: SeekSource const
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_pos - the offset.
                 const int32 a_mod - the mode from which to calculate
                     the offset. Default: SEEK_SET.
  Returns:       off_t - the new position.
  Throws:        none
  Description: Function to set the reading position for the srouce
      data area.
\*--------------------------------------------------------------------*/
		
		off_t SeekSource(const off_t a_pos, const int32 a_mod = SEEK_SET) const;
		
/*--------------------------------------------------------------------*\
  Function name: SetSizeDestination
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_size - the new size.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to set the size of the destination data area.
\*--------------------------------------------------------------------*/
		
		status_t SetSizeDestination(const off_t a_size);
		
/*--------------------------------------------------------------------*\
  Function name: WriteAtDestination
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     const off_t a_pos - the position at which to begin
                     writing.
                 void * const a_buf - the buffer from which to write.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes written on success, an
                     error code on failure.
  Throws:        none
  Description: Function to write to a given position to the
      destination data area.
\*--------------------------------------------------------------------*/
		
		ssize_t WriteAtDestination(const off_t a_pos, void * const a_buf, const size_t a_buf_len);
		
/*--------------------------------------------------------------------*\
  Function name: WriteDestination
  Member of:     protected Translator
  Defined in:    Translator.h
  Arguments:     void * const a_buf - the buffer from which to write.
                 const size_t a_buf_len - the size of the buffer.
  Returns:       ssize_t - the number of bytes written on success, an
                     error code on failure.
  Throws:        none
  Description: Function to write to the current position to the
      destination data area.
\*--------------------------------------------------------------------*/
		
		ssize_t WriteDestination(void * const a_buf, const size_t a_buf_len);
	
	private:
	
		// Private data members
		status_t m_status;
		BPositionIO *m_src;
		BPositionIO *m_dest;
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private Translator
  Defined in:    Translator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(BPositionIO * const a_src, BPositionIO * const a_dest);
		
		// Private prohibitted member functions
		Translator(const Translator &a_obj);
		Translator &operator=(const Translator &a_obj);
};

#ifndef NO_CONFIGURABLE_TRANSLATOR
/*--------------------------------------------------------------------*\
  Class name:       ConfigurableTranslator
  Inherits from:    public Translator
  Defined in:       Translator.cpp
  New data members: private BMessage m_config - the configuration
                        message.
  Description: Abstract class to represent a configurable translator.
\*--------------------------------------------------------------------*/

class DECL_SPEC ConfigurableTranslator : public Translator
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: ConfigurableTranslator
  Member of:     public ConfigurableTranslator
  Defined in:    Translator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable.
\*--------------------------------------------------------------------*/
		
		ConfigurableTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~ConfigurableTranslator
  Member of:     public ConfigurableTranslator
  Defined in:    Translator.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~ConfigurableTranslator(void);
	
	protected:
	
		// Protected member functions
		
/*--------------------------------------------------------------------*\
  Function name: GetConfigMessage
  Member of:     protected ConfigurableTranslator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       BMessage * - the configuration message.
  Throws:        none
  Description: Function to get the configuration message retrieved
      from the translation add-on's GetConfigMessage() function.
\*--------------------------------------------------------------------*/
		
		BMessage *GetConfigMessage(void);
	
	private:
	
		typedef Translator Inherited;
		
		// Private data members
		BMessage m_config;
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private ConfigurableTranslator
  Defined in:    Translator.h
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(void);
		
		// Private prohibitted member functions
		ConfigurableTranslator(const ConfigurableTranslator &a_obj);
		ConfigurableTranslator &operator=(const ConfigurableTranslator &a_obj);
};
#endif    // NO_CONFIGURABLE_TRANSLATOR

/*--------------------------------------------------------------------*\
  Class name:       DumbCopyTranslator
  Inherits from:    public Translator
  Defined in:       Translator.cpp
  New data members: none
  Description: Class to represent a dumb translator which merely
      copies the data straight from the source data area to the
      destination data area when invoked with the Translate()
      function.
\*--------------------------------------------------------------------*/

class DECL_SPEC DumbCopyTranslator : public Translator
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: DumbCopyTranslator
  Member of:     public DumbCopyTranslator
  Defined in:    Translator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable.
\*--------------------------------------------------------------------*/
		
		DumbCopyTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~DumbCopyTranslator
  Member of:     public DumbCopyTranslator
  Defined in:    Translator.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~DumbCopyTranslator(void);
	
	protected:
	
		// Protected member functions
		
/*--------------------------------------------------------------------*\
  Function name: virtual PerformTranslation
  Member of:     protected DumbCopyTranslator
  Defined in:    Translator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual status_t PerformTranslation(void);
	
	private:
	
		typedef Translator Inherited;
		
		// Private prohibitted member functions
		DumbCopyTranslator(const DumbCopyTranslator &a_obj);
		DumbCopyTranslator &operator=(const DumbCopyTranslator &a_obj);
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline Translator::Translator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err)
//====================================================================
{
	Init(a_src, a_dest);
	Status(a_err);
}

//====================================================================
inline status_t Translator::Translate(void)
//====================================================================
{
	status_t err;
	
	if (Status(&err) != B_NO_ERROR)
	{
		return err;
	}
	
	err = PerformTranslation();
	m_status = B_NOT_ALLOWED;
	
	return err;
}

//====================================================================
inline off_t Translator::PositionDestination(void) const
//====================================================================
{
	return m_dest->Position();
}

//====================================================================
inline off_t Translator::PositionSource(void) const
//====================================================================
{
	return m_src->Position();
}

//====================================================================
inline ssize_t Translator::ReadAtDestination(const off_t a_pos, void * const a_buf, const size_t a_buf_len) const
//====================================================================
{
	return m_dest->ReadAt(a_pos, a_buf, a_buf_len);
}

//====================================================================
inline ssize_t Translator::ReadAtSource(const off_t a_pos, void * const a_buf, const size_t a_buf_len) const
//====================================================================
{
	return m_src->ReadAt(a_pos, a_buf, a_buf_len);
}

//====================================================================
inline ssize_t Translator::ReadDestination(void * const a_buf, const size_t a_buf_len) const
//====================================================================
{
	return m_dest->Read(a_buf, a_buf_len);
}

//====================================================================
inline ssize_t Translator::ReadSource(void * const a_buf, const size_t a_buf_len) const
//====================================================================
{
	return m_src->Read(a_buf, a_buf_len);
}

//====================================================================
inline off_t Translator::SeekDestination(const off_t a_pos, const int32 a_mod) const
//====================================================================
{
	return m_dest->Seek(a_pos, a_mod);
}

//====================================================================
inline off_t Translator::SeekSource(const off_t a_pos, const int32 a_mod) const
//====================================================================
{
	return m_src->Seek(a_pos, a_mod);
}

//====================================================================
inline status_t Translator::SetSizeDestination(const off_t a_size)
//====================================================================
{
	return m_dest->SetSize(a_size);
}

//====================================================================
inline ssize_t Translator::WriteAtDestination(const off_t a_pos, void * const a_buf, const size_t a_buf_len)
//====================================================================
{
	return m_dest->WriteAt(a_pos, a_buf, a_buf_len);
}

//====================================================================
inline ssize_t Translator::WriteDestination(void * const a_buf, const size_t a_buf_len)
//====================================================================
{
	return m_dest->Write(a_buf, a_buf_len);
}

//====================================================================
inline void Translator::Init(BPositionIO * const a_src, BPositionIO * const a_dest)
//====================================================================
{
	m_status = B_NO_ERROR;
	m_src = a_src;
	m_dest = a_dest;
}

#ifndef NO_CONFIGURABLE_TRANSLATOR
//====================================================================
inline ConfigurableTranslator::ConfigurableTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err) :
//====================================================================
	Inherited(a_src, a_dest, a_err)
{
	Init();
}

//====================================================================
inline BMessage *ConfigurableTranslator::GetConfigMessage(void)
//====================================================================
{
	return &m_config;
}

//====================================================================
inline void ConfigurableTranslator::Init(void)
//====================================================================
{
	// Call the add-on's defined function
	::GetConfigMessage(&m_config);
}
#endif    // NO_CONFIGURABLE_TRANSLATOR

//====================================================================
inline DumbCopyTranslator::DumbCopyTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err) :
//====================================================================
	Inherited(a_src, a_dest, a_err)
{
	;
}


#endif    // TRANSLATOR_H
