/*--------------------------------------------------------------------*\
  File:      Translator.cpp
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Source file containing the member functions for the
      generic Translator classes.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "Translator.h"


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Initialize the DumbCopyTranslator protected static const members
const size_t Translator::mk_buf_size(4096);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
Translator::~Translator(void)
//====================================================================
{
	;
}

//====================================================================
status_t Translator::Status(status_t * const a_err) const
//====================================================================
{
	if (a_err != NULL)
	{
		*a_err = m_status;
	}
	
	return m_status;
}

#ifndef NO_CONFIGURABLE_TRANSLATOR
//====================================================================
ConfigurableTranslator::~ConfigurableTranslator(void)
//====================================================================
{
	;
}
#endif    // NO_CONFIGURABLE_TRANSLATOR

//====================================================================
DumbCopyTranslator::~DumbCopyTranslator(void)
//====================================================================
{
	;
}

//====================================================================
status_t DumbCopyTranslator::PerformTranslation(void)
//====================================================================
{
	ssize_t bytes_read, bytes_written;
	uint8 buf[mk_buf_size];
	
	while ((bytes_read = ReadSource(buf, sizeof (uint8) * mk_buf_size)) > 0)
	{
		if ((bytes_written = WriteDestination(buf, bytes_read)) != bytes_read)
		{
			return B_NO_TRANSLATOR;
		}
	}
	
	if (bytes_read == -1 || bytes_read == 0) return B_OK;
	else return B_ERROR;
}
