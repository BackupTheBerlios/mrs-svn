/*	$Id: CXMLDocument.cpp 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
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
 
#include "MRS.h"

#include <libxml/xpath.h>

#include "HError.h"

#include "CXMLDocument.h"

using namespace std;

CXMLDocument::CXMLDocument(const string& inData)
	: mDoc(nil)
	, mXPathContext(nil)
{
	mDoc = xmlReadDoc(BAD_CAST inData.c_str(), nil, nil, 0);
	if (mDoc == nil)
		THROW(("Failed to read XML doc"));
	
	mXPathContext = xmlXPathNewContext(mDoc);
	if (mXPathContext == nil)
		THROW(("Failed to create XPath context"));
	
	mXPathContext->node = xmlDocGetRootElement(mDoc);
}

CXMLDocument::~CXMLDocument()
{
	xmlXPathFreeContext(mXPathContext);
	xmlFreeDoc(mDoc);
	xmlCleanupParser();
}

void CXMLDocument::CollectText(xmlNodePtr inNode, string& outText)
{
	const char* text = (const char*)XML_GET_CONTENT(inNode);
	if (text != nil)
	{
		outText += text;
		outText += ' ';
	}
	
	xmlNodePtr next = inNode->children;
	while (next != nil)
	{	
		CollectText(next, outText);
		next = next->next;
	}
}

void CXMLDocument::FetchText(xmlXPathCompExprPtr inPath, string& outText)
{
	xmlXPathObjectPtr res = xmlXPathCompiledEval(inPath, mXPathContext);
	if (res != nil)
	{
		xmlNodeSetPtr nodes = res->nodesetval;
		if (nodes != nil)
		{
			for (int i = 0; i < nodes->nodeNr; ++i)
			{
				xmlNodePtr node = nodes->nodeTab[i];
				CollectText(node, outText);
			}
		}
		
		xmlXPathFreeObject(res);
	}
}

