/* cs_int.h - interpreter definitions */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#ifndef __CSINT_H__
#define __CSINT_H__

/* opcodes */
#define BC_NOP        0x00    /* NOP */
#define BC_BRT        0x01    /* branch on true */
#define BC_BRF        0x02    /* branch on false */
#define BC_BR         0x03    /* branch unconditionally */
#define BC_T          0x04    /* load val with true */
#define BC_NULL       0x05    /* load val with null */
#define BC_PUSH       0x06    /* push val onto stack */
#define BC_NOT        0x07    /* logical negate top of stack */
#define BC_ADD        0x08    /* add two numeric expressions */
#define BC_SUB        0x09    /* subtract two numeric expressions */
#define BC_MUL        0x0a    /* multiply two numeric expressions */
#define BC_DIV        0x0b    /* divide two numeric expressions */
#define BC_REM        0x0c    /* remainder of two numeric expressions */
#define BC_BAND       0x0d    /* bitwise and of top two stack entries */
#define BC_BOR        0x0e    /* bitwise or of top two stack entries */
#define BC_XOR        0x0f    /* bitwise xor of top two stack entries */
#define BC_BNOT       0x10    /* bitwise not of top two stack entries */
#define BC_SHL        0x11    /* shift left top two stack entries, signed int */
#define BC_SHR        0x12    /* shift right top two stack entries, signed int */
#define BC_LT         0x13    /* less than */
#define BC_LE         0x14    /* less than or equal */
#define BC_EQ         0x15    /* equal */
#define BC_NE         0x16    /* not equal */
#define BC_GE         0x17    /* greater than or equal */
#define BC_GT         0x18    /* greater than */
#define BC_LIT        0x19    /* load literal */
#define BC_GREF       0x1a    /* load a global variable value */
#define BC_GSET       0x1b    /* set the value of a global variable */
#define BC_GETP       0x1c    /* get the value of an obj property */
#define BC_SETP       0x1d    /* set the value of an obj property and return the value */
#define BC_RETURN     0x1e    /* return from interpreter */
#define BC_CALL       0x1f    /* call a function */
#define BC_SEND       0x20    /* send a message to an obj */
#define BC_EREF       0x21    /* load an environment value */
#define BC_ESET       0x22    /* set an environment value */
#define BC_FRAME      0x23    /* push an environment frame */
#define BC_UNFRAME    0x24    /* pop an environment frame */
#define BC_VREF       0x25    /* get an element of a vector */
#define BC_VSET       0x26    /* set an element of a vector */
#define BC_NEG        0x27    /* negate top of stack */
#define BC_INC        0x28    /* increment */
#define BC_DEC        0x29    /* decrement */
#define BC_DUP2       0x2a    /* duplicate top two elements on the stack */
#define BC_DROP       0x2b    /* drop the top entry from the stack */
#define BC_DUP        0x2c    /* duplicate the top entry on the stack */
#define BC_OVER       0x2d    /* duplicate the second entry on the stack */
#define BC_NEWOBJECT  0x2e    /* create a new obj */
//#define BC_CFRAME   0x2f    /* create an environment frame */
#define BC_NEWVECTOR  0x30    /* create a new vector */
#define BC_AFRAME     0x31    /* create an argument frame */
#define BC_AFRAMER    0x32    /* create an argument frame with rest argument */
#define BC_CLOSE      0x33    /* create a closure */
#define BC_SWITCH     0x34    /* switch pdispatch */
#define BC_ARGSGE     0x35    /* argc greater than or equal to */
#define BC_PUSHSCOPE  0x36    /* push two copies of the current scope obj */
#define BC_THROW      0x37    /* throw an exception */
#define BC_UNDEFINED  0x38    /* load val with undefined */
#define BC_INSTANCEOF 0x39    /* val <- true/false  */
#define BC_TYPEOF     0x3a    /* val <- typeof (symbol) */
#define BC_EH_PUSH    0x3b    /* push error handler */
#define BC_EH_POP     0x3c    /* pop error handler */
//#define BC_EH_RETURN  0x3d    /* return in try block */
#define BC_IN         0x3e    /* val <- true/false */
#define BC_NEXT       0x3f    /* val <- next(top,val) */
#define BC_NOTHING    0x40    /* load val with inernal 'nothing' value*/
#define BC_BRDEF      0x41    /* branch on c->val != nothingValue */
#define BC_OUTPUT     0x42    /* output value */
#define BC_EQ_STRONG  0x43    /* identical */
#define BC_NE_STRONG  0x44    /* not identical */
#define BC_GETRANGE   0x45    /* make a range */
#define BC_F          0x46    /* load val with false */
#define BC_SETPM      0x47    /* set method */
#define BC_GSETC      0x48    /* set the value of a global constant */
#define BC_PUSH_NS    0x49    /* push currentScope.globals on stack */
#define BC_POP_NS     0x50    /* pops currentScope.globals from stack */
#define BC_PROTO      0x51    /* c->val = CsObjectClass(c->val) */
#define BC_BRUNDEF    0x52    /* branch on c->val == nothingValue */
#define BC_INCLUDE    0x53    /* include instruction */
#define BC_LIKE       0x54    /* val <- true/false if left matches right */
#define BC_DEBUG      0x55
#define BC_S_CALL     0x56    /* push current PC of next instruction on top of the stack and jump to instruction given by arg.  */
#define BC_S_RETURN   0x57    /* pop value of PC from top of the stack. */
#define BC_YIELD      0x58    /* N/A. */
#define BC_NEWCLASS   0x59    /* pop value of PC from top of the stack. */
#define BC_USHL       0x5a    /* shift left top two stack entries, signed int */
#define BC_USHR       0x5b    /* shift right top two stack entries, signed int */
#define BC_NS         0x5c    /* c->val = c->currentNS */

#define BC_CAR        0x5d    /* s ~/ d */
#define BC_CDR        0x5e    /* s ~% d */
#define BC_RCAR       0x5f    /* s /~ d */
#define BC_RCDR       0x60    /* s %~ d */

#define BC_GSETNS     0x61    /* set the value of a variable in current namespace */
#define BC_ROTATE     0x62    /* rotate N elements of the stack so s[n-1] = s[0], s[0] = s[1], ... */
#define BC_INCLUDE_LIBRARY 0x63 /* load native libaray */

#define BC_PRE_YIELD  0x64    /* unstack current frame and store it in c->env_yield  */

#endif
