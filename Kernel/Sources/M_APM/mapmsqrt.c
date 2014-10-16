
/* 
 *  M_APM  -  mapmsqrt.c
 *
 *  Copyright (C) 1999, 2000   Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted, 
 *  provided that the above copyright notice appear in all copies and 
 *  that both that copyright notice and this permission notice appear 
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted, but not the right to
 *  distribute the modified code.  Modifications are to be distributed 
 *  as patches to released version.
 *  
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapmsqrt.c,v 1.12 2000/09/26 18:32:04 mike Exp $
 *
 *      This file contains the SQRT function.
 *
 *      $Log: mapmsqrt.c,v $
 *      Revision 1.12  2000/09/26 18:32:04  mike
 *      use new algorithm which only uses multiply and subtract
 *      (avoids the slower version which used division)
 *
 *      Revision 1.11  2000/07/11 17:56:22  mike
 *      make better estimate for initial precision
 *
 *      Revision 1.10  1999/07/21 02:48:45  mike
 *      added some comments
 *
 *      Revision 1.9  1999/07/19 00:25:44  mike
 *      adjust local precision again
 *
 *      Revision 1.8  1999/07/19 00:09:41  mike
 *      adjust local precision during loop
 *
 *      Revision 1.7  1999/07/18 22:57:08  mike
 *      change to dynamically changing local precision and
 *      change tolerance checks using integers
 *
 *      Revision 1.6  1999/06/19 21:18:00  mike
 *      changed local static variables to MAPM stack variables
 *
 *      Revision 1.5  1999/05/31 01:40:39  mike
 *      minor update to normalizing the exponent
 *
 *      Revision 1.4  1999/05/31 01:21:41  mike
 *      optimize for large exponents
 *
 *      Revision 1.3  1999/05/12 20:59:35  mike
 *      use a better 'guess' function
 *
 *      Revision 1.2  1999/05/10 21:15:26  mike
 *      added some comments
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_sqrt(rr,places,aa)
M_APM	rr, aa;
int	places;
{
M_APM   last_x, guess, tmpN, tmp8, tmp9;
int	ii, jj, bflag, nexp, tolerance, dplaces;

if (aa->m_apm_sign <= 0)
  {
   if (aa->m_apm_sign == -1)
     fprintf(stderr,"Warning! ... \'m_apm_sqrt\', Negative argument\n");

   rr->m_apm_datalength = 1;
   rr->m_apm_sign       = 0;
   rr->m_apm_exponent   = 0;
   rr->m_apm_data[0]    = 0;
   return;
  }

last_x = M_get_stack_var();
guess  = M_get_stack_var();
tmpN   = M_get_stack_var();
tmp8   = M_get_stack_var();
tmp9   = M_get_stack_var();

m_apm_copy(tmpN, aa);
nexp = 0;

/* 
    normalize the input number (make the exponent near 0) so
    the 'guess' function will not over/under flow on large
    magnitude exponents.
*/

if (aa->m_apm_exponent > 2)
  {
   nexp = aa->m_apm_exponent / 2;
   tmpN->m_apm_exponent -= 2 * nexp;
  }

if (aa->m_apm_exponent < -2)
  {
   nexp = aa->m_apm_exponent / 2 - 1;
   tmpN->m_apm_exponent -= 2 * nexp;
  }

M_get_sqrt_guess(guess, tmpN);    /* actually gets 1/sqrt guess */

tolerance = places + 4;
dplaces   = places + 16;
bflag     = FALSE;

m_apm_negate(last_x, MM_Ten);

/*   Use the following iteration to calculate 1 / sqrt(N) :

         X    =  0.5 * X * [ 3 - N * X^2 ]
          n+1                    
*/

ii = 0;

while (TRUE)
  {
   m_apm_multiply(tmp9, tmpN, guess);
   m_apm_multiply(tmp8, tmp9, guess);
   m_apm_subtract(tmp9, MM_Three, tmp8);
   m_apm_multiply(tmp8, tmp9, guess);
   m_apm_multiply(tmp9, tmp8, MM_0_5);

   if (bflag)
     break;

   m_apm_round(guess, dplaces, tmp9);

   /* force at least 2 iterations so 'last_x' has valid data */

   if (ii != 0)
     {
      m_apm_subtract(tmp8, guess, last_x);

      if (tmp8->m_apm_sign == 0)
        break;

      /* 
       *   if we are within a factor of ~4 on the error term,
       *   we will be accurate enough after the **next** iteration
       *   is complete. we will use a ratio of 3.8 (or 19/5)
       *   to have a little safety net. (note that the sign of
       *   the exponent on the error term will be a negative number).
       */

      jj = (-19 * tmp8->m_apm_exponent) / 5;

      if (jj > tolerance)
        bflag = TRUE;
     }

   m_apm_copy(last_x, guess);
   ii++;
  }

/*
 *    multiply by the starting number to get the final
 *    sqrt and then adjust the exponent since we found
 *    the sqrt of the normalized number.
 */

m_apm_multiply(tmp8, tmp9, tmpN);
m_apm_round(rr, places, tmp8);
rr->m_apm_exponent += nexp;

M_restore_stack(5);
}
/****************************************************************************/
