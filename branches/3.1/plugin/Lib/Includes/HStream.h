/*	$Id: HStream.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Hekkelman Programmatuur b.v.
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
 
#ifndef HSTREAM_H
#define HSTREAM_H

#include <string>

#include "HTypes.h"
#include "HByteSwap.h"

class HUrl;
class HMutex;
class HMMappedFileStream;

class HStreamBase
{
  public:
					HStreamBase();
	virtual			~HStreamBase();
	
	virtual int32	Write(const void* inBuffer, uint32 inSize) = 0;
	virtual int32	PWrite(const void* inBuffer, uint32 inSize, int64 inOffset);

	virtual int32	Read(void* inBuffer, uint32 inSize) = 0;
	virtual int32	PRead(void* inBuffer, uint32 inSize, int64 inOffset);
	
	void			CopyTo(HStreamBase& inDest, int64 inSize, int64 inOffset);
	
	virtual int64	Seek(int64 inOffset, int inMode) = 0;
	virtual int64	Tell() const;
	virtual int64	Size() const;
	
	void			SetSwapBytes(bool inSwap);
	bool			SwapsBytes() const					{ return fSwap; }

	HStreamBase&	operator>> (int8& d);
	HStreamBase&	operator>> (int16& d);
	HStreamBase&	operator>> (int32& d);
	HStreamBase&	operator>> (int64& d);
	HStreamBase&	operator>> (uint8& d);
	HStreamBase&	operator>> (uint16& d);
	HStreamBase&	operator>> (uint32& d);
	HStreamBase&	operator>> (uint64& d);
	HStreamBase&	operator>> (float& d);
	HStreamBase&	operator>> (char* s);
	HStreamBase&	operator>> (std::string& s);
	//HStreamBase&	operator>> (HRect& r);

	HStreamBase&	operator<< (int8 d);
	HStreamBase&	operator<< (int16 d);
	HStreamBase&	operator<< (int32 d);
	HStreamBase&	operator<< (int64 d);
	HStreamBase&	operator<< (uint8 d);
	HStreamBase&	operator<< (uint16 d);
	HStreamBase&	operator<< (uint32 d);
	HStreamBase&	operator<< (uint64 d);
	HStreamBase&	operator<< (float d);
	HStreamBase&	operator<< (const char* s);
	HStreamBase&	operator<< (std::string s);
	
	virtual void	Truncate(int64 inOffset);

  protected:
	virtual void	ReadString(std::string& outString);
	virtual void	WriteString(std::string inString);
	
  private:
	bool			fSwap;

					HStreamBase(const HStreamBase&);
	HStreamBase&	operator=(const HStreamBase&);
};

class HResourceStream : public HStreamBase
{
  public:
					HResourceStream(uint32 inType, int inResID);
					HResourceStream(uint32 inType, const char* inName);
	
	virtual int32	Write(const void* inBuffer, uint32 inSize);
	virtual int32	Read(void* inBuffer, uint32 inSize);
	virtual int64	Seek(int64 inOffset, int inMode);
	virtual int64	Tell() const;
	virtual int64	Size() const;

  private:

	virtual void	ReadString(std::string& outString);

					HResourceStream (const HResourceStream&);
	void			operator = (const HResourceStream&);

	uint32			fType;
	int				fID;
	const void*		fData;
	uint32			fSize;
	uint32			fPointer;
};

class HMemoryStream : public HStreamBase
{
  public:
					HMemoryStream();
					HMemoryStream(void* inData, int64 inSize);
					HMemoryStream(const void* inData, int64 inSize);
					~HMemoryStream();

	virtual int32	Write(const void* inBuffer, uint32 inSize);
	virtual int32	Read(void* inBuffer, uint32 inSize);
	virtual int64	Seek(int64 inOffset, int inMode);
	virtual int64	Tell() const;
	virtual int64	Size() const;
	virtual void	Truncate(int64 inOffset);
	
	const void*		Buffer() const;

  protected:
					HMemoryStream (const HMemoryStream&);
	void			operator = (const HMemoryStream&);

	char*			fData;
	int64			fLogicalSize;
	int64			fPhysicalSize;
	int64			fPointer;
	bool			fReadOnly;
};

class HFileStream : public HStreamBase
{
	friend class	HMMappedFileStream;
  public:
					HFileStream(const HUrl& inURL, int inMode);
					~HFileStream();

	virtual int32	Write(const void* inBuffer, uint32 inSize);
	virtual int32	PWrite(const void* inBuffer, uint32 inSize, int64 inOffset);
	virtual int32	Read(void* inBuffer, uint32 inSize);
	virtual int32	PRead(void* inBuffer, uint32 inSize, int64 inOffset);
	virtual int64	Seek(int64 inOffset, int inMode);
	virtual int64	Tell() const;
	virtual int64	Size() const;
	virtual void	Truncate(int64 inOffset);

  protected:
					HFileStream();
					HFileStream (const HFileStream&);
	void			operator = (const HFileStream&);

	virtual void	Close();

	int				fFD;
	int64			fOffset;
	int64			fSize;
};

class HBufferedFileStream : public HFileStream
{
  public:
					HBufferedFileStream(const HUrl& inURL, int inMode);
	virtual			~HBufferedFileStream();
	
	virtual int32	PWrite(const void* inBuffer, uint32 inSize, int64 inOffset);
	virtual int32	PRead(void* inBuffer, uint32 inSize, int64 inOffset);

	virtual void	Truncate(int64 inOffset);
	
  protected:
					HBufferedFileStream();

	virtual void	Close();
	void			Flush();

  private:

	void			Init();

	struct BufferBlock
	{
		int64			fOffset;
		char*			fText;
		BufferBlock*	fNext;
		BufferBlock*	fPrev;
		uint32			fPageSize;
		bool			fDirty;
	};

	BufferBlock*	GetBlock(int64 inOffset);
	
	BufferBlock*	fBlocks;
	BufferBlock*	fLRUHead;
	BufferBlock*	fLRUTail;
	HMutex*			fMutex;
};

class HTempFileStream : public HBufferedFileStream
{
  public:
					HTempFileStream(const HUrl& inBaseName);
	virtual			~HTempFileStream();

  protected:

	virtual void	Close();

	HUrl*			fTempUrl;
};

class HMMappedFileStream : public HMemoryStream
{
  public:
					HMMappedFileStream(HFileStream& inFile, int64 inOffset, int64 inLength);
					~HMMappedFileStream();

  private:
	struct HMMappedFileStreamImp*	fImpl;
};

template<class T>
class HSwapStream : public HStreamBase
{
  public:
					HSwapStream(HStreamBase& inOther);

	virtual int32	Write(const void* inBuffer, uint32 inSize);
	virtual int32	Read(void* inBuffer, uint32 inSize);
	virtual int64	Seek(int64 inOffset, int inMode);
	virtual void	Truncate(int64 inOffset);

  private:
	HStreamBase&	fStream;
};

class HStreamView : public HStreamBase
{
  public:
					HStreamView(HStreamBase& inMain, int64 inOffset,
						int64 inSize);

	virtual int32	Write(const void* inBuffer, uint32 inSize);
	virtual int32	Read(void* inBuffer, uint32 inSize);
	virtual int64	Seek(int64 inOffset, int inMode);
	virtual int64	Size() const		{ return fSize; }

  private:
	HStreamBase&	fMain;
	int64			fOffset;
	int64			fSize;
	int64			fPointer;
};

template<class T>
inline
HSwapStream<T>::HSwapStream(HStreamBase& inOther)
	: fStream(inOther)
{
	assert(false);
}

template<>
inline
HSwapStream<net_swapper>::HSwapStream(HStreamBase& inOther)
	: fStream(inOther)
{
#if P_LITTLEENDIAN
	SetSwapBytes(true);
#endif
}

template<class T>
inline
int32 HSwapStream<T>::Write(const void* inBuffer, uint32 inSize)
{
	return fStream.Write(inBuffer, inSize);
}

template<class T>
inline
int32 HSwapStream<T>::Read(void* inBuffer, uint32 inSize)
{
	return fStream.Read(inBuffer, inSize);
}

template<class T>
inline
int64 HSwapStream<T>::Seek(int64 inOffset, int inMode)
{
	return fStream.Seek(inOffset, inMode);
}

template<class T>
inline
void HSwapStream<T>::Truncate(int64 inOffset)
{
	return fStream.Truncate(inOffset);
}

//class HCompressedStream
//{
//  public:
//					HCompressedStream(HStream& inStream);
//	virtual			~HCompressedStream();
//	
//	virtual int32	Write(const void* inBuffer, uint32 inSize);
//	virtual int32	PWrite(const void* inBuffer, uint32 inSize, int64 inOffset);
//
//	virtual int32	Read(void* inBuffer, uint32 inSize);
//	virtual int32	PRead(void* inBuffer, uint32 inSize, int64 inOffset);
//	
//	virtual int64	Seek(int64 inOffset, int inMode);
//	virtual int64	Tell() const;
//	virtual int64	Size() const;
//
//  private:
//	struct HCompressedStreamImp*	fImpl;
//};

#endif
