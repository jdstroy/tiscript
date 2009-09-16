/* tiscript.cpp - the main routine */
/*
        Copyright (c) 2005, by Andrew Fedoniouk, Terra Informatica
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "cs.h"
#include "cs_com.h"


// cheap conversions for console (OEM encoding?)
wchar to_wchar(char c)
{
  wchar wc = '?';
#ifdef WIN32
  MultiByteToWideChar(CP_OEMCP,0,&c,1,&wc,1);
#else
  mbtowc(&wc, &c, 1 );
#endif
  return wc;
}

int to_char(wchar wc, char* pc10)
{
#ifdef WIN32
  return WideCharToMultiByte(CP_OEMCP,0,&wc,1,pc10,10,0,0);
#else
  return wctomb(pc10,wc);
#endif
}

struct console_stream: public tis::stream
{
  virtual bool is_output_stream() const { return true; }
  virtual int  get() { int c = getchar(); return c != EOF? to_wchar(c) : int(EOS); }
  virtual bool put(int ch)
  {
    char bf[MB_LEN_MAX];
    int n = to_char(ch,bf);
    for( int i = 0; i < n; ++i ) putchar(bf[i]);
    return true;
  }
};

console_stream console;

/* prototypes */
static void ErrorHandler(tis::VM *c,int code, char* message, char *stackTrace);
static void CompileFile(tis::VM *c,char *inputName,char *outputName, int forceType);

enum FILE_TYPE
{
  CLIENT_SCRIPT = 1,
  SERVER_SCRIPT,
  BYTECODE,
};

static void LoadFile(tis::VM *c,char *name, int type );
static void ReadEvalPrint(tis::VM *c);
static void Usage(void);

static int sourceType(const char* t)
{
  if(stricmp(t, "c") == 0)
    return CLIENT_SCRIPT;
  if(stricmp(t, "client") == 0)
    return CLIENT_SCRIPT;
  if(stricmp(t, "s") == 0)
    return SERVER_SCRIPT;
  if(stricmp(t, "server") == 0)
    return SERVER_SCRIPT;
  if(stricmp(t, "b") == 0)
    return BYTECODE;
  if(stricmp(t, "bc") == 0)
    return BYTECODE;
  if(stricmp(t, "bytecode") == 0)
    return BYTECODE;
  return 0;
}


// main - the main routine
int main(int argc,char **argv)
{
    bool interactiveP = true;
    int  forceType = 0;
   
    tis::VM vm;
           
    // setup standard i/o on console
    vm.standardInput = &console;
    vm.standardOutput = &console;
    vm.standardError = &console;
  
    try
    {
        char *inputName = 0,*outputName = 0;
        //bool verboseP = false;
        int i;

        /* process arguments */
        for (i = 1; i < argc; ++i) {
          if (argv[i][0] == '-')
          {
                switch (argv[i][1])
                {
                case 'h':
                    Usage();
                    break;
                case 'c':   /* compile source file */
                    if (argv[i][2])
                        inputName = &argv[i][2];
                    else if (++i < argc)
                        inputName = argv[i];
                    else
                        Usage();
                    interactiveP = false;
                    CompileFile(&vm,inputName,outputName, forceType);
                    outputName = NULL;
                    break;
                case 'g':   /* emit debugging information when compiling */
                    vm.compiler->emitLineNumbersP = true;
                    break;
                case 'i':   /* enter interactive mode after loading */
                    interactiveP = true;
                    break;
                case 'o':   /* specify output filename when compiling */
                    if (argv[i][2])
                        outputName = &argv[i][2];
                    else if (++i < argc)
                        outputName = argv[i];
                    else
                        Usage();
                    interactiveP = false;
                    break;
                case 't':   /* display values of expressions loaded */
                    //verboseP = true;
                    if (argv[i][2])
                        forceType = sourceType(&argv[i][2]);
                    else if (++i < argc)
                        forceType = sourceType(argv[i]);
                    else
                        Usage();
                    break;
                default:
                    Usage();
                    break;
                }
            }
            else {
                interactiveP = false;
                LoadFile(&vm,argv[i],forceType);
                
            }
        }
    }
    catch(tis::error_event& e) // uncaught error
    {
      e;
      tis::CsDisplay(&vm,vm.val[0],vm.standardError);
    }

    /* read/eval/print loop */
    if (interactiveP)
    {
        ReadEvalPrint(&vm);
    }
    /* return successfully */

//FINISH:

    return 0;
}

/* CompileFile - compile a single file */
static void CompileFile(tis::VM *c,char *inputName,char *outputName, int forceType)
{
    char oname[1024],*p;
    char *ext;

    /* determine the input filename */
    //if ((p = strrchr(inputName,'.')) == NULL) {
    //    strcpy(iname,inputName);
    //    strcat(iname,".js");
    //    inputName = iname;
    //}

    int type = 0;

    if ((ext = strrchr(inputName,'.')) != NULL)
    {
      if (stricmp(ext,".js") == 0)
          type = CLIENT_SCRIPT;
      else if (stricmp(ext,".jsp") == 0)
          type = SERVER_SCRIPT;
    }

    if( type == 0 && forceType == 0 )
    {
        fprintf(stderr,"Unknown file type '%s'\n",inputName);
        exit(1);
    }

    if( forceType )
      type = forceType;


    /* determine the output filename */
    if (outputName) {
        if ((p = strrchr(outputName,'.')) == NULL) {
            strcpy(oname,outputName);
            strcat(oname,".jsb");
            outputName = oname;
        }
    }
    /* construct an output filename */
    else {
        if ((p = strrchr(inputName,'.')) == NULL)
            strcpy(oname,inputName);
        else {
            int len = p - inputName;
            strncpy(oname,inputName,len);
            oname[len] = '\0';
        }
        strcat(oname,".jsb");
        outputName = oname;
    }

    /* compile the file */
    printf("Compiling '%s' -> '%s'\n",inputName,outputName);
    tis::CsCompileFile(tis::CsCurrentScope(c),tool::ustring(inputName),tool::ustring(outputName), type == SERVER_SCRIPT);
}

/* LoadFile - load a single file */
static void LoadFile(tis::VM *c,char *name, int forceType)
{
    //tis::stream *s = verboseP ? c->standardOutput : NULL;
    tis::stream *s = c->standardOutput;
    const char *ext;

    int type = 0;

    if ((ext = strrchr(name,'.')) != NULL)
    {
      if (stricmp(ext,".js") == 0)
          type = CLIENT_SCRIPT;
      else if (stricmp(ext,".tis") == 0)
          type = CLIENT_SCRIPT;
      else if (stricmp(ext,".jsp") == 0)
          type = SERVER_SCRIPT;
      else if (stricmp(ext,".jsb") == 0)
          type = BYTECODE;
    }

    if( type == 0 && forceType == 0 )
    {
        fprintf(stderr,"Unknown file type '%s'\n",name);
        exit(1);
    }

    if( forceType )
      type = forceType;

    switch( type )
    {
      default:
      case CLIENT_SCRIPT:
        tis::CsLoadFile(tis::CsGlobalScope(c),tool::ustring(name),0);
        break;
      case SERVER_SCRIPT:
        tis::CsLoadFile(tis::CsGlobalScope(c),tool::ustring(name),c->standardOutput);
        break;
      case BYTECODE:
        tis::CsLoadObjectFile(tis::CsGlobalScope(c),tool::ustring(name));
        break;
    }

}

/* ReadEvalPrint - enter a read/eval/print loop */
static void ReadEvalPrint(tis::VM *c)
{
    char lineBuffer[256];
    tis::value val;

    /* protect a pointer from the garbage collector */
//    tis::CsProtectPointer(c,&val);

    for (;;)
    {
      TRY
      {
        printf("\nExpr> "); fflush(stdout);
        if (fgets(lineBuffer,sizeof(lineBuffer),stdin))
        {
            tool::ustring line(lineBuffer);
            val = tis::CsEvalString(tis::CsCurrentScope(c),NULL_VALUE, line, line.length());
            if (val) {
                printf("Value: ");
                tis::CsPrint(c,val,c->standardOutput);
            }
        }
        else
            break; // done
      }
      catch(tis::error_event& e) // uncaught error
      {
        e;
        tis::CsDisplay(c,c->val[0],c->standardError);
      }
    }

//    tis::CsUnprotectPointer(c,&val);
}


/* Usage - display a usage message and exit */
static void Usage(void)
{
    fprintf(stderr,"\
usage: tiscript.exe \n\
       [-c file]     compile a source file\n\
       [-g]          include debugging information\n\
       [-i]          enter interactive mode after loading\n\
       [-o file]     bytecode output file name for compilation\n\
       [-t typename] force type of the file (ignore extensions).\n\
                     where typename is one of:\n\
                     c[lient] - client script, \n\
                     s[erver] - server script - interprets <% %>\n\
                                script inclusions, \n\
                     b[ytecode] - load as compiled bytecode file.\n\
       [-h]          display (this) help information\n\
       [file]        load a source or bytecode file, \n\
                     supported file extensions: \n\
                       js - client script (ECMAScript),\n\
                       jsp - server script (ECMAScript),\n\
                       jsb - compiled script (bytecodes).\n\n\
See: http://terrainformatica.com/tiscript for more details.\n\n"
       );

    exit(1);
}

