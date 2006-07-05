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
 
%module MRS
%include "std_string.i"

%{
#include <vector>
#include <iostream>
using namespace std;

class MDatabank;
typedef std::vector<MDatabank*> MDatabankArray;
%}

%typemap(in) MDatabankArray (jint size)
{
	MDatabankArray a;

	_jobjectArray* aa = (_jobjectArray*)($input);

	size = jenv->GetArrayLength(aa);

	/* make a copy of each string */
	for (int i = 0; i < size; ++i)
	{
		MDatabank* obj = (MDatabank*)jenv->GetObjectArrayElement(aa, i);
		a.push_back(obj);
	}
	
	$1 = a;
}

extern std::string gErrStr;

%javaexception("java.lang.Exception") {
	try {
		$action
	}
	catch (const std::exception& e) {
		gErrStr = e.what();
		jclass clazz = jenv->FindClass("java/lang/Exception");
		jenv->ThrowNew(clazz, e.what());
		return $null;
	}
	catch (...) {
		gErrStr = "Unknown exception";
		jclass clazz = jenv->FindClass("java/lang/Exception");
		jenv->ThrowNew(clazz, "Unknown exception");
		return $null;
	}
}

%{
#include "MRSInterface.h"
%}

%include "MRSInterface.h"

