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
#ifndef __ISortable__
#define __ISortable__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API ISortable
{
public:
			ISortable ();
	virtual ~ISortable ();

	static void	MultiCriteriaQSort (ISortable** inArrays, Boolean* inInvert, sLONG inMultiCriteriaLevel, sLONG inFrom, sLONG inTo);

protected:
	virtual uBYTE*	LockAndGetData () const { return NULL; };
	virtual void	UnlockData () const {};
	
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB) = 0;
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB) = 0;

	static CompareResult	MultiCriteriaCompare (ISortable** inArrays, uBYTE** inDatas, Boolean* inInvert, sLONG inMultiCriteriaLevel, sLONG inIndexA, sLONG inIndexB);
	static void	MultiCriteriaSwap (ISortable** inArrays, uBYTE** inDatas, sLONG inIndexA, sLONG inIndexB);
};


// This is an optimized quicksort routines for arrays of standard types like short, long or double

#define CUTOFF 8

template <class Type> void QSort (Type *inBase, sLONG inNb, Boolean inInvert)
{
    Type *lo, *hi;					/* ends of sub-array currently sorting */
    Type *mid;						/* points to middle of subarray */
    Type *loguy, *higuy;			/* traveling pointers for partition step */
	Type tmp;
    sLONG size;						/* size of the sub-array */
    Type *lostk[30], *histk[30];
    sLONG stkptr;					/* stack for saving sub-array to be processed */

    if (inNb < 2)
        return;						/* nothing to do */

    stkptr = 0;						/* initialize stack */

    lo = inBase;
    hi = inBase + inNb - 1;			/* initialize limits */

    if (hi - lo <= CUTOFF)
	{
		Type *p, *max;
		
		while (hi > lo)
		{
			max = lo;
			for (p = lo + 1; p <= hi; p++)
			{
				if (inInvert)
				{
					if (*p < *max)
						max = p;
				}
				else
				{
					if (*p > *max)
						max = p;
				}
			}
        
			tmp = *max;
			*max = *hi;
			*hi = tmp;

			hi--;
		}
		return;
    }
    else
	{
recurse:
	    size = (sLONG) (hi - lo + 1);		/* number of el's to sort */
		mid = lo + size / 2;	/* find middle element */
		
		tmp = *mid;
		*mid = *lo;
		*lo = tmp;				/* swap it to beginning of array */

        loguy = lo;
        higuy = hi + 1;

        if (inInvert)
		{
			for (;;)
			{
				do 
				{
					loguy++;
				}
				while (loguy <= hi && *loguy >= *lo);

				do
				{
					higuy--;
				}
				while (higuy > lo && *higuy <= *lo);

				if (higuy < loguy)
					break;

				tmp = *loguy;
				*loguy = *higuy;
				*higuy = tmp;
			}
		}
		else
		{
			for (;;)
			{
				do 
				{
					loguy++;
				}
				while (loguy <= hi && *loguy <= *lo);

				do
				{
					higuy--;
				}
				while (higuy > lo && *higuy >= *lo);

				if (higuy < loguy)
					break;

				tmp = *loguy;
				*loguy = *higuy;
				*higuy = tmp;
			}
		}
        tmp = *lo;
		*lo = *higuy;
		*higuy = tmp;					/* put partition element in place */

        if (higuy - 1 - lo >= hi - loguy)
		{
            if (lo + 1 < higuy)
			{
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 1;
                ++stkptr;
            }							/* save big recursion for later */

            if (loguy < hi)
			{
                lo = loguy;
                goto recurse;			/* do small recursion */
            }
        }
        else
		{
            if (loguy < hi) 
			{
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;			/* save big recursion for later */
            }

            if (lo + 1 < higuy)
			{
                hi = higuy - 1;
                goto recurse;		/* do small recursion */
            }
        }
    }

    --stkptr;
    if (stkptr >= 0)
	{
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;			/* pop subarray from stack */
    }
    else
        return;					/* all subarrays done */
}

END_TOOLBOX_NAMESPACE

#endif