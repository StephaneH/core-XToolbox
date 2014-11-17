
/* 
 *  M_APM  -  mapm_log.c
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
 *      $Id: mapm_log.c,v 1.17 2000/10/22 00:24:29 mike Exp $
 *
 *      This file contains the LOG and LOG10 functions.
 *
 *      $Log: mapm_log.c,v $
 *      Revision 1.17  2000/10/22 00:24:29  mike
 *      minor optimization
 *
 *      Revision 1.16  2000/10/21 16:22:50  mike
 *      use an improved log_near_1 algorithm
 *
 *      Revision 1.15  2000/10/20 16:49:33  mike
 *      update algorithm for basic log function and add new
 *      function when input is close to '1'
 *
 *      Revision 1.14  2000/09/23 19:48:21  mike
 *      change divide call to reciprocal
 *
 *      Revision 1.13  2000/07/11 18:58:35  mike
 *      do it right this time
 *
 *      Revision 1.12  2000/07/11 18:19:27  mike
 *      estimate a better initial precision
 *
 *      Revision 1.11  2000/05/19 16:14:15  mike
 *      update some comments
 *
 *      Revision 1.10  2000/05/17 23:47:35  mike
 *      recompute a local copy of log E base 10 on the fly
 *      if more precision is needed.
 *
 *      Revision 1.9  2000/03/27 21:44:12  mike
 *      determine how many iterations should be required at
 *      run time for log
 *
 *      Revision 1.8  1999/07/21 02:56:18  mike
 *      added some comments
 *
 *      Revision 1.7  1999/07/19 00:28:51  mike
 *      adjust local precision again
 *
 *      Revision 1.6  1999/07/19 00:10:34  mike
 *      adjust local precision during iterative loop
 *
 *      Revision 1.5  1999/07/18 23:15:54  mike
 *      change local precision dynamically and change
 *      tolerance to integers for faster iterative routine.
 *
 *      Revision 1.4  1999/06/19 21:08:32  mike
 *      changed local static variables to MAPM stack variables
 *
 *      Revision 1.3  1999/05/15 01:34:50  mike
 *      add check for number of decimal places
 *
 *      Revision 1.2  1999/05/10 21:42:32  mike
 *      added some comments
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"
#include <math.h>

static	M_APM   M_lc_log_E_base_10;
static  int	M_lc_log_E_digits = -1;

/****************************************************************************/
/*
        Calls the LOG function. The formula used is :

        log10(x)  =  A * log(x) where A = log  (e) [0.43429...] 
                                             10
*/
void	m_apm_log10(r,places,a)
M_APM	r, a;
int	places;
{
int     dplaces;
M_APM   tmp8, tmp9;

tmp8 = M_get_stack_var();
tmp9 = M_get_stack_var();

if (M_lc_log_E_digits < 0)    /* assign local copy on first call */
  {
   M_lc_log_E_digits  = VALID_DECIMAL_PLACES;
   M_lc_log_E_base_10 = m_apm_init();
   m_apm_copy(M_lc_log_E_base_10, MM_LOG_E_BASE_10);
  }

dplaces = places + 4;

/* 
 *   if our local copy of Log  (E) is not accurate enough,
 *                           10
 *   compute a more precise value and save for later.
 */

if (dplaces > M_lc_log_E_digits)
  {
   M_lc_log_E_digits = dplaces + 2;
   m_apm_log(tmp8, (dplaces + 4), MM_Ten);
   m_apm_reciprocal(M_lc_log_E_base_10, (dplaces + 4), tmp8);
  }

m_apm_log(tmp9, dplaces, a);
m_apm_multiply(tmp8, tmp9, M_lc_log_E_base_10);
m_apm_round(r, places, tmp8);
M_restore_stack(2);                    /* restore the 2 locals we used here */
}
/****************************************************************************/
void	m_apm_log(r,places,a)
M_APM	r, a;
int	places;
{
M_APM   tmp0, tmp1, last_x, guess;
int	ii, bflag, maxiter, dplaces, tolerance, local_precision;

if (a->m_apm_sign <= 0)
  {
   fprintf(stderr,"Warning! ... \'m_apm_log\', Negative argument\n");

   r->m_apm_datalength = 1;
   r->m_apm_sign       = 0;
   r->m_apm_exponent   = 0;
   r->m_apm_data[0]    = 0;
   return;
  }

tmp0   = M_get_stack_var();
tmp1   = M_get_stack_var();
last_x = M_get_stack_var();
guess  = M_get_stack_var();

tolerance = places + 4;
dplaces   = places + 16;
bflag     = FALSE;

/*
 *    if the input is real close to 1, use the series expansion
 *    to compute the log.
 *    
 *    0.9999 < a < 1.0001
 */

m_apm_subtract(tmp0, a, MM_One);

if (tmp0->m_apm_sign == 0)    /* is input exactly 1 ?? */
  {                           /* if so, result is 0    */
   r->m_apm_datalength = 1;
   r->m_apm_sign       = 0;
   r->m_apm_exponent   = 0;
   r->m_apm_data[0]    = 0;
   M_restore_stack(4);   
   return;
  }

if (tmp0->m_apm_exponent <= -4)
  {
   M_log_near_1(r, places, tmp0);
   M_restore_stack(4);   
   return;
  }

M_get_log_guess(guess, a);
m_apm_exp(tmp0, 30, guess);
m_apm_subtract(tmp1, a, tmp0);

local_precision = 16 + tmp0->m_apm_exponent - tmp1->m_apm_exponent;

if (local_precision < 30)
  local_precision = 30;

m_apm_negate(last_x, MM_Ten);

/*
 *      compute the maximum number of iterations
 *	that should be needed to calculate to
 *	the desired accuracy.  [ constant below ~= 1 / log(2) ]
 */

maxiter = (int)(log((double)(places + 2)) * 1.442695) + 1;

if (maxiter < 5)
  maxiter = 5;

/*
     Solve for the log by using the following iteration :

                           -X             -X
     X     =  X - 1 + N * e        where e   is  exp(-X)
      n+1
*/

ii = 0;

while (TRUE)
  {
   m_apm_negate(tmp1, guess);
   m_apm_exp(tmp0, local_precision, tmp1);
   m_apm_multiply(tmp1, a, tmp0); 
   m_apm_add(tmp0, tmp1, guess);
   m_apm_subtract(tmp1, tmp0, MM_One);

   if (bflag)
     break;

   m_apm_round(guess, dplaces, tmp1);

   /* force at least 2 iterations so 'last_x' has valid data */

   if (ii != 0)
     {
      m_apm_subtract(tmp0, guess, last_x);

      if (tmp0->m_apm_sign == 0)
        break;

      /*
       *   if we are within a factor of 4 on the exponent of the
       *   error term, we will be accurate enough after the *next*
       *   iteration is complete.
       */

      if ((-4 * tmp0->m_apm_exponent) > tolerance)
        bflag = TRUE;
     }

   if (ii == maxiter)
     {
      fprintf(stderr,
          "Warning! ... \'m_apm_log\', max iteration count reached\n");
      break;
     }

   if (ii == 0)
     local_precision *= 2;
   else
     local_precision += 2 - 2 * tmp0->m_apm_exponent;

   if (local_precision > dplaces)
     local_precision = dplaces;

   m_apm_copy(last_x, guess);
   ii++;
  }

m_apm_round(r, places, tmp1);
M_restore_stack(4);                    /* restore the 4 locals we used here */
}
/****************************************************************************/
/*
	calculate log (1 + x) with the following series:

              x
	y = -----      ( |y| < 1 )
	    x + 2


            [ 1 + y ]                 y^3     y^5     y^7
	log [-------] ==  2 * [ y  +  ---  +  ---  +  ---  ... ] 
            [ 1 - y ]                  3       5       7 

*/
void	M_log_near_1(rr,places,xx)
M_APM	rr, xx;
int	places;
{
M_APM   tmp0, tmp1, tmp2, tmpS, term;
int	tolerance,  local_precision;
long    m1;

tmp0 = M_get_stack_var();
tmp1 = M_get_stack_var();
tmp2 = M_get_stack_var();
tmpS = M_get_stack_var();
term = M_get_stack_var();

tolerance       = xx->m_apm_exponent - places - 4;
local_precision = places + 8 - xx->m_apm_exponent;

m_apm_add(tmp0, xx, MM_Two);
m_apm_divide(tmpS, (local_precision + 6), xx, tmp0);

m_apm_copy(term, tmpS);
m_apm_multiply(tmp0, tmpS, tmpS);
m_apm_round(tmp2, (local_precision + 6), tmp0);

m1 = 3;

while (TRUE)
  {
   m_apm_set_long(tmp1, m1);
   m_apm_multiply(tmp0, term, tmp2);
   m_apm_round(term, local_precision, tmp0);
   m_apm_divide(tmp0, local_precision, term, tmp1);
   m_apm_add(tmp1, tmpS, tmp0);

   if ((tmp0->m_apm_exponent < tolerance) || (tmp0->m_apm_sign == 0))
     break;

   m_apm_copy(tmpS, tmp1);
   m1 += 2;
  }

m_apm_multiply(tmp0, MM_Two, tmp1);
m_apm_round(rr, places, tmp0);

M_restore_stack(5);                    /* restore the 5 locals we used here */
}
/****************************************************************************/
