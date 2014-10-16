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
#include "VObject.h"
#include "VProcess.h"
#include "VMemoryCpp.h"
// #include "VMemorySlot.h"
//#include "VErrorContext.h"



////////////////////////////////////////////////////////////////////////////////
//
// Some intersting doc for shared librarys and linux :
//
// - http://tldp.org/HOWTO/Program-Library-HOWTO/miscellaneous.html#INIT-AND-CLEANUP
// - http://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Function-Attributes.html#Function-Attributes
//
////////////////////////////////////////////////////////////////////////////////



extern "C"
{
    void __attribute__ ((constructor)) LibraryInit();
    void __attribute__ ((destructor)) LibraryFini();
}


//jmo - revoir ca : tentative de correction d'un bug lie au delete du CppMemMgr
//      (voir delete en commentaire dans la version Windows)
static int gDeleteCount=INT_MIN;


void LibraryInit()
{
    gDeleteCount++;

	if(VObject::GetMainMemMgr() == NULL)
    {
        gDeleteCount=0;
		VObject::_ProcessEntered(new VCppMemMgr);
    }
}


void LibraryFini()
{
	// In case we crashed, VProcess::sProcess may not be deleted when we get there.
	// so let's ensure VProcess::sProcess is NULL to tell mem managers the game is over.
	VProcess::_ProcessExited();

    if(!gDeleteCount--)
    {
        VCppMemMgr*	cppMgr=VObject::GetAllocator();
        xbox_assert(cppMgr!=NULL);
    
        if(cppMgr)
            delete cppMgr;

        VObject::_ProcessExited();
    }
}
