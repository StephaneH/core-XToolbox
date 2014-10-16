
/* 
 *  M_APM  -  mapmistr.c
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
 *      $Id: mapmistr.c,v 1.3 2000/02/03 22:48:38 mike Exp $
 *
 *      This file contains M_APM -> integer string function
 *
 *      $Log: mapmistr.c,v $
 *      Revision 1.3  2000/02/03 22:48:38  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.2  1999/07/18 01:33:04  mike
 *      minor tweak to code alignment
 *
 *      Revision 1.1  1999/07/12 02:06:08  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_to_integer_string(s,mtmp)
char	*s;
M_APM	mtmp;
{
void    *vp;
char	*cp, *cpdest, ch, lbuf[256];
int	ct;

vp = NULL;
ct = mtmp->m_apm_exponent;

if (ct <= 0 || mtmp->m_apm_sign == 0)
  {
   strcpy(s, "0");
   return;
  }

if (ct > 240)
  {
   if ((vp = (void *)MAPM_MALLOC((ct + 32) * sizeof(char))) == NULL)
     {
      fprintf(stderr,"\'m_apm_to_integer_string\', Out of memory\n");
      exit(16);
     }

   cp = (char *)vp;
  }
else
  {
   cp = lbuf;
  }

m_apm_to_string(cp, (ct + 2), mtmp);

cpdest = s;
*cpdest++ = *cp++;

if (mtmp->m_apm_sign == 1)
  ct--;

while (TRUE)
  {
   if (ct == 0)
     break;

   if ((ch = *cp++) != '.')
     {
      *cpdest++ = ch;
      ct--;
     }
  }

*cpdest = '\0';

if (vp != NULL)
  MAPM_FREE(vp);
}

/****************************************************************************/

void	M_set_to_zero(M_APM z)
{
z->m_apm_datalength = 1;
z->m_apm_sign       = 0;
z->m_apm_exponent   = 0;
z->m_apm_data[0]    = 0;
}

void	M_apm_round_fixpt(M_APM btmp, int places, M_APM atmp)
{
int	xp, ii;

xp = atmp->m_apm_exponent;
ii = xp + places - 1;

M_set_to_zero(btmp); /* assume number is too small so the net result is 0 */

if (ii >= 0)
  {
   m_apm_round(btmp, ii, atmp);
  }
else
  {
   if (ii == -1)	/* next digit is significant which may round up */
     {
      if (atmp->m_apm_data[0] >= 50)	/* digit >= 5, round up */
        {
         m_apm_copy(btmp, atmp);
	 btmp->m_apm_data[0] = 10;
	 btmp->m_apm_exponent += 1;
	 btmp->m_apm_datalength = 1;
	 M_apm_normalize(btmp);
	}
     }
  }
}


void	m_apm_to_fixpt_string(char *ss, int dplaces, M_APM mtmp)
{
M_APM   ctmp;
void	*vp;
int	places, i2, ii, jj, kk, xp, dl, numb;
UCHAR   *ucp, numdiv, numrem;
char	*cpw, *cpd, sbuf[128];

ctmp   = M_get_stack_var();
vp     = NULL;
cpd    = ss;
places = dplaces;

/* just want integer portion if places == 0 */

if (places == 0)
  {
   if (mtmp->m_apm_sign >= 0)
     m_apm_add(ctmp, mtmp, MM_0_5);
   else
     m_apm_subtract(ctmp, mtmp, MM_0_5);

   m_apm_to_integer_string(cpd, ctmp);

   M_restore_stack(1);
   return;
  }

if (places > 0)
  M_apm_round_fixpt(ctmp, places, mtmp);
else
  m_apm_copy(ctmp, mtmp);	  /* show ALL digits */

if (ctmp->m_apm_sign == 0)        /* result is 0 */
  {
   if (places < 0)
     {
      cpd[0] = '0';		  /* "0.0" */
      cpd[1] = '.';
      cpd[2] = '0';
      cpd[3] = '\0';
     }
   else
     {
      memset(cpd, '0', (places + 2));	/* pre-load string with all '0' */
      cpd[1] = '.';
      cpd[places + 2] = '\0';
     }

   M_restore_stack(1);
   return;
  }

xp   = ctmp->m_apm_exponent;
dl   = ctmp->m_apm_datalength;
numb = (dl + 1) >> 1;

if (places < 0)
  {
   if (dl > xp)
     jj = dl + 16;
   else
     jj = xp + 16;
  }
else
  {
   jj = places + 16;
   
   if (xp > 0)
     jj += xp;
  }

if (jj > 112)
  {
   if ((vp = (void *)MAPM_MALLOC((jj + 16) * sizeof(char))) == NULL)
     {
      /* fatal, this does not return */
      fprintf(stderr,"\'m_apm_to_fixpt_string\', Out of memory\n");
      exit(16);
     }

   cpw = (char *)vp;
  }
else
  {
   cpw = sbuf;
  }

/*
 *  at this point, the number is non-zero and the the output
 *  string will contain at least 1 significant digit.
 */

if (ctmp->m_apm_sign == -1) 	  /* negative number */
  {
   *cpd++ = '-';
  }

ucp = ctmp->m_apm_data;
ii  = 0;

/* convert MAPM num to ASCII digits and store in working char array */

while (TRUE)
  {
   M_get_div_rem_10((int)(*ucp++), &numdiv, &numrem);

   cpw[ii++] = numdiv + '0';
   cpw[ii++] = numrem + '0';

   if (--numb == 0)
     break;
  }

i2 = ii;		/* save for later */

if (places < 0)		/* show ALL digits */
  {
   places = dl - xp;

   if (places < 1)
     places = 1;
  }

/* pad with trailing zeros if needed */

kk = xp + places + 2 - ii;

if (kk > 0)
  memset(&cpw[ii], '0', kk);

if (xp > 0)          /* |num| >= 1, NO lead-in "0.nnn" */
  {
   ii = xp + places + 1;
   jj = 0;

   for (kk=0; kk < ii; kk++)
     {
      if (kk == xp)
        cpd[jj++] = '.';

      cpd[jj++] = cpw[kk];
     }

   cpd[ii] = '\0';
  }
else			/* |num| < 1, have lead-in "0.nnn" */
  {
   jj = 2 - xp;
   ii = 2 + places;
   memset(cpd, '0', (ii + 1));	/* pre-load string with all '0' */
   cpd[1] = '.';		/* assign decimal point */

   for (kk=0; kk < i2; kk++)
     {
      cpd[jj++] = cpw[kk];
     }

   cpd[ii] = '\0';
  }

if (vp != NULL)
  MAPM_FREE(vp);

M_restore_stack(1);
}

/****************************************************************************/
