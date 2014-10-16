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
#include "VKernelPrecompiled.h"

#include "XLinuxProcParsers.h"

#include <stdio.h>



////////////////////////////////////////////////////////////////////////////////
//
// StatmParser
//
////////////////////////////////////////////////////////////////////////////////

StatmParser::StatmParser() : fSize(0), fResident(0), fShared(0), fText(0), fData(0) {}


VError StatmParser::Init()
{
	VError verr=VE_OK;

	FILE* file=fopen("/proc/self/statm", "r");
	
	if(file==NULL)
		verr=MAKE_NATIVE_VERROR(errno);
	else
	{
		if(fscanf(file, "%u %u %u %u %*u %u %*u", &fSize, &fResident, &fShared, &fText, &fData)!=7)
			verr=VE_INVALID_PARAMETER;
	}

	if(file!=NULL)
		fclose(file);
	
	return verr;
}


uLONG StatmParser::GetSize()
{
	return fSize;
}

uLONG StatmParser::GetResident()
{
	return fResident;
}

uLONG StatmParser::GetShared()
{
	return fShared;
}

uLONG StatmParser::GetText()
{
	return fText;
}

uLONG StatmParser::GetData()
{
	return fData;
}



////////////////////////////////////////////////////////////////////////////////
//
// StatParser : Reads and parses /proc/self/stat
//
////////////////////////////////////////////////////////////////////////////////

StatParser::StatParser() : fVSize(0) {}


VError StatParser::Init()
{
	VError verr=VE_OK;

	FILE* file=fopen("/proc/self/stat", "r");
	
	if(file==NULL)
		verr=MAKE_NATIVE_VERROR(errno);
	else
	{
		unsigned long tmpSize;
		int n=fscanf(file,
                     "%*d "      //pid
                     "(%*s) "    //comm
                     "%*c "      //state
                     "%*d "      //ppid
                     "%*d "      //pgrp
                     "%*d "      //session
                     "%*d "      //tty_nr
                     "%*d "      //tgpid
                     "%*u "      //flags
                     "%*lu "     //minflt
                     "%*lu "     //cminflt
                     "%*lu "     //majflt
                     "%*lu "     //cmajflt
                     "%*lu "     //utime
                     "%*lu "     //stime
                     "%*ld "     //cutime
                     "%*ld "     //cstime
                     "%*ld "     //priority
                     "%*ld "     //nice
                     "%*ld "     //num_threads
                     "%*ld "     //itrealvalue
                     "%*llu "    //starttime
                     "%lu "      //vsize                 <--- take this one !   
                     "%*ld "     //rss
                     "%*lu "     //rsslim
                     "%*lu "     //startcode
                     "%*lu "     //endcode
                     "%*lu "     //startstack
                     "%*lu "     //kstkesp
                     "%*lu "     //kstkeip
                     "%*lu "     //signal
                     "%*lu "     //blocked
                     "%*lu "     //sigignore
                     "%*lu "     //sigcatch
                     "%*lu "     //wchan
                     "%*lu "     //nswap
                     "%*lu "     //cnswap                <--- n=37
                     /************************************************************
					"%*d "      //exit_signal           (since Linux 2.1.22)
					"%*d "      //processor             (since Linux 2.2.8)
					"%*u "      //rt_priority           (since Linux 2.5.19)
					"%*u "      //policy                (since Linux 2.5.19)
					"%*llu "    //delayacct_blkio_ticks (since Linux 2.6.18)
					"%*lu "     //guest_time            (since Linux 2.6.24)
					"%*ld "     //cguest_time           (since Linux 2.6.24)
                     ************************************************************/
                     , &tmpSize
                     );
		fVSize = (uLONG)tmpSize;

		if(n!=37)
			verr=VE_INVALID_PARAMETER;
	}
	
	if(file!=NULL)
		fclose(file);
	
	return verr;
}

		
uLONG StatParser::GetVSize()
{
	return fVSize;
}
