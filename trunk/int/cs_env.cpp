/* env.c - environment types */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis 
{

/* ENVIRONMENT */

/* Environment pdispatch */
dispatch CsEnvironmentDispatch = {
    "Environment",
    &CsEnvironmentDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    CsBasicVectorSizeHandler,
    CsDefaultCopy,
    CsBasicVectorScanHandler,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem

};

/* CsMakeEnvironment - make an environment value */
value CsMakeEnvironment(VM *c,long size)
{
    return CsMakeBasicVector(c,&CsEnvironmentDispatch,size);
}

/* STACK ENVIRONMENT */

/* StackEnvironment handlers */
static value StackEnvironmentCopy(VM *c,value obj);

/* StackEnvironment pdispatch */
dispatch CsStackEnvironmentDispatch = {
    "StackEnvironment",
    &CsEnvironmentDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    CsBasicVectorSizeHandler,
    StackEnvironmentCopy,
    CsBasicVectorScanHandler,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* StackEnvironmentCopy - StackEnvironment copy handler */
static value StackEnvironmentCopy(VM *c,value obj)
{
    /* copying the stack will copy the elements */
    return obj;
}

/* MOVED ENVIRONMENT */

/* MovedEnvironment handlers */
static value MovedEnvironmentCopy(VM *c,value obj);

/* MovedEnvironment pdispatch */
dispatch CsMovedEnvironmentDispatch = {
    "MovedEnvironment",
    &CsEnvironmentDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    CsBasicVectorSizeHandler,
    MovedEnvironmentCopy,
    CsBasicVectorScanHandler,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* MovedEnvironmentCopy - MovedEnvironment copy handler */
static value MovedEnvironmentCopy(VM *c,value obj)
{
    return CsCopyValue(c,CsMovedEnvForwardingAddr(obj));
}

}
