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
#include "Kernel/Sources/VTask.h"

#include "VInterlocked.h"


sLONG VInterlocked::Increment( sLONG* inValue)
{
#if VERSIONWIN

    return ::InterlockedIncrement( inValue);

#elif VERSIONMAC

    return ::OSAtomicAdd32Barrier( 1, reinterpret_cast<int32_t*>( inValue));

#elif VERSION_LINUX

    return __sync_add_and_fetch(inValue, 1);

#endif
}


sLONG VInterlocked::Decrement( sLONG* inValue)
{
#if VERSIONWIN

    // attention sur 95 et NT351, seule le signe de la valeur retournee est significatif ! 
    // sur 98 et nt4 ca retourne bien la valeur apres modif
    return ::InterlockedDecrement( inValue);

#elif VERSIONMAC

    return ::OSAtomicAdd32Barrier( -1, reinterpret_cast<int32_t*>( inValue));

#elif VERSION_LINUX

    return __sync_sub_and_fetch(inValue, 1);

#endif
}


sLONG VInterlocked::AtomicAdd( sLONG* inValue, sLONG inAddValue)
{
#if VERSIONWIN

    // attention sur 95 et NT351, seule le signe de la valeur retournee est significatif ! 
    // sur 98 et nt4 ca retourne bien la valeur apres modif

    //msdn : The function performs an atomic addition of Value to the value pointed to by Addend. The result is
    //stored in the address specified by Addend. The function returns the initial value of the variable pointed to
    //by Addend.
    return ::InterlockedExchangeAdd( inValue, inAddValue);

#elif VERSIONMAC

    return ::OSAtomicAdd32Barrier( inAddValue, reinterpret_cast<int32_t*>( inValue));

#elif VERSION_LINUX

    return __sync_fetch_and_add(inValue, inAddValue);

#endif
}


sLONG VInterlocked::CompareExchange( sLONG* inValue, sLONG inCompareValue, sLONG inNewValue)
{
#if VERSIONWIN

    sLONG val = ::InterlockedCompareExchange( inValue, inNewValue, inCompareValue);

#elif VERSIONMAC

    sLONG val;
    do {
        if (::OSAtomicCompareAndSwap32Barrier((int32_t)inCompareValue, (int32_t) inNewValue, reinterpret_cast<int32_t*>(inValue)))
        {
            return inCompareValue;
        }
        // one must loop if a swap occured between reading the old val and a failed CAS
        // because if we return inCompareValue, the caller may assume that the CAS has succeeded.
        val = *inValue;
    } while(val == inCompareValue);
    
#elif VERSION_LINUX
 
    sLONG val=__sync_val_compare_and_swap(inValue, inCompareValue, inNewValue);
    
#endif
		
    return val;
}


void* VInterlocked::CompareExchangePtr(void** inValue, void* inCompareValue, void* inNewValue)
{
#if VERSIONWIN

	// requires Win98 or NT4

	// pp 27/02/01
	// header 0x0500
	void* val = ::InterlockedCompareExchangePointer(inValue, inNewValue, inCompareValue);
	// header 0x0401
	//void* val = ::InterlockedCompareExchange(inValue, inNewValue, inCompareValue);

#elif VERSIONMAC

	void* val;
	do {
		if (::OSAtomicCompareAndSwapPtrBarrier( inCompareValue, inNewValue, inValue))
		{
			return inCompareValue;
		}
		// one must loop if a swap occured between reading the old val and a failed CAS
		// because if we return inCompareValue, the caller may assume that the CAS has succeeded.
		val = *inValue;
	} while(val == inCompareValue);

#elif VERSION_LINUX

    //Sous Linux (avec GCC) on a les deux parfums !
    //bool __sync_bool_compare_and_swap (type *ptr, type oldval, type newval, ...)
    void* val=__sync_val_compare_and_swap(inValue, inCompareValue, inNewValue);

#endif

	return val;

}


sLONG VInterlocked::Exchange(sLONG* inValue, sLONG inNewValue)
{
#if VERSIONWIN

	sLONG val = ::InterlockedExchange(inValue, inNewValue);

#elif VERSIONMAC

	sLONG val;
	do {
		val = *inValue;
		// one must loop if a swap occured between reading the old val and a failed CAS
		// because if we return the old val, the caller may assume that the CAS has succeeded.
	} while(!::OSAtomicCompareAndSwap32Barrier((int32_t) val, (int32_t) inNewValue, (int32_t*) inValue));

#elif VERSION_LINUX

    sLONG val=__sync_lock_test_and_set(inValue, inNewValue);

#endif

	return val;
}


#if ARCH_64
sLONG8 VInterlocked::Exchange(sLONG8* inValue, sLONG8 inNewValue)
{
	sLONG8 val;

#if VERSIONWIN

	//jmo - Ugly workaround for win XP lack of InterlockedCompareExchange64
	//      We use the VoidPtr version to achieve the same result on 64 bit platforms.
	
	val=(sLONG8)VInterlocked::ExchangeVoidPtr((void**)inValue, (void*)inNewValue);

#elif VERSIONMAC

	do {
		val = *inValue;
		// one must loop if a swap occured between reading the old val and a failed CAS
		// because if we return the old val, the caller may assume that the CAS has succeeded.
	} while(!::OSAtomicCompareAndSwap64Barrier((int64_t) val, (int64_t) inNewValue, (int64_t*) inValue));

#elif VERSION_LINUX

    val=__sync_lock_test_and_set(inValue, inNewValue);

#endif

	return val;

}
#endif

void* VInterlocked::ExchangeVoidPtr(void** inValue, void* inNewValue)
{
#if VERSIONWIN

	#pragma warning (push)
	#pragma warning (disable: 4311)
	#pragma warning (disable: 4312)

	void* val = InterlockedExchangePointer( inValue, inNewValue);
//	void* val = (void*) ::InterlockedExchange((long*) inValue, (long)inNewValue);

	#pragma warning (pop)

#elif VERSIONMAC

	void* val;
	do {
		val = *inValue;
		// one must loop if a swap occured between reading the old val and a failed CAS
		// because if we return the old val, the caller may assume that the CAS has succeeded.
	} while(!::OSAtomicCompareAndSwapPtrBarrier( val, inNewValue, inValue));

#elif VERSION_LINUX

    void* val=__sync_lock_test_and_set(inValue, inNewValue);

#endif
	return val;
}


bool VInterlocked::TestAndSet( sLONG *inValue, sLONG inBitNumber)
{
    xbox_assert( inBitNumber >= 0 && inBitNumber <= 31);

#if VERSIONMAC

    // works on byte ((char*)theAddress + (n>>3))
    // and operate on bit (0x80>>(n&7))
    #if SMALLENDIAN
    return ::OSAtomicTestAndSetBarrier( inBitNumber ^ 7, inValue);
    #else
    return ::OSAtomicTestAndSetBarrier( 31 - inBitNumber, inValue);
    #endif

#elif VERSION_LINUX

    uLONG mask=0x1<<inBitNumber;
    sLONG val=__sync_fetch_and_or(inValue, mask);
    
    return (val & mask ? true : false);

#elif VERSIONWIN

    // always work on a long
    return InterlockedBitTestAndSet( (LONG*) inValue, inBitNumber) != 0;

#endif
}


bool VInterlocked::TestAndClear( sLONG *inValue, sLONG inBitNumber)
{
    xbox_assert( inBitNumber >= 0 && inBitNumber <= 31);

#if VERSIONMAC

    #if SMALLENDIAN
    return ::OSAtomicTestAndClearBarrier( inBitNumber ^ 7, inValue);
    #else
    return ::OSAtomicTestAndClearBarrier( 31 - inBitNumber, inValue);
    #endif

#elif VERSION_LINUX

    uLONG mask=~(0x1<<inBitNumber);
    sLONG val=__sync_fetch_and_and(inValue, mask);
    
    return (val & mask ? true : false);

#elif VERSIONWIN

    return InterlockedBitTestAndReset( (LONG*)inValue, inBitNumber) != 0;

#endif
}
