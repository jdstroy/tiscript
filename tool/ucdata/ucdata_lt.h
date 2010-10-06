/* 
Lite version (with local translation tables) of original code of M. Leisher
Andrew Fedoniouk @ terrainformatica.com
*/


/*
 * Copyright 2005 Computing Research Labs, New Mexico State University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _h_ucdata_lt
#define _h_ucdata_lt

/*
 * $Id: ucdata.h,v 1.20 2005/03/24 22:39:48 mleisher Exp $
 */

#define UCDATA_VERSION "2.9"

/**************************************************************************
 *
 * Functions for library initialization and cleanup.
 *
 **************************************************************************/


/**************************************************************************
 *
 * Masks and macros for character properties.
 *
 **************************************************************************/

/*
 * Values that can appear in the `mask1' parameter of the ucisprop() and
 * ucprops() functions.
 */
#define UC_MN 0x00000001 /* Mark, Non-Spacing          */
#define UC_MC 0x00000002 /* Mark, Spacing Combining    */
#define UC_ME 0x00000004 /* Mark, Enclosing            */
#define UC_ND 0x00000008 /* Number, Decimal Digit      */
#define UC_NL 0x00000010 /* Number, Letter             */
#define UC_NO 0x00000020 /* Number, Other              */
#define UC_ZS 0x00000040 /* Separator, Space           */
#define UC_ZL 0x00000080 /* Separator, Line            */
#define UC_ZP 0x00000100 /* Separator, Paragraph       */
#define UC_CC 0x00000200 /* Other, Control             */
#define UC_CF 0x00000400 /* Other, Format              */
#define UC_OS 0x00000800 /* Other, Surrogate           */
#define UC_CO 0x00001000 /* Other, Private Use         */
#define UC_CN 0x00002000 /* Other, Not Assigned        */
#define UC_LU 0x00004000 /* Letter, Uppercase          */
#define UC_LL 0x00008000 /* Letter, Lowercase          */
#define UC_LT 0x00010000 /* Letter, Titlecase          */
#define UC_LM 0x00020000 /* Letter, Modifier           */
#define UC_LO 0x00040000 /* Letter, Other              */
#define UC_PC 0x00080000 /* Punctuation, Connector     */
#define UC_PD 0x00100000 /* Punctuation, Dash          */
#define UC_PS 0x00200000 /* Punctuation, Open          */
#define UC_PE 0x00400000 /* Punctuation, Close         */
#define UC_PO 0x00800000 /* Punctuation, Other         */
#define UC_SM 0x01000000 /* Symbol, Math               */
#define UC_SC 0x02000000 /* Symbol, Currency           */
#define UC_SK 0x04000000 /* Symbol, Modifier           */
#define UC_SO 0x08000000 /* Symbol, Other              */
#define UC_L  0x10000000 /* Left-To-Right              */
#define UC_R  0x20000000 /* Right-To-Left              */
#define UC_EN 0x40000000 /* European Number            */
#define UC_ES 0x80000000 /* European Number Separator  */

/*
 * Values that can appear in the `mask2' parameter of the ucisprop() and
 * ucprops() function.
 */
#define UC_ET 0x00000001 /* European Number Terminator */
#define UC_AN 0x00000002 /* Arabic Number              */
#define UC_CS 0x00000004 /* Common Number Separator    */
#define UC_B  0x00000008 /* Block Separator            */
#define UC_S  0x00000010 /* Segment Separator          */
#define UC_WS 0x00000020 /* Whitespace                 */
#define UC_ON 0x00000040 /* Other Neutrals             */

/*
 * Implementation specific character properties.
 */
#define UC_CM 0x00000080 /* Composite                  */
#define UC_NB 0x00000100 /* Non-Breaking               */
#define UC_SY 0x00000200 /* Symmetric                  */
#define UC_HD 0x00000400 /* Hex Digit                  */
#define UC_QM 0x00000800 /* Quote Mark                 */
#define UC_MR 0x00001000 /* Mirroring                  */
#define UC_SS 0x00002000 /* Space, other               */
#define UC_CP 0x00004000 /* Defined                    */
#define UC_BC 0x00040000 /* Bicameral (case variants)  */

/*
 * Added for UnicodeData-2.1.3.
 */
#define UC_PI 0x00008000 /* Punctuation, Initial       */
#define UC_PF 0x00010000 /* Punctuation, Final         */

/*
 * Added for UnicodeData-3.1.0.
 */
#define UC_AL 0x00020000 /* Letter, Arabic             */

/*
 * This function is used to check all flag bits and return the set of
 * bits matching the general and bidi properties of the character. This is
 * provided to improve the performance of certain algorithms that use
 * properties extensively.
 */
int ucprops (unsigned long code, unsigned long mask1,
                       unsigned long mask2, unsigned long *mask1_out,
                       unsigned long *mask2_out);

/*
 * This is the primary function for testing to see if a character has some set
 * of properties.  The macros that test for various character properties all
 * call this function with some set of masks.
 */
int ucisprop (unsigned long code, unsigned long mask1, unsigned long mask2);

#define ucisalpha(cc) ucisprop(cc, UC_LU|UC_LL|UC_LM|UC_LO|UC_LT, 0)
#define ucisdigit(cc) ucisprop(cc, UC_ND, 0)
#define ucisalnum(cc) ucisprop(cc, UC_LU|UC_LL|UC_LM|UC_LO|UC_LT|UC_ND, 0)
#define uciscntrl(cc) ucisprop(cc, UC_CC|UC_CF, 0)
#define ucisspace(cc) ucisprop(cc, UC_ZS|UC_SS, 0)
#define ucisblank(cc) ucisprop(cc, UC_ZS, 0)
#define ucispunct(cc) ucisprop(cc, UC_PD|UC_PS|UC_PE|UC_PO, UC_PI|UC_PF)
#define ucisgraph(cc) ucisprop(cc, UC_MN|UC_MC|UC_ME|UC_ND|UC_NL|UC_NO|\
                               UC_LU|UC_LL|UC_LT|UC_LM|UC_LO|UC_PC|UC_PD|\
                               UC_PS|UC_PE|UC_PO|UC_SM|UC_SM|UC_SC|UC_SK|\
                               UC_SO, UC_PI|UC_PF)
#define ucisprint(cc) ucisprop(cc, UC_MN|UC_MC|UC_ME|UC_ND|UC_NL|UC_NO|\
                               UC_LU|UC_LL|UC_LT|UC_LM|UC_LO|UC_PC|UC_PD|\
                               UC_PS|UC_PE|UC_PO|UC_SM|UC_SM|UC_SC|UC_SK|\
                               UC_SO|UC_ZS, UC_PI|UC_PF)
#define ucisupper(cc) ucisprop(cc, UC_LU, 0)
#define ucislower(cc) ucisprop(cc, UC_LL, 0)
#define ucistitle(cc) ucisprop(cc, UC_LT, 0)
#define uchascase(cc) ucisprop(cc, 0, UC_BC)
#define ucisxdigit(cc) ucisprop(cc, 0, UC_HD)

#define ucisisocntrl(cc) ucisprop(cc, UC_CC, 0)
#define ucisfmtcntrl(cc) ucisprop(cc, UC_CF, 0)

#define ucissymbol(cc) ucisprop(cc, UC_SM|UC_SC|UC_SO|UC_SK, 0)
#define ucisnumber(cc) ucisprop(cc, UC_ND|UC_NO|UC_NL, 0)
#define ucisnonspacing(cc) ucisprop(cc, UC_MN, 0)
#define ucisopenpunct(cc) ucisprop(cc, UC_PS, 0)
#define ucisclosepunct(cc) ucisprop(cc, UC_PE, 0)
#define ucisinitialpunct(cc) ucisprop(cc, 0, UC_PI)
#define ucisfinalpunct(cc) ucisprop(cc, 0, UC_PF)

#define uciscomposite(cc) ucisprop(cc, 0, UC_CM)
#define ucishex(cc) ucisprop(cc, 0, UC_HD)
#define ucisquote(cc) ucisprop(cc, 0, UC_QM)
#define ucissymmetric(cc) ucisprop(cc, 0, UC_SY)
#define ucismirroring(cc) ucisprop(cc, 0, UC_MR)
#define ucisnonbreaking(cc) ucisprop(cc, 0, UC_NB)

/*
 * Directionality macros.
 */
#define ucisrtl(cc) ucisprop(cc, UC_R, UC_AL)
#define ucisltr(cc) ucisprop(cc, UC_L, 0)
#define ucisstrong(cc) ucisprop(cc, UC_L|UC_R|UC_ND, UC_AL)
#define ucisweak(cc) ucisprop(cc, UC_EN|UC_ES, UC_ET|UC_AN|UC_CS)
#define ucisneutral(cc) ucisprop(cc, 0, UC_B|UC_S|UC_WS|UC_ON)
#define ucisseparator(cc) ucisprop(cc, 0, UC_B|UC_S)


/*
 * Very fine-grained directionality macros.  Added by request.
 */
#define ucisen(cc) ucisprop(cc, UC_EN, 0)
#define ucises(cc) ucisprop(cc, UC_ES, 0)
#define uciset(cc) ucisprop(cc, 0, UC_ET)
#define ucisan(cc) ucisprop(cc, 0, UC_AN)
#define uciscs(cc) ucisprop(cc, 0, UC_CS)
#define ucisal(cc) ucisprop(cc, 0, UC_AL)
#define ucisdfc(cc) (0x202A <= (cc) && (cc) <= 0x202E)

/*
 * Other macros inspired by John Cowan.
 */
#define ucismark(cc) ucisprop(cc, UC_MN|UC_MC|UC_ME, 0)
#define ucismodif(cc) ucisprop(cc, UC_LM, 0)
#define ucisletnum(cc) ucisprop(cc, UC_NL, 0)
#define ucisconnect(cc) ucisprop(cc, UC_PC, 0)
#define ucisdash(cc) ucisprop(cc, UC_PD, 0)
#define ucismath(cc) ucisprop(cc, UC_SM, 0)
#define uciscurrency(cc) ucisprop(cc, UC_SC, 0)
#define ucismodifsymbol(cc) ucisprop(cc, UC_SK, 0)
#define ucisnsmark(cc) ucisprop(cc, UC_MN, 0)
#define ucisspmark(cc) ucisprop(cc, UC_MC, 0)
#define ucisenclosing(cc) ucisprop(cc, UC_ME, 0)
#define ucisprivate(cc) ucisprop(cc, UC_CO, 0)
#define ucissurrogate(cc) ucisprop(cc, UC_OS, 0)
#define ucislsep(cc) ucisprop(cc, UC_ZL, 0)
#define ucispsep(cc) ucisprop(cc, UC_ZP, 0)

#define ucisidentstart(cc) ucisprop(cc, UC_LU|UC_LL|UC_LT|UC_LO|UC_NL, 0)
#define ucisidentpart(cc) ucisprop(cc, UC_LU|UC_LL|UC_LT|UC_LO|UC_NL|\
                                   UC_MN|UC_MC|UC_ND|UC_PC|UC_CF, 0)

#define ucisdefined(cc) ucisprop(cc, 0, UC_CP)
#define ucisundefined(cc) !ucisprop(cc, 0, UC_CP)

#define is_in_range(cc, mi, ma)  ((cc) >= (mi) && (cc) <= (ma))

/*
 * Other miscellaneous character property macros.
 */
inline  bool ucishan(unsigned long cc) { return is_in_range(cc,0x4e00,0x9fff) || is_in_range(cc,0xf900,0xfaff); }
inline  bool ucishangul(unsigned long cc) { return is_in_range(cc,0xac00,0xd7ff); }

inline  bool ucisideograph(unsigned long cc) { 
                     return is_in_range(cc,0x3000, 0xFAFF)
                            && !is_in_range(cc,0xD800,0xDFFF) // surrogate pairs
                            && !ucishangul(cc); }

/**************************************************************************
 *
 * Functions for case conversion.
 *
 **************************************************************************/

extern unsigned long uctoupper (unsigned long code);
extern unsigned long uctolower (unsigned long code);
extern unsigned long uctotitle (unsigned long code);

/*
 * Convert from UCS-4 to UTF-8.
 */
extern int uctoutf8 (unsigned long ch, unsigned char buf[],
                        int bufsize);

/*
 * Convert from UTF-8 to UCS-4.
 */
extern unsigned long uctoutf32 (unsigned char utf8[], int bytes);

#endif /* _h_ucdata */
