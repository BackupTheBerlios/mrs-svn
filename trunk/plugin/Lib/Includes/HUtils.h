/*	$Id: HUtils.h,v 1.61 2005/08/22 12:38:05 maarten Exp $

	Utils.h, Copyright Hekkelman Programmatuur b.v.
	Created: 12/04/97 14:08:52 by Maarten Hekkelman
*/

/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef HUTILS_H
#define HUTILS_H

#include "HStlAlgorithm.h"
#include "HStlString.h"
#include "HStdCTime.h"
#include "HTypes.h"

class HUrl;

template <class T>
class HAutoPtr
{
  public:
	explicit		HAutoPtr();
	explicit		HAutoPtr(T* inPtr);
					HAutoPtr(HAutoPtr& inPtr);
#if ! P_VISUAL_CPP
					template <class Y>
					HAutoPtr(HAutoPtr<Y>& inOther);
#endif
					~HAutoPtr();

	HAutoPtr&		operator=(HAutoPtr& inOther);
#if ! P_VISUAL_CPP
	template <class Y>
	HAutoPtr&		operator=(HAutoPtr<Y>& inOther);
#endif
	void			reset(T* inNewPtr);
	T*				get() const;
	T*				release();
	T*				operator->() const;
	T&				operator*() const;

  private:
	T*				fPtr;
};

template <class T>
inline
HAutoPtr<T>::HAutoPtr ()
	: fPtr(nil)
{
}

template <class T>
inline
HAutoPtr<T>::HAutoPtr(T* inPtr)
	: fPtr(inPtr)
{
}

template <class T>
inline
HAutoPtr<T>::HAutoPtr(HAutoPtr<T>& inPtr)
	: fPtr(inPtr)
{
}

#if ! P_VISUAL_CPP
template <class T>
template <class Y>
inline
HAutoPtr<T>::HAutoPtr(HAutoPtr<Y>& inOther)
	: fPtr(inOther)
{
}
#endif

template <class T>
inline
HAutoPtr<T>::~HAutoPtr()
{
	delete fPtr;
}

template <class T>
inline
HAutoPtr<T>&
HAutoPtr<T>::operator=(HAutoPtr& inOther)
{
	reset(inOther.release());
	return *this;
}

#if ! P_VISUAL_CPP
template <class T>
template <class Y>
inline
HAutoPtr<T>&
HAutoPtr<T>::operator=(HAutoPtr<Y>& inOther)
{
	reset(inOther.release());
	return *this;
}
#endif

template <class T>
inline
void HAutoPtr<T>::reset(T* inNewPtr)
{
	delete fPtr;
	fPtr = inNewPtr;
}

template <class T>
inline
T* HAutoPtr<T>::get() const
{
	return fPtr;
}

template <class T>
inline
T* HAutoPtr<T>::operator->() const
{
	return fPtr;
}

template <class T>
inline
T& HAutoPtr<T>::operator*() const
{
	return *fPtr;
}

template <class T>
inline
T* HAutoPtr<T>::release()
{
	T* ptr = fPtr;
	fPtr = nil;
	return ptr;
}


template <class T>
class HAutoBuf
{
  public:
	explicit		HAutoBuf(T* inPtr);
					HAutoBuf(HAutoBuf& inPtr);
//					template <class Y>
//					HAutoBuf(HAutoBuf<Y, traits::rebind<Y>::other>& inOther);
					~HAutoBuf();

	HAutoBuf&		operator=(HAutoBuf& inOther);
//	template <class Y>
//	HAutoBuf&		operator=(HAutoBuf<Y, traits::rebind<Y>::other>& inOther);
	
	T&				operator[](uint32 inIndex) const;

	void			reset(T* inNewPtr);
	T*				get() const;
	T*				release();
	T*				operator->() const;
	T&				operator*() const;

  private:
	T*				fPtr;
};

template <class T>
inline
HAutoBuf<T>::HAutoBuf(T* inPtr)
	: fPtr(inPtr)
{
}

template <class T>
inline
HAutoBuf<T>::HAutoBuf(HAutoBuf& inPtr)
	: fPtr(inPtr.release())
{
}

/*
template <class T>
template <class Y>
inline
HAutoBuf<T>::HAutoBuf(HAutoBuf<Y, traits::rebind<Y>::other>& inOther)
	: fPtr(inOther.release())
{
}
*/

template <class T>
inline
HAutoBuf<T>::~HAutoBuf()
{
	delete[] fPtr;
}

template <class T>
inline
HAutoBuf<T>&
HAutoBuf<T>::operator=(HAutoBuf& inOther)
{
	reset(inOther.release());
	return *this;
}

/*
template <class T>
template <class Y>
inline
HAutoBuf<T>&
HAutoBuf<T>::operator=(HAutoBuf<Y, traits::rebind<Y>::other>& inOther)
{
	reset(inOther.release());
	return *this;
}
*/

template <class T>
inline
void HAutoBuf<T>::reset(T* inNewPtr)
{
	delete[] fPtr;
	fPtr = inNewPtr;
}

template <class T>
inline
T* HAutoBuf<T>::get() const
{
	return fPtr;
}

template <class T>
inline
T* HAutoBuf<T>::operator->() const
{
	return fPtr;
}

template <class T>
inline
T& HAutoBuf<T>::operator*() const
{
	return *fPtr;
}

template <class T>
inline
T& HAutoBuf<T>::operator[](uint32 inIndex) const
{
	return fPtr[inIndex];
}


template <class T>
inline
T* HAutoBuf<T>::release()
{
	T* ptr = fPtr;
	fPtr = nil;
	return ptr;
}


template <class T>
class HValueChanger
{
  public:
					HValueChanger(T& inVariable, const T inValue)
						: fVariable(inVariable)
						, fSavedValue(inValue)
					{
						std::swap(fVariable, fSavedValue);
					}
					~HValueChanger()
					{
						fVariable = fSavedValue;
					}
  private:
	void			operator= (const HValueChanger&);
	T&				fVariable;
	T				fSavedValue;
};

class HTextWriter
{
  public:
					HTextWriter(const HUrl& inUrl);
					~HTextWriter();
	HTextWriter&	operator << (const char* s);
	HTextWriter&	operator << (std::string s);

  private:
	int				fFD;
};

class HProfile
{
  public:
	HProfile();
	~HProfile();
};

class HUrl;

inline unsigned int Hash(unsigned int h, unsigned int c)
{
	return c + ((h << 7) | (h >> 25));
}

void EscapeString(std::string& ioString, bool inEscape);
bool ModifierKeyDown(int inModifiers);
bool GetUserName(std::string& outName);
bool UsePassiveFTPMode();		// since this is system global on MacOS
bool MapFileName(const unsigned char* inFileName, unsigned long& outCreator,
		unsigned long& outFileType);

/* Equel to the StdC strdup but uses new */
char * string_copy (const char* inString);

void copy_to_charbuf(char* inString, const std::string& inText, int inStringSize);

char* p2cstr(unsigned char* inString);
unsigned char* c2pstr(char* inString);

std::string NumToString(int inNumber);

void beep();

void DoubleToTM(double inTime, struct std::tm& outTM);
void TMToDouble(const struct std::tm& inTM, double& outDouble);

std::string GetFormattedDate();
std::string GetFormattedTime();

std::string GetFormattedFileDateAndTime(double inDateTime);
std::string GetFormattedFileDateAndTime(int64 inDateTime);

double system_time();
double get_dbl_time();
double get_caret_time();
void delay(double inSeconds);

bool IsCancelEventInEventQueue();

void NormalizePath(std::string& ioPath);

void ShowInExplorer(const HUrl& inUrl);
void ShowInWebBrowser(const HUrl& inUrl);
void DisplayHelpFile(HUrl inHelpFile);

unsigned short CalculateCRC(const void* ptr, long count, unsigned short crc);
bool Code1Tester(const char* inName, unsigned int inSeed, unsigned int inCode);

bool GetFormattedBool();

void FormatHex(HUnicode inUnicode, char* outString, int inBufferSize);

#endif
