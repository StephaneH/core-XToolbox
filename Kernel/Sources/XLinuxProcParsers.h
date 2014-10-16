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
#ifndef __XLinuxProcParsers__
#define __XLinuxProcParsers__



BEGIN_TOOLBOX_NAMESPACE



////////////////////////////////////////////////////////////////////////////////
//
// StatmParser : Reads and parses /proc/self/statm
//
////////////////////////////////////////////////////////////////////////////////

class StatmParser
{
public :

	StatmParser();

	VError	Init();

	uLONG	GetSize();
	uLONG	GetResident();
	uLONG	GetShared();
	uLONG	GetText();
	uLONG	GetData();


private :

	uLONG	fSize;		// total program size (same as VmSize in /proc/[pid]/status)
	uLONG	fResident;	// resident set size (same as VmRSS in /proc/[pid]/status)
	uLONG	fShared;	// shared pages (from shared mappings)
	uLONG	fText;	    // text (code)
	//		lib            library (unused in Linux 2.6)
	uLONG	fData;		// data + stack
	//dt                   dirty pages (unused in Linux 2.6)
};



////////////////////////////////////////////////////////////////////////////////
//
// StatParser : Reads and parses /proc/self/stat
//
////////////////////////////////////////////////////////////////////////////////

class StatParser
{
public :

	StatParser();

	VError	Init();

	uLONG	GetVSize();
	
private :

	uLONG	fVSize;
};



END_TOOLBOX_NAMESPACE



#endif	//__XLinuxProcParsers__
