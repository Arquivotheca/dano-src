/*--------------------------------------------------------------------*\
  File:      CycleStringView.cpp
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Source file containing the member functions for the
      CycleStringView class.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "CycleStringView.h"


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
CycleStringView::~CycleStringView(void)
//====================================================================
{
	Clear();
}

//====================================================================
status_t CycleStringView::Archive(BMessage * const a_archv, const bool a_deep) const
//====================================================================
{
	// TODO
	
	return Inherited::Archive(a_archv, a_deep);
}

//====================================================================
void CycleStringView::Clear(void)
//====================================================================
{
	deleteArguments(m_strs);
	Init();
}

//====================================================================
void CycleStringView::MouseDown(const BPoint /*a_point*/)
//====================================================================
{
	if (m_strs != NULL
		&& m_strs[m_which_str] != NULL)
	{
		if (m_strs[++m_which_str] == NULL)
		{
			m_which_str = 0;
		}
		
		SetText(m_strs[m_which_str]);
	}
}

//====================================================================
status_t CycleStringView::Status(status_t * const a_err) const
//====================================================================
{
	if (a_err != NULL)
	{
		*a_err = m_status;
	}
	
	return m_status;
}
