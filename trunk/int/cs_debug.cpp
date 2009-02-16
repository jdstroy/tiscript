/* debug.c - debug routines */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <stdio.h>
#include <string.h>
#include "cs.h"
#include "cs_int.h"

namespace tis 
{

/* instruction output formats */
#define FMT_NONE        0
#define FMT_BYTE        1
#define FMT_2BYTE       2
#define FMT_WORD        3
#define FMT_LIT         4
#define FMT_SWITCH      5

typedef struct { int ot_code; char *ot_name; int ot_fmt; } OTDEF;
OTDEF otab[] = {
{       BC_BRT,       "BRT",          FMT_WORD        },
{       BC_BRF,       "BRF",          FMT_WORD        },
{       BC_BR,        "BR",           FMT_WORD        },
{       BC_T,         "T",            FMT_NONE        },
{       BC_NULL,      "NULL",         FMT_NONE        },
{       BC_PUSH,      "PUSH",         FMT_NONE        },
{       BC_NOT,       "NOT",          FMT_NONE        },
{       BC_ADD,       "ADD",          FMT_NONE        },
{       BC_SUB,       "SUB",          FMT_NONE        },
{       BC_MUL,       "MUL",          FMT_NONE        },
{       BC_DIV,       "DIV",          FMT_NONE        },
{       BC_REM,       "REM",          FMT_NONE        },
{       BC_BAND,      "BAND",         FMT_NONE        },
{       BC_BOR,       "BOR",          FMT_NONE        },
{       BC_XOR,       "XOR",          FMT_NONE        },
{       BC_BNOT,      "BNOT",         FMT_NONE        },
{       BC_SHL,       "SHL",          FMT_NONE        },
{       BC_SHR,       "SHR",          FMT_NONE        },
{       BC_LT,        "LT",           FMT_NONE        },
{       BC_LE,        "LE",           FMT_NONE        },
{       BC_EQ,        "EQ",           FMT_NONE        },
{       BC_NE,        "NE",           FMT_NONE        },
{       BC_GE,        "GE",           FMT_NONE        },
{       BC_GT,        "GT",           FMT_NONE        },
{       BC_LIT,       "LIT",          FMT_LIT         },
{       BC_GREF,      "GREF",         FMT_LIT         },
{       BC_GSET,      "GSET",         FMT_LIT         },
{       BC_GETP,      "GETP",         FMT_NONE        },
{       BC_SETP,      "SETP",         FMT_NONE        },
{       BC_RETURN,    "RETURN",       FMT_NONE        },
{       BC_CALL,      "CALL",         FMT_BYTE        },
{       BC_SEND,      "SEND",         FMT_BYTE        },
{       BC_EREF,      "EREF",         FMT_2BYTE       },
{       BC_ESET,      "ESET",         FMT_2BYTE       },
{       BC_FRAME,     "FRAME",        FMT_BYTE        },
{       BC_CFRAME,    "CFRAME",       FMT_BYTE        },
{       BC_UNFRAME,   "UNFRAME",      FMT_NONE        },
{       BC_VREF,      "VREF",         FMT_NONE        },
{       BC_VSET,      "VSET",         FMT_NONE        },
{       BC_NEG,       "NEG",          FMT_NONE        },
{       BC_INC,       "INC",          FMT_NONE        },
{       BC_DEC,       "DEC",          FMT_NONE        },
{       BC_DUP2,      "DUP2",         FMT_NONE        },
{       BC_DROP,      "DROP",         FMT_NONE        },
{       BC_DUP,       "DUP",          FMT_NONE        },
{       BC_OVER,      "OVER",         FMT_NONE        },
{       BC_NEWOBJECT, "NEWOBJECT",    FMT_BYTE        },
{       BC_NEWVECTOR, "NEWVECTOR",    FMT_NONE        },
{       BC_AFRAME,    "AFRAME",       FMT_2BYTE       },
{       BC_AFRAMER,   "AFRAMER",      FMT_2BYTE       },
{       BC_CLOSE,     "CLOSE",        FMT_BYTE        },
{       BC_SWITCH,    "SWITCH",       FMT_SWITCH      },
{       BC_ARGSGE,    "ARGSGE",       FMT_BYTE        },
{       BC_PUSHSCOPE, "PUSHSCOPE",    FMT_NONE        },
{       BC_THROW,     "THROW",        FMT_NONE        },
{       BC_UNDEFINED, "UNDEFINED",    FMT_NONE        },
{       BC_INSTANCEOF,"INSTANCEOF",   FMT_NONE        },
{       BC_TYPEOF,    "TYPEOF",       FMT_NONE        },
{       BC_EH_PUSH,   "EH_PUSH",      FMT_WORD        },
{       BC_EH_POP,    "EH_POP",       FMT_WORD        },
{       BC_IN,        "IN",           FMT_NONE        },
{       BC_NEXT,      "NEXT",         FMT_NONE        },
{       BC_NOTHING,   "NOTHING",      FMT_NONE        },
{       BC_BRDEF,     "BRDEF",        FMT_WORD        },
{       BC_OUTPUT,    "OUTPUT",       FMT_NONE        },
{       BC_EQ_STRONG, "EQ_STRONG",    FMT_NONE        },
{       BC_NE_STRONG, "NE_STRONG",    FMT_NONE        },
{       BC_GETRANGE,  "GETRANGE",     FMT_NONE        },
{       BC_F,         "F",            FMT_NONE        },
{       BC_SETP,      "SETPM",        FMT_NONE        },
{       BC_GSETC,     "GSETC",        FMT_LIT         },
{       BC_PUSH_NS,   "PUSH_NS",      FMT_NONE        },
{       BC_POP_NS,    "POP_NS",       FMT_NONE        },
{       BC_PROTO,     "PROTO",        FMT_NONE        },
{       BC_BRUNDEF,   "BRUNDEF",      FMT_WORD        },
{       BC_INCLUDE,   "INCLUDE",      FMT_NONE        },
{       BC_LIKE,      "LIKE",         FMT_NONE        },
{       BC_DEBUG,     "DEBUG",        FMT_NONE        },

{       BC_S_CALL,    "BC_S_CALL",    FMT_WORD        },
{       BC_S_RETURN,  "BC_S_RETURN",  FMT_NONE        },
{       BC_YIELD,     "YIELD",        FMT_WORD        },
{       BC_NEWCLASS,  "NEWCLASS",     FMT_NONE        },
{       BC_USHL,       "USHL",        FMT_NONE        },
{       BC_USHR,       "USHR",        FMT_NONE        },
{       BC_NS,         "NS",          FMT_NONE        },

{       BC_CAR,       "CAR",          FMT_NONE        },
{       BC_CDR,       "CDR",          FMT_NONE        },
{       BC_RCAR,      "RCAR",         FMT_NONE        },
{       BC_RCDR,      "RCDR",         FMT_NONE        },
{       BC_GSETNS,    "GSETNS",       FMT_LIT         },

{       BC_ROTATE,    "ROTATE",       FMT_BYTE        },

{       BC_INCLUDE_LIBRARY, "INCLUDE LIBRARY", FMT_NONE  }, 

{0,0,0}
};

/* CsDecodeProcedure - decode the instructions in a code obj */
void CsDecodeProcedure(VM *c,value method,stream *s)
{
    value code = CsMethodCode(method);
    int len,lc,n;
    len = (int)CsByteVectorSize(CsCompiledCodeBytecodes(code));
    for (lc = 0; lc < len; lc += n)
        n = CsDecodeInstruction(c,code,lc,s);
}

/* CsDecodeInstruction - decode a single bytecode instruction */
int CsDecodeInstruction(VM *c,value code,int lc,stream *s)
{
    char buf[100];
    value name;
    byte *cp;
    int i,cnt,n=1;
    OTDEF *op;

    /* get bytecode pointer for this instruction and the method name */
    cp = CsByteVectorAddress(CsCompiledCodeBytecodes(code)) + lc;
    name = CsCompiledCodeName(code);
    
    /* show the address and opcode */
    if (CsStringP(name)) {
        //char *data = CsStringAddress(name);
        //long size = CsStringSize(name);
        //if (size > 32) size = 32;
        //strncpy(buf,(char *)data,(size_t)size);
        //sprintf(&buf[size],":%04x %02x ",lc,*cp);
        s->printf(L"%S:%04x %02x",CsStringAddress(name),lc,*cp);
    }
    else
        s->printf(L"%08lx:%04x %02x ",(long)code,lc,*cp);
    //s->put_str(buf);

    /* display the operands */
    for (op = otab; op->ot_name; ++op)
        if (*cp == op->ot_code) {
            switch (op->ot_fmt) {
            case FMT_NONE:
                sprintf(buf,"      %s\n",op->ot_name);
                s->put_str(buf);
                break;
            case FMT_BYTE:
                sprintf(buf,"%02x    %s %02x\n",cp[1],op->ot_name,cp[1]);
                s->put_str(buf);
                n += 1;
                break;
            case FMT_2BYTE:
                sprintf(buf,"%02x %02x %s %02x %02x\n",cp[1],cp[2],
                        op->ot_name,cp[1],cp[2]);
                s->put_str(buf);
                n += 2;
                break;
            case FMT_WORD:
                sprintf(buf,"%02x %02x %s %02x%02x\n",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                s->put_str(buf);
                n += 2;
                break;
            case FMT_LIT:
                sprintf(buf,"%02x %02x %s %02x%02x ; ",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                s->put_str(buf);
                CsPrint(c,CsCompiledCodeLiteral(code,(cp[2] << 8) | cp[1]),s);
                s->put('\n');
                n += 2;
                break;
            case FMT_SWITCH:
                sprintf(buf,"%02x %02x %s %02x%02x\n",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                s->put_str(buf);
                cnt = cp[2] << 8 | cp[1];
                n += 2 + cnt * 4 + 2;
                i = 3;
                while (--cnt >= 0) {
                    sprintf(buf,"                 %02x%02x %02x%02x ; ",cp[i+1],cp[i],cp[i+3],cp[i+2]);
                    s->put_str(buf);
                    CsPrint(c,CsCompiledCodeLiteral(code,(cp[i+1] << 8) | cp[i]),s);
                    s->put('\n');
                    i += 4;
                }
                sprintf(buf,"                 %02x%02x\n",cp[i+1],cp[i]);
                s->put_str(buf);
                break;
            }
            return n;
        }
    
    /* unknown opcode */
    sprintf(buf,"      <UNKNOWN>\n");
    s->put_str(buf);
    return 1;
}

}
