
/* 
 *  M_APM  -  m_apm.h
 *
 *  Copyright (C) 1999 - 2001   Michael C. Ring
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
 *  This software is provided  "as is" without express or implied warranty.
 */

/*
 *      This is the header file that the user will include.
 *
 *      $Log: m_apm.h,v $
 *      Revision 1.21  2001/03/25 21:24:55  mike
 *      add floor and ceil functions
 *
 *      Revision 1.20  2000/09/23 19:05:29  mike
 *      add _reciprocal prototype
 *
 *      Revision 1.19  2000/08/21 23:30:13  mike
 *      add _is_integer function
 *
 *      Revision 1.18  2000/07/06 00:10:15  mike
 *      redo declare for MM_cpp_min_precision
 *
 *      Revision 1.17  2000/07/04 20:59:43  mike
 *      move MM_cpp_min_precision into cplusplus block below
 *
 *      Revision 1.16  2000/07/04 20:49:04  mike
 *      move 'MM_cpp_min_precision' inside the extern "C"
 *      brackets
 *
 *      Revision 1.15  2000/04/06 21:19:38  mike
 *      minor final tweaks from Orion
 *
 *      Revision 1.14  2000/04/05 20:15:25  mike
 *      add cpp_min_precision
 *
 *      Revision 1.13  2000/04/04 22:20:09  mike
 *      updated some comments from Orion
 *
 *      Revision 1.12  2000/04/04 19:46:36  mike
 *      fix preincrement, postincrement operators
 *      added some comments
 *      added 'ipow' operators
 *
 *      Revision 1.11  2000/04/03 22:08:35  mike
 *      added VFloat C++ wrapper class
 *      supplied by Orion Sky Lawlor (olawlor@acm.org)
 *
 *      Revision 1.10  2000/04/03 18:40:28  mike
 *      add #define atan2 for alias
 *
 *      Revision 1.9  2000/04/03 18:05:23  mike
 *      added hyperbolic functions
 *
 *      Revision 1.8  2000/04/03 17:26:57  mike
 *      add cbrt prototype
 *
 *      Revision 1.7  1999/09/18 03:11:23  mike
 *      add new prototype
 *
 *      Revision 1.6  1999/09/18 03:08:25  mike
 *      add new prototypes
 *
 *      Revision 1.5  1999/09/18 01:37:55  mike
 *      added new prototype
 *
 *      Revision 1.4  1999/07/12 02:04:30  mike
 *      added new function prototpye (m_apm_integer_string)
 *
 *      Revision 1.3  1999/05/15 21:04:08  mike
 *      added factorial prototype
 *
 *      Revision 1.2  1999/05/12 20:50:12  mike
 *      added more constants
 *
 *      Revision 1.1  1999/05/12 20:48:25  mike
 *      Initial revision
 *
 *      $Id: m_apm.h,v 1.21 2001/03/25 21:24:55 mike Exp $
 */

#ifndef M__APM__INCLUDED
#define M__APM__INCLUDED

#ifdef __cplusplus
/* Comment this line out if you've compiled the library as C++. */
#define APM_CONVERT_FROM_C
#endif

#ifdef APM_CONVERT_FROM_C
extern "C" {
#endif

typedef unsigned char UCHAR;

struct  M_APM_struct {
	long m_apm_id;
	long m_apm_malloclength;
	long m_apm_datalength;
	long m_apm_exponent;
	char m_apm_sign;
	UCHAR *m_apm_data;
};

typedef struct M_APM_struct *M_APM;

/*
 *	convienient predefined constants
 */

extern	M_APM	MM_Zero;
extern	M_APM	MM_One;
extern	M_APM	MM_Two;
extern	M_APM	MM_Three;
extern	M_APM	MM_Four;
extern	M_APM	MM_Five;
extern	M_APM	MM_Ten;

extern	M_APM	MM_PI;
extern	M_APM	MM_HALF_PI;
extern	M_APM	MM_2_PI;
extern	M_APM	MM_E;

extern	M_APM	MM_LOG_E_BASE_10;
extern	M_APM	MM_LOG_10_BASE_E;
extern	M_APM	MM_LOG_2_BASE_E;
extern	M_APM	MM_LOG_3_BASE_E;


/*
 *	function prototypes
 */

extern	M_APM	m_apm_init(void);
extern	void	m_apm_free(M_APM);
extern	void	m_apm_set_string(M_APM, char *);
extern	void	m_apm_set_double(M_APM, double);
extern	void	m_apm_set_long(M_APM, long);

extern	void	m_apm_to_string(char *, int, M_APM);
extern  void    m_apm_to_integer_string(char *, M_APM);
extern	void	m_apm_to_fixpt_string(char *, int, M_APM);

extern	void	m_apm_absolute_value(M_APM, M_APM);
extern	void	m_apm_negate(M_APM, M_APM);
extern	void	m_apm_copy(M_APM, M_APM);
extern	void	m_apm_round(M_APM, int, M_APM);
extern	int	m_apm_compare(M_APM, M_APM);
extern	int	m_apm_sign(M_APM);
extern	int	m_apm_exponent(M_APM);
extern	int	m_apm_significant_digits(M_APM);
extern	int	m_apm_is_integer(M_APM);

extern	void	m_apm_add(M_APM, M_APM, M_APM);
extern	void	m_apm_subtract(M_APM, M_APM, M_APM);
extern	void	m_apm_multiply(M_APM, M_APM, M_APM);
extern	void	m_apm_divide(M_APM, int, M_APM, M_APM);
extern	void	m_apm_integer_divide(M_APM, M_APM, M_APM);
extern	void	m_apm_integer_div_rem(M_APM, M_APM, M_APM, M_APM);
extern	void	m_apm_reciprocal(M_APM, int, M_APM);
extern	void	m_apm_factorial(M_APM, M_APM);
extern	void	m_apm_floor(M_APM, M_APM);
extern	void	m_apm_ceil(M_APM, M_APM);
extern	void	m_apm_get_random(M_APM);

extern	void	m_apm_sqrt(M_APM, int, M_APM);
extern	void	m_apm_cbrt(M_APM, int, M_APM);
extern	void	m_apm_log(M_APM, int, M_APM);
extern	void	m_apm_log10(M_APM, int, M_APM);
extern	void	m_apm_exp(M_APM, int, M_APM);
extern	void	m_apm_pow(M_APM, int, M_APM, M_APM);
extern  void	m_apm_integer_pow(M_APM, int, M_APM, int);

extern	void	m_apm_sin_cos(M_APM, M_APM, int, M_APM);
extern	void	m_apm_sin(M_APM, int, M_APM);
extern	void	m_apm_cos(M_APM, int, M_APM);
extern	void	m_apm_tan(M_APM, int, M_APM);
extern	void	m_apm_arcsin(M_APM, int, M_APM);
extern	void	m_apm_arccos(M_APM, int, M_APM);
extern	void	m_apm_arctan(M_APM, int, M_APM);
extern	void	m_apm_arctan2(M_APM, int, M_APM, M_APM);

extern  void    m_apm_sinh(M_APM, int, M_APM);
extern  void    m_apm_cosh(M_APM, int, M_APM);
extern  void    m_apm_tanh(M_APM, int, M_APM);
extern  void    m_apm_arcsinh(M_APM, int, M_APM);
extern  void    m_apm_arccosh(M_APM, int, M_APM);
extern  void    m_apm_arctanh(M_APM, int, M_APM);

extern  void    m_apm_cpp_precision(int);   /* only for C++ wrapper */

/* more intuitive alternate names for the ARC functions ... */

#define m_apm_asin m_apm_arcsin
#define m_apm_acos m_apm_arccos
#define m_apm_atan m_apm_arctan
#define m_apm_atan2 m_apm_arctan2

#define m_apm_asinh m_apm_arcsinh
#define m_apm_acosh m_apm_arccosh
#define m_apm_atanh m_apm_arctanh

#ifdef APM_CONVERT_FROM_C
}      /* End extern "C" bracket */
#endif

#ifdef __cplusplus   /*<- Hides the class below from C compilers */

/*
    This class lets you use M_APM's a bit more intuitively with
    C++'s operator and function overloading, constructors, etc.

    Added 3/24/2000 by Orion Sky Lawlor, olawlor@acm.org
*/

extern 
#ifdef APM_CONVERT_FROM_C
"C" 
#endif

#endif
#endif

