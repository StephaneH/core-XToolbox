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
#include "VXMLPrecompiled.h"

#include "XMLSaxWriter.h"

#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/parsers/SAXParser.hpp>

BEGIN_TOOLBOX_NAMESPACE

class  VStreamXMLFormatTarget;

class XTOOLBOX_API VStreamSaxWriter : public VSaxWriter
{
public :
		VStreamSaxWriter(VStream *inStream);
		~VStreamSaxWriter();
		virtual void PutSimpleBeginTag(const VString &inStr,bool inCloseItAlso);
		virtual void PutEndTag();

		virtual void PutComplexBeginTag(const VString &inStr);
		virtual void PutComplexBeginTagArguments(const VString &inName,const VString &inValue);
		virtual void PutEndComplexBeginTag(bool inCloseItAlso);

		virtual void PutComments(const VString &inStr);

protected:
		void	_Write(const VString &XMLStr);

private :
		VStreamXMLFormatTarget*		fFormatTarget;
		xercesc::XMLFormatter*				fFormatter;
};

class  VStreamXMLFormatTarget : public xercesc::XMLFormatTarget
{
public:
	VStreamXMLFormatTarget(VStream *inStream);
    virtual ~VStreamXMLFormatTarget();
    #ifdef XERCES_3_0_1
	virtual void writeChars
    (
        const   XMLByte* const      toWrite
        , const XMLSize_t        count
		,       xercesc::XMLFormatter* const formatter
    );
 	#else
	virtual void writeChars
    (
        const   XMLByte* const      toWrite
        , const unsigned int        count
		,       xercesc::XMLFormatter* const formatter
    );
	#endif
protected :
    VStreamXMLFormatTarget() {}
    VStreamXMLFormatTarget(const VStreamXMLFormatTarget&) {}
    void operator=(const VStreamXMLFormatTarget&) {}

private :
	VStream *fStream;
};

VStreamXMLFormatTarget::VStreamXMLFormatTarget(VStream *inStream)
{
	fStream = inStream;
}

VStreamXMLFormatTarget::~VStreamXMLFormatTarget()
{
}

#ifdef XERCES_3_0_1
void VStreamXMLFormatTarget::writeChars(const XMLByte* const toWrite, const XMLSize_t count,xercesc::XMLFormatter* const formatter)
#else
void VStreamXMLFormatTarget::writeChars(const XMLByte* const toWrite, const unsigned int count,xercesc::XMLFormatter* const formatter)
#endif
{
	fStream->PutData(toWrite,count);
}

VStreamSaxWriter::VStreamSaxWriter(VStream *inStream)
{
	const XMLCh fgVersion1_0[] = {xercesc::chDigit_1, xercesc::chPeriod, xercesc::chDigit_0, xercesc::chNull};

	fFormatTarget = NULL;
	fFormatter = NULL;

	try {
		fFormatTarget = new VStreamXMLFormatTarget(inStream);
		fFormatter = new xercesc::XMLFormatter(xercesc::XMLUni::fgUTF8EncodingString, fgVersion1_0, fFormatTarget, xercesc::XMLFormatter::StdEscapes, xercesc::XMLFormatter::UnRep_CharRef);
		
		fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
		_Write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
		fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
		NextLine();
	}
	catch (...)
	{
	}
}

VStreamSaxWriter::~VStreamSaxWriter()
{
	delete fFormatTarget;
	delete fFormatter;
}

void VStreamSaxWriter::_Write(const VString &XMLStr)
{
	try
	{
		fFormatter->formatBuf ((XMLCh*)XMLStr.GetCPointer(),XMLStr.GetLength());
	}
	catch (...)
	{
		;
	}
}

void VStreamSaxWriter::PutSimpleBeginTag(const VString &inStr,bool inCloseItAlso)
{
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	_Write(CVSTR("<"));
	_Write(inStr);

	if (inCloseItAlso)
		_Write(CVSTR("/>"));
	else
	{
		fStackIndentation.push(inStr);
		_Write(CVSTR(">"));
	}

	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
}

void VStreamSaxWriter::PutComplexBeginTag(const VString &inStr)
{
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	_Write(CVSTR("<"));
	_Write(inStr);
	fStackIndentation.push(inStr);
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
}

void VStreamSaxWriter::PutComplexBeginTagArguments(const VString &inName,const VString &inValue)
{
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	_Write(CVSTR(" "));
	_Write(inName);
	_Write(CVSTR("=\""));
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
	_Write(inValue);
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	_Write(CVSTR("\""));
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
}

void VStreamSaxWriter::PutEndComplexBeginTag(bool inCloseItAlso)
{
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	if (inCloseItAlso)
	{
		fStackIndentation.pop();
		_Write(CVSTR("/>"));
	}
	else
		_Write(CVSTR(">"));
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
}

void VStreamSaxWriter::PutComments(const VString &inStr)
{
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
	_Write(CVSTR("<!-- "));
	_Write(inStr);
	_Write(CVSTR(" -->"));
	fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);
}

void VStreamSaxWriter::PutEndTag()
{
	if (fStackIndentation.empty() == false)
	{
		XBOX::VString inText = fStackIndentation.top();

		fFormatter->setEscapeFlags(xercesc::XMLFormatter::NoEscapes);
		_Write(CVSTR("</"));
		_Write(inText);
		_Write(CVSTR(">"));
		fFormatter->setEscapeFlags(xercesc::XMLFormatter::StdEscapes);

		fStackIndentation.pop();
	}
}

void VSaxWriter::NextLine()
{
	_Write(CVSTR("\r\n"));
}

void VSaxWriter::PutContents(const VString &inStr)
{
	_Write(inStr);
}

void VSaxWriter::PutPrintfContents(const char* inMessage, ...)
{
	VStr255 text;

	va_list argList;
	va_start(argList, inMessage);
	text.VPrintf(inMessage, argList);
	va_end(argList);

	return _Write(text);
}

void VSaxWriter::PutVPrintfContents(const char* inMessage, va_list argList)
{
	VStr255 text;

	text.VPrintf(inMessage, argList);

	_Write(text);
}


VSaxWriter *VSaxWriter::CreateSaxWriterFromStream(VStream *inStream)
{
	return new VStreamSaxWriter(inStream);
}

END_TOOLBOX_NAMESPACE
