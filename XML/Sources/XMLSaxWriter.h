/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#ifndef __XMLSaxWriter__
#define __XMLSaxWriter__

#include <stack>

BEGIN_TOOLBOX_NAMESPACE

/*
	the goal of this class is to be able to create easily and sequentially
	XML files without thinking about encodings, escape characters with an interface
	independent of a specific XML engine (m.c)

	To use:
	* create a VSaxWriter from a VStream with a factory call
	* PutBeginTag(CVSTR("aaa"));
	* PutContentsTag(CVSTR("contents"));
	* PutEndTag();

	this create the following xml contents:

	<aaa>contents</aaa>
*/

class XTOOLBOX_API VSaxWriter : public VObject
{
public :
	VSaxWriter() {;}
	virtual ~VSaxWriter() {;}

	virtual void PutSimpleBeginTag(const VString &inStr,bool inCloseItAlso=false) = 0;
	virtual void PutEndTag() = 0;

	virtual void PutComplexBeginTag(const VString &inStr) = 0;
	virtual void PutComplexBeginTagArguments(const VString &inName,const VString &inValue) = 0;
	virtual void PutEndComplexBeginTag(bool inCloseItAlso=false) = 0;

	virtual void PutComments(const VString &inStr) = 0;

	void NextLine();

	void PutContents(const VString &inStr);
	void PutPrintfContents(const char* inMessage, ...);
	void PutVPrintfContents(const char* inMessage, va_list argList);

	// factory calls:
	static VSaxWriter* CreateSaxWriterFromStream(VStream *inStream);

protected:
	virtual void _Write(const VString &inStr) = 0;
	std::stack<XBOX::VString> fStackIndentation;
};

END_TOOLBOX_NAMESPACE

#endif
