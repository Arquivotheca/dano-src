//****************************************************************************************
//
//	File:		Translator.h
//
//  Written by:	Matt Bagosian
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <DataIO.h>
#include <Message.h>
#include <TranslatorAddOn.h>
#include <unistd.h>

class Translator {
	public:
		// Public member functions
		Translator(BPositionIO *src, BPositionIO *dest, status_t *err = NULL) {
			Init(src, dest);
			Status(err);
		}
		virtual ~Translator() { }
		virtual status_t Status(status_t *err = NULL) const {
			if (err != NULL) {
				*err = m_status;
			}
			return m_status;
		}
		status_t Translate() {
			status_t err;
			if (Status(&err) != B_NO_ERROR) return err;
			err = PerformTranslation();
			m_status = B_NOT_ALLOWED;
			return err;
		}
	
	protected:
		// Protected static const data members
		static const size_t mk_buf_size = 4096;
	
		// Protected member functions
		virtual status_t PerformTranslation() = NULL;
	
		// Protected data members
		status_t m_status;
		BPositionIO *m_src;
		BPositionIO *m_dest;

	private:
		// Private member functions
		void Init(BPositionIO *src, BPositionIO *dest) {
			m_status = B_NO_ERROR;
			m_src = src;
			m_dest = dest;
		}
		
		// Private prohibitted member functions
		Translator(const Translator &obj);
		Translator &operator=(const Translator &obj);
};

#endif    // TRANSLATOR_H
