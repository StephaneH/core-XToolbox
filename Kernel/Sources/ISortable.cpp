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
#include "ISortable.h"
#include "VMemoryCpp.h"


ISortable::ISortable()
{
}


ISortable::~ISortable()
{
}


void ISortable::MultiCriteriaSwap(ISortable** inArrays, uBYTE** inDatas, sLONG inIndexA, sLONG inIndexB)
{
	while (0 != *inArrays)
	{
		(*inArrays)->SwapElements(*inDatas, inIndexA, inIndexB);

		inDatas++;
		inArrays++;
	}
}


CompareResult ISortable::MultiCriteriaCompare(ISortable** inArrays, uBYTE** inDatas, Boolean* inInvert, sLONG inMultiCriteriaLevel, sLONG inIndexA, sLONG inIndexB)
{
	CompareResult result;
	do
	{
		result = (*inArrays)->CompareElements(*inDatas, inIndexA, inIndexB);
		if (result != CR_EQUAL)
		{
			if (*inInvert)
			{
				if (result == CR_BIGGER)
					return CR_SMALLER;
				else
					return CR_BIGGER;
			}
			else
				return result;
		}

		inInvert++;
		inDatas++;
		inArrays++;
	}
	while (--inMultiCriteriaLevel);

	return CR_EQUAL;
}


void ISortable::MultiCriteriaQSort(ISortable** inArrays, Boolean* inInvert, sLONG inMultiCriteriaLevel, sLONG inFrom, sLONG inTo)
{
    sLONG lo, hi;					/* ends of sub-array currently sorting */
    sLONG mid;						/* points to middle of subarray */
    sLONG loguy, higuy;				/* traveling pointers for partition step */
    sLONG size;						/* size of the sub-array */
    sLONG lostk[30], histk[30];
    sLONG stkptr;					/* stack for saving sub-array to be processed */
	uBYTE** datas;
	sLONG i;

    if (inTo - inFrom < 2)
        return;					/* nothing to do */

	// compute number of arrays;
	sLONG nb = 0;
	while (inArrays[nb])
	{
		nb++;
	}

	// allocate buffer for arrays
	datas = (uBYTE**) vMalloc(nb  * sizeof(uBYTE*), 'sort');
	if (!testAssert(datas != NULL))
		return;

	// lock all the used arrays
	for (i = 0; i < nb; i++)
		datas[i] = inArrays[i]->LockAndGetData();
	
	stkptr = 0;						/* initialize stack */

    lo = inFrom;
    hi = inTo;			/* initialize limits */

    if (hi - lo <= 8)
	{
		sLONG p, max;
		
		while (hi > lo)
		{
			max = lo;
			for (p = lo + 1; p <= hi; p++)
			{
				if (MultiCriteriaCompare(inArrays, datas, inInvert, inMultiCriteriaLevel, p, max) == CR_BIGGER)
					max = p;
			}
       
			MultiCriteriaSwap(inArrays, datas, max, hi);

			hi--;
		}
		goto done;
    }
    else
	{
recurse:
		size = hi - lo + 1;		/* number of el's to sort */
        mid = lo + size / 2;	/* find middle element */
		
		MultiCriteriaSwap(inArrays, datas, mid, lo);	/* swap it to beginning of array */

        loguy = lo;
        higuy = hi + 1;

		for (;;)
		{
			do 
			{
				loguy++;
			}
			while (loguy <= hi && (MultiCriteriaCompare(inArrays, datas, inInvert, inMultiCriteriaLevel, loguy, lo) <= CR_EQUAL));

			do
			{
				higuy--;
			}
			while (higuy > lo && (MultiCriteriaCompare(inArrays, datas, inInvert, inMultiCriteriaLevel, higuy, lo) >= CR_EQUAL));

			if (higuy < loguy)
				break;

			MultiCriteriaSwap(inArrays, datas, loguy, higuy);
		}
		
		MultiCriteriaSwap(inArrays, datas, lo, higuy);		/* put partition element in place */

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

done:

	for (i = 0; i < nb; i++)
		inArrays[i]->UnlockData();

	vFree(datas);
}


