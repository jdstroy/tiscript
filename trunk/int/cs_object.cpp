/* object.c - 'Object' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include "cs.h"

namespace tis
{


/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_prototype(VM *c);
static value CSF_clone(VM *c);
static value CSF_exists(VM *c);
static value CSF_remove(VM *c);
static value CSF_call(VM *c);
static value CSF_show(VM *c);
static value CSF_store(VM *c);
static value CSF_restore(VM *c);
       value CSF_propertyAt(VM *c);

static value CSF_length(VM *c,value obj);
static value CSF_persistent(VM *c,value obj);
static value CSF_prototype(VM *c,value obj);
static void CSF_set_prototype(VM *c,value obj, value pro);

static value CSF_class_name(VM *c,value obj);

static value CSF_class_class_name(VM *c,value obj);
static void CSF_set_class_class_name(VM *c,value obj, value pro);

#define FETCH(c,obj) if( _CsIsPersistent(obj) ) obj = CsFetchObjectData(c, obj);
#define FETCH_P(c,obj,p1) if( _CsIsPersistent(obj) ) { CsPush(c,p1); obj = CsFetchObjectData(c, obj); p1 = CsPop(c); }
#define FETCH_PP(c,obj,p1,p2) if( _CsIsPersistent(obj) ) { CsPush(c,p2); CsPush(c,p1); obj = CsFetchObjectData(c, obj); p1 = CsPop(c); p2 = CsPop(c); }


/* Object methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor      ),
C_METHOD_ENTRY( "toLocaleString",   CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "toString",         CSF_std_toString        ),
C_METHOD_ENTRY( "valueOf",          CSF_std_valueOf         ),

C_METHOD_ENTRY( "clone",            CSF_clone           ),
C_METHOD_ENTRY( "exists",           CSF_exists          ),
C_METHOD_ENTRY( "remove",           CSF_remove          ),
C_METHOD_ENTRY( "call",             CSF_call            ),
C_METHOD_ENTRY( "show",             CSF_show            ),
C_METHOD_ENTRY( "store",            CSF_store           ),
C_METHOD_ENTRY( "restore",          CSF_restore         ),
C_METHOD_ENTRY( "eval",             CSF_eval            ),
C_METHOD_ENTRY( "propertyAt",       CSF_propertyAt      ),
C_METHOD_ENTRY( 0,                  0                   )
};

/* Object properties */
static vp_method properties[] = {
//VP_METHOD_ENTRY( "class",    CSF_prototype, CSF_set_prototype ),
VP_METHOD_ENTRY( "length",      CSF_length, 0 ),
VP_METHOD_ENTRY( "className",   CSF_class_name, 0 ),
VP_METHOD_ENTRY( "persistent",  CSF_persistent, 0 ),

VP_METHOD_ENTRY( 0,          0,         0         )
};


/* Object properties */
static vp_method class_properties[] = {
//VP_METHOD_ENTRY( "class",    CSF_prototype, CSF_set_prototype ),
VP_METHOD_ENTRY( "length",      CSF_length, 0 ),
VP_METHOD_ENTRY( "className",   CSF_class_class_name, CSF_set_class_class_name ),
VP_METHOD_ENTRY( 0,          0,         0         )
};


/* CsInitObject - initialize the 'Object' obj */
void CsInitObject(VM *c)
{
    /* make the base of the obj inheritance tree */
    c->objectObject = CsEnterObject(CsGlobalScope(c),"Object",UNDEFINED_VALUE,methods, properties);
    c->classObject = CsEnterObject(CsGlobalScope(c),"Class",UNDEFINED_VALUE,methods, class_properties);
}

/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    CsCheckArgCnt(c,2);
    CsCheckType(c,1,CsObjectP);
    return CsGetArg(c,1);
}

/* CSF_Class - built-in function 'Class' */
/* CSF_size - built-in property 'length' */
static value CSF_length(VM *c,value obj)
{
    FETCH( c, obj );
    return CsMakeInteger(CsObjectPropertyCount(obj));
}

static value CSF_persistent(VM *c,value obj)
{
    return CsIsPersistent(c,obj)? TRUE_VALUE:FALSE_VALUE;
}

static value CSF_prototype(VM *c,value obj)
{
    FETCH( c, obj );
    return CsObjectClass(obj);
}

static void CSF_set_prototype(VM *c,value obj, value pro)
{
    FETCH_P( c, obj, pro );
    if( CsObjectOrMethodP(pro) )
      CsSetObjectClass(obj, pro);
    else
      CsTypeError(c, pro);
}

static value CSF_class_name(VM *c,value obj)
{
    value cls = CsObjectClass(obj);
    if( CsClassP(cls) ) 
      return CsClassName(cls);
    return cls;
}

static value CSF_class_class_name(VM *c,value obj)
{
    if( CsClassP(obj) ) 
      return CsClassName(obj);
    else
    {
      value cls = CsObjectClass(obj);
      if( CsClassP(cls) ) 
        return CsClassName(cls);
      return cls;
    }
}
static void CSF_set_class_class_name(VM *c,value obj, value v)
{
    if( CsClassP(obj) && CsStringP(v)) 
    {
      value n = CsClassName(obj);
      if( n == UNDEFINED_VALUE )
        CsSetClassName(obj,v);
    }
}


/* CSF_clone - built-in method 'Clone' */
static value CSF_clone(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsObjectDispatch);
    FETCH( c, obj );
    return CsCloneObject(c,obj);
}

/* CSF_Exists - built-in method 'exists' */
static value CSF_exists(VM *c)
{
    value obj,tag;
    bool deep = false;
    CsParseArguments(c,"V=*V|B",&obj,&CsObjectDispatch,&tag,&deep);
    FETCH_P( c, obj,tag );
    while (CsObjectP(obj)) {
        if (CsFindProperty(c,obj,tag,NULL,NULL))
            return TRUE_VALUE;
        if( !deep ) break;
        obj = CsObjectClass(obj);
    }
    return FALSE_VALUE;
}

/* CSF_propertyAt - built-in method 'val = Object.propertyAt(sym)' 
   It is a direct equivalent of val = Object.sym;  
*/
value CSF_propertyAt(VM *c)
{
    value obj,tag, val;
    CsParseArguments(c,"V*V",&obj,&tag);
    FETCH_P( c, obj, tag );

    if(CsGetProperty(c,obj,tag,&val))
      return val;

    return UNDEFINED_VALUE;
}

/* CSF_remove - built-in method 'Remove' */
static value CSF_remove(VM *c)
{
    value obj,tag;
    CsParseArguments(c,"V=*V",&obj,&CsObjectDispatch,&tag);
    FETCH_P( c, obj,tag );
    CsRemoveObjectProperty(c,obj,tag);
    return UNDEFINED_VALUE;
}



/* CSF_call - built-in method 'call'
   calls method in context of 'this' */
value CSF_call(VM *c)
{
    int_t i,vcnt,argc;
    value argv;

    if(CsArgCnt(c) == 3)
    {
      CsCheckType(c,1,CsObjectOrMethodP);
      CsCheckType(c,3,CsMethodP);
      CsCheck(c,3);
      CsPush(c,CsGetArg(c,1));
      CsPush(c,CsGetArg(c,3));
      CsPush(c,CsGetArg(c,1));
      return CsInternalSend(c,2);
    }
    CsCheckArgMin(c,4);
    CsCheckType(c,1,CsObjectOrMethodP);
    CsCheckType(c,CsArgCnt(c),CsVectorP);
    argv = CsGetArg(c,CsArgCnt(c));
    if (CsMovedVectorP(argv))
        argv = CsVectorForwardingAddr(argv);
    vcnt = CsVectorSizeI(argv);
    argc = CsArgCnt(c) + vcnt - 2;
    CsCheck(c,argc + 1);
    CsPush(c,CsGetArg(c,1));
    CsPush(c,CsGetArg(c,3));
    CsPush(c,CsGetArg(c,1));
    for (i = 4; i < CsArgCnt(c); ++i)
        CsPush(c,CsGetArg(c,i));
    for (i = 0; i < vcnt; ++i)
        CsPush(c,CsVectorElementI(argv,i));
    return CsInternalSend(c,argc);
}

/* replaced by generator each_property

void CsScanObject( VM *c, value obj, object_scanner& osc )
{
  FETCH(c,obj);
  CsScanObjectNoLoad( c, obj, osc );
}

void CsScanObjectNoLoad( VM *c, value obj, object_scanner& osc )
{
    value props;
    props = CsObjectProperties(obj);
    if (!CsObjectPropertyCount(obj))
      return;

    if (CsHashTableP(props)) {
        int_t cnt = CsHashTableSize(props);
        int_t i;
        for (i = 0; i < cnt; ++i) {
            value prop = CsHashTableElement(props,i);
            for (; prop != UNDEFINED_VALUE; prop = CsPropertyNext(prop))
            {
              if(!osc.item(c,CsPropertyTag(prop),CsPropertyValue(prop)))
                 return;
            }
        }
    }
    else
        for (; props != UNDEFINED_VALUE; props = CsPropertyNext(props))
        {
            if(!osc.item(c,CsPropertyTag(props),CsPropertyValue(props)))
              return;
        }
}*/


/* CSF_show - built-in method 'Show' */
static value CSF_show(VM *c)
{
    stream *s = c->standardOutput;
    value obj,props;
    CsParseArguments(c,"V=*|P=",&obj,&CsObjectDispatch,&s,c->fileDispatch);
    FETCH( c, obj );
    props = CsObjectProperties(obj);
    s->put_str("Class: ");
    CsPrint(c,CsObjectClass(obj),s);
    s->put('\n');
    if (CsObjectPropertyCount(obj)) {
        s->put_str("Properties:\n");
        if (CsHashTableP(props)) {
            int_t cnt = CsHashTableSize(props);
            int_t i;
            for (i = 0; i < cnt; ++i) {
                value prop = CsHashTableElement(props,i);
                for (; prop != UNDEFINED_VALUE; prop = CsPropertyNext(prop)) {
                    s->put_str("  ");
                    CsPrint(c,CsPropertyTag(prop),s);
                    s->put_str(": ");
                    CsPrint(c,CsPropertyValue(prop),s);
                    s->put('\n');
                }
            }
        }
        else {
            for (; props != UNDEFINED_VALUE; props = CsPropertyNext(props)) {
                s->put_str("  ");
                CsPrint(c,CsPropertyTag(props),s);
                s->put_str(": ");
                CsPrint(c,CsPropertyValue(props),s);
                s->put('\n');
            }
        }
    }
    return obj;
}

/* CSF_store - built-in method 'store' for persistent objects */
value CSF_store(VM *c)
{
  value obj;
  CsParseArguments( c, "V=*", &obj, &CsObjectDispatch );

  if( !CsIsPersistent(c,obj) )
   return FALSE_VALUE;
  if( !CsIsModified( obj ) )
   return TRUE_VALUE;

  CsStoreObjectData(c, obj);
  return TRUE_VALUE;
}

/* CSF_rollback - built-in method 'Rollback' for persistent objects */
/* usage: obj.restore(); */
value CSF_restore(VM *c)
{
  value obj;
  CsParseArguments( c, "V=*", &obj, &CsObjectDispatch );

  if( !CsIsPersistent(c,obj) )
  { return FALSE_VALUE; }

  return CsRestoreObj(c, obj);
}


/* OBJECT */

inline void IncObjectPropertyCount(value o)  { ptr<object>(o)->propertyCount += 1; }

/* Object handlers */
//static bool CsGetObjectProperty(VM *c,value obj,value tag,value *pValue);
//static bool CsSetObjectProperty(VM *c,value obj,value tag,value value);
static value CopyPropertyTable(VM *c,value table);
static value CopyPropertyList(VM *c,value plist);
static void CreateHashTable(VM *c,value obj,value p);
static int ExpandHashTable(VM *c,value obj,int_t i);
static bool AddObjectConstant(VM *c,value obj,value tag,value val);
static bool AddClassConstant(VM *c,value obj,value tag,value val);


/* Object pdispatch */
dispatch CsObjectDispatch = {
    "Object",
    &CsObjectDispatch,
    CsGetObjectProperty,
    CsSetObjectProperty,
    CsMakeObject,
    CsDefaultPrint,
    CsObjectSize,
    CsDefaultCopy,
    CsObjectScan,
    CsDefaultHash,
    CsObjectGetItem,
    CsObjectSetItem,
    CsObjectNextElement,
    AddObjectConstant,
    CsRemoveObjectProperty,
};

static void CsClassScan(VM *c,value obj);
static long CsClassSize(value obj);
static bool CsSetClassProperty(VM *c,value obj,value tag,value val);
static void CsClassSetItem(VM *c,value obj,value tag,value val);

/* Class pdispatch */
dispatch CsClassDispatch = {
    "Class",
    &CsObjectDispatch,
    CsGetObjectProperty,
    CsSetClassProperty,
    CsMakeObject,
    CsDefaultPrint,
    CsClassSize,
    CsDefaultCopy,
    CsClassScan,
    CsDefaultHash,
    CsObjectGetItem,
    CsClassSetItem,
    CsObjectNextElement,
    AddClassConstant,
};

/* Namespace pdispatch */
dispatch CsNamespaceDispatch = {
    "Namespace",
    &CsObjectDispatch,
    CsGetObjectProperty,
    CsSetClassProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    CsClassSize,
    CsDefaultCopy,
    CsClassScan,
    CsDefaultHash,
    CsObjectGetItem,
    CsClassSetItem,
    CsObjectNextElement,
    AddClassConstant,
};


/* CsSetCObjectProperty - CObject set property handler */
static bool AddClassConstant(VM *c,value obj,value tag,value val)
{
    if( tag == UNDEFINED_VALUE ) // special treatment for property undefined(n,v) handler
    {
      if(CsPropertyMethodP(val))
      {
        CsSetClassUndefinedPropHandler(obj,val);
        return true;
      }
    }
    return AddObjectConstant(c,obj,tag,val);
}

static bool CsSetClassProperty(VM *c,value obj,value tag,value val)
{
   if( tag == UNDEFINED_VALUE ) // special treatment for property undefined(n,v) handler
   {
      if(CsPropertyMethodP(val))
      {
        CsSetClassUndefinedPropHandler(obj,val);
        return true;
      }
   }
   return CsSetObjectProperty(c,obj,tag,val);
}

static void CsClassSetItem(VM *c,value obj,value tag,value val)
{
  if( tag == UNDEFINED_VALUE ) // special treatment for property undefined(n,v) handler
  {
    if(CsPropertyMethodP(val))
    {
      CsSetClassUndefinedPropHandler(obj,val);
      return;
    }
  }
  CsObjectSetItem(c,obj,tag,val);
}

value CsObjectGetItem(VM *c,value obj,value tag)
{
    FETCH_P(c,obj,tag);
    value p;
    if( tag == PROTOTYPE_SYM )
    {
      return CsObjectClass(obj);
    }
    if( p = CsFindProperty(c,obj,tag,NULL,NULL))
    {
      value propValue = CsPropertyValue(p);
      return propValue;
    }
    return UNDEFINED_VALUE;
}

void CsObjectSetItemNoLoad(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) && val != UNDEFINED_VALUE)
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
    }
    else if (!(p = CsFindProperty(c,obj,tag,&hashValue,&i)))
      CsAddProperty(c,obj,tag,val,hashValue,i);
    else
    {
      value t = CsPropertyValue(p);
      if(CsPropertyMethodP(t))
        CsSendMessage(c,obj,t,1, val );
      else
        CsSetPropertyValue(p,val);
    }
}

void     CsObjectSetItem(VM *c,value obj,value tag,value val)
{
   FETCH_PP(c,obj,tag,val);
   CsSetModified(obj, true);
   CsObjectSetItemNoLoad(c,obj, tag,val);
  //CsSetObjectProperty(c,obj,tag,value);
}

/* CsGetProperty - recursively search for a property value */
bool CsGetProperty(VM *c,value obj,value tag,value *pValue)
{
    return CsGetProperty1(c,obj,tag,pValue);
}

/* CsGetObjectProperty - Object get property handler */
bool CsGetObjectProperty(VM *c,value& obj,value tag,value *pValue)
{
    FETCH_P(c,obj,tag);
    value p;

    if( CsSymbolP(tag) && tag == PROTOTYPE_SYM )
    {
      *pValue = CsObjectClass(obj);
      return true;
    }

    value self = obj;

    if( p = CsFindProperty(c,obj,tag,NULL,NULL))
    {
      value propValue = CsPropertyValue(p);
      if (CsVPMethodP(propValue))
      {
        vp_method *method = ptr<vp_method>(propValue);
        if (method->get(c,self,*pValue))
          return true;
        else
          CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
      }
      else if(CsPropertyMethodP(propValue))
        *pValue = CsSendMessage(c,self,propValue,1, NOTHING_VALUE);
      else
        *pValue = propValue;
      return true;
    }

    value uph = UNDEFINED_VALUE; // undefined property handler

    while ( ((obj = CsObjectClass(obj)) != 0) && CsObjectOrMethodP(obj) )
    {
      if( p = CsFindProperty(c,obj,tag,NULL,NULL))
      {
        value propValue = CsPropertyValue(p);
        if (CsVPMethodP(propValue))
        {
          vp_method *method = ptr<vp_method>(propValue);
          if (method->get(c,self,*pValue))
            return true;
          else
            CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
        }
        else if(CsPropertyMethodP(propValue))
          *pValue = CsSendMessage(c,self,propValue,1, NOTHING_VALUE);
        else
          *pValue = propValue;
        return true;
      }
      if( CsClassP(obj))
      {
         if(uph == UNDEFINED_VALUE)
         uph = CsClassUndefinedPropHandler(obj);
    }
    }

    if( uph != UNDEFINED_VALUE && CsPropertyMethodP(uph))
    {
      value v = CsSendMessage(c,self,uph, 2, tag, NOTHING_VALUE );
      if(v != NOTHING_VALUE )
      {
        *pValue = v;
        return true;
      }
    }
    return false;
}


bool CsSetObjectPropertyNoLoad(VM *c,value obj,value tag,value val)
{
    int_t hashValue = 0,i = 0;
    value p;

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) && val != UNDEFINED_VALUE)
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
      return true;
    }

#ifdef _DEBUG
    if(CsStringP(tag) && CsStringChars(tag) == WCHARS("101"))
        tag = tag;
    //dispatch * pd = CsGetDispatch(tag);
    //tool::string name = CsSymbolName(tag);
#endif

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      value propValue = CsPropertyValue(p);

      if(CsPropertyMethodP(propValue))
        CsSendMessage(c,obj,propValue,1, val );
      else if( CsPropertyIsConst(p) )
        CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
      else
        CsSetPropertyValue(p,val);
      return true;
    }

    value self = obj;
    value uph  = UNDEFINED_VALUE;

    while ( ((obj = CsObjectClass(obj)) != 0) && CsObjectOrMethodP(obj))
    {
      if( p = CsFindProperty(c,obj,tag,0,0))
      {
        value propValue = CsPropertyValue(p);
        if(CsPropertyMethodP(propValue))
        {
          CsSendMessage(c,self,propValue,1, val );
          return true;
        }
        else if (CsVPMethodP(propValue))
        {
          vp_method *method = ptr<vp_method>(propValue);
          if (method->set(c,obj,val))
            return true;
          else
            CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
        }
        else if( CsPropertyIsConst(p) )
          CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
        break;
      }
      if( CsClassP(obj) && uph == UNDEFINED_VALUE)
         uph = CsClassUndefinedPropHandler(obj);
    }

    if( CsPropertyMethodP(uph))
    {
      if(NOTHING_VALUE != CsSendMessage(c,self,uph, 2, tag, val ))
         return true;
    }

    CsAddProperty(c,self,tag,val,hashValue,i);
    return true;
}

bool CsSetObjectPersistentProperty(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) && val != UNDEFINED_VALUE )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
      return true;
    }

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      //value propValue = CsPropertyValue(p);
      CsSetPropertyValue(p,val);
      return true;
    }

    value self = obj;

    while ( ((obj = CsObjectClass(obj)) != 0) && CsObjectOrMethodP(obj))
    {
      if( p = CsFindProperty(c,obj,tag,0,0))
      {
        value propValue = CsPropertyValue(p);
        if(CsPropertyMethodP(propValue))
        {
          //CsSendMessage(c,self,propValue,1, val );
          return true;
        }
        else if (CsVPMethodP(propValue))
        {
          //vp_method *method = ptr<vp_method>(propValue);
          //if (method->set(c,obj,val))
          return true;
          //else
          //  CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
        }
        else if( CsPropertyIsConst(p) )
          //CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
          return true;
        break;
      }
    }
    CsAddProperty(c,self,tag,val,hashValue,i);
    return true;
}

/* CsSetObjectProperty - Object set property handler */
bool CsSetObjectProperty(VM *c,value obj,value tag,value val)
{
    FETCH_PP(c,obj,tag,val);
    CsSetModified(obj, true);
    return CsSetObjectPropertyNoLoad(c,obj,tag,val);
}

const char* CsClassClassName(VM* c, value cls)
{
  if( CsClassP(cls) )
  {
    value n = CsClassName(cls);
    if( CsSymbolP(n))
      return CsSymbolPrintName(n);
  }

  /*static value classNameTag = 0;
  if(!classNameTag)
    classNameTag = symbol_value(".className");

  value className = 0;
  if(CsGetProperty1(c,cls, classNameTag, &className))
  {
    if( CsSymbolP(className))
      return CsSymbolPrintName(className);

    else
      dbg_printf("", CsTypeName(className));
  }
  */

  return "";
}

const char* CsObjectClassName(VM* c, value obj)
{
  value cls = CsObjectClass(obj);
  if(cls == UNDEFINED_VALUE)
    return "";
  return CsClassClassName(c,cls);
}


bool CsSetProperty1(VM *c, value obj,value tag,value val )
{
/*
    int_t hashValue = 0,i = 0;
    value p;

    if( !CsObjectP(obj) && !CsCObjectP(obj) )
      CsUnexpectedTypeError(c,val, "instance of Object class");

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
      return true;
    }

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      CsAlreadyDefined(c, tag);
      return true;
    }

    value self = obj;

    CsAddProperty(c,self,tag,val,hashValue,i);
    return true;
*/
    int_t hashValue,i;
    value p;

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) && val != UNDEFINED_VALUE )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
    }
    else if (!(p = CsFindProperty(c,obj,tag,&hashValue,&i)))
    {
      //CsAddProperty(c,obj,tag,val,hashValue,i);
      dispatch* pd = CsGetDispatch(obj);
      pd->setItem(c,obj,tag,val);
    }
    else
    {
      value t = CsPropertyValue(p);
      //if(CsPropertyMethodP(t))
      //  CsSendMessage(c,obj,t,1, val );
      //else
      CsSetPropertyValue(p,val);
    }
    return true;

}

/*
bool CsSetObjectProperty(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( tag == PROTOTYPE_SYM )
    {
      if( !CsObjectP(val) )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
    }
    else if (!(p = CsFindProperty(c,obj,tag,&hashValue,&i)))
        CsAddProperty(c,obj,tag,val,hashValue,i);
    else
    {
      value t = CsPropertyValue(p);
      if(CsPropertyMethodP(t))
      {
        CsSendMessage(c,obj,t,1, val );
      }
      else
        CsSetPropertyValue(p,val);
    }
    return true;
}
*/


/* CsObjectSize - Object size handler */
long CsObjectSize(value obj)
{
    return sizeof(object);
}

/* CsObjectScan - Object scan handler */
void CsObjectScan(VM *c,value obj)
{
    CsSetObjectClass(obj,CsCopyValue(c,CsObjectClass(obj)));
    CsSetObjectProperties(obj,CsCopyValue(c,CsObjectProperties(obj)));
}


/* CsClassSize - Class size handler */
long CsClassSize(value obj)
{
    return sizeof(klass);
}

/* CsObjectScan - Object scan handler */
void CsClassScan(VM *c,value obj)
{
    CsObjectScan(c,obj);
    CsSetClassName(obj,CsCopyValue(c,CsClassName(obj)));
    CsSetClassNamespace(obj,CsCopyValue(c,CsClassNamespace(obj)));
    CsSetClassUndefinedPropHandler(obj,CsCopyValue(c,CsClassUndefinedPropHandler(obj)));
}


/* CsMakeObject - make a new obj */
value CsMakeObject(VM *c,value proto)
{
    value newo;
    CsCPush(c,proto);
    newo = CsAllocate(c,sizeof(object));
    CsSetDispatch(newo,&CsObjectDispatch);
    CsSetObjectClass(newo,CsPop(c));
    CsSetObjectProperties(newo,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(newo,0);
    _CsInitPersistent(newo);
    assert(sizeof(object) == ValueSize(newo));
    return newo;
}

/* CsMakeObject - make a new obj */
value CsMakeClass(VM *c,value proto)
{
    value newo;
    CsCPush(c,proto);
    newo = CsAllocate(c,sizeof(klass));
    CsSetDispatch(newo,&CsClassDispatch);
    CsSetClassName(newo,UNDEFINED_VALUE);
    CsSetClassUndefinedPropHandler(newo,UNDEFINED_VALUE);
    CsSetObjectClass(newo,CsPop(c));
    CsSetObjectProperties(newo,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(newo,0);
    assert(sizeof(klass) == ValueSize(newo));
    return newo;
}

value CsNewClassInstance(VM* c,value parentClass, value nameSymbol)
{
    value newo;
    CsCPush(c,parentClass);
    CsCPush(c,nameSymbol);
    newo = CsAllocate(c,sizeof(klass));
    CsSetDispatch(newo,&CsClassDispatch);
    CsSetClassName(newo,CsPop(c));
    CsSetClassNamespace(newo, c->currentNS );
    CsSetObjectClass(newo,CsPop(c));
    CsSetClassUndefinedPropHandler(newo,UNDEFINED_VALUE);
    CsSetObjectProperties(newo,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(newo,0);
    assert(sizeof(klass) == ValueSize(newo));
    return newo;
}

value CsNewNamespaceInstance(VM* c,value parentNamespace, value nameSymbol)
{
    value newo;
    CsCPush(c,parentNamespace);
    CsCPush(c,nameSymbol);
    newo = CsAllocate(c,sizeof(klass));
    CsSetDispatch(newo,&CsNamespaceDispatch);
    CsSetClassName(newo,CsPop(c));
    CsSetClassNamespace(newo, c->currentNS );
    CsSetObjectClass(newo,CsPop(c));
    CsSetClassUndefinedPropHandler(newo,UNDEFINED_VALUE);
    CsSetObjectProperties(newo,UNDEFINED_VALUE);
    CsSetObjectPropertyCount(newo,0);
    assert(sizeof(klass) == ValueSize(newo));
    return newo;
}


/* CsCloneObject - clone an existing obj */
value CsCloneObject(VM *c,value obj)
{
    FETCH( c, obj );
    value properties;
    CsCheck(c,2);
    CsPush(c,obj);
    CsPush(c,CsMakeObject(c,CsObjectClass(obj)));
    properties = CsObjectProperties(c->sp[1]);
    if (CsHashTableP(properties))
        CsSetObjectProperties(CsTop(c),CopyPropertyTable(c,properties));
    else
        CsSetObjectProperties(CsTop(c),CopyPropertyList(c,properties));
    CsSetObjectPropertyCount(CsTop(c),CsObjectPropertyCount(c->sp[1]));
    obj = CsPop(c);
    CsDrop(c,1);
    return obj;
}

static value CopyPropertyTableExcept(VM *c,value table, value tag, bool& r);
static value CopyPropertyListExcept(VM *c,value table, value tag, bool& r);

bool CsRemoveObjectProperty(VM *c,value obj, value tag)
{
    FETCH_P( c, obj, tag );
    value properties;
    CsCheck(c,1);
    CsPush(c,obj);
    properties = CsObjectProperties(obj);
    bool removed = false;
    if (CsHashTableP(properties))
    {
        properties = CopyPropertyTableExcept(c,properties,tag,removed);
    }
    else
    {
        properties = CopyPropertyListExcept(c,properties,tag,removed);
    }
    if( removed )
    {
      CsSetModified( obj, true );
      CsSetObjectProperties(CsTop(c),properties);
      CsSetObjectPropertyCount(CsTop(c),CsObjectPropertyCount(CsTop(c)) - 1);
    }
    CsPop(c);
    return removed;
}


/* CsFindProperty - find a property of a non-inherited obj property */
value CsFindProperty(VM *c,value obj,value tag,int_t *pHashValue,int_t *pIndex)
{
#ifdef _DEBUG
    dispatch *pd = CsGetDispatch(obj);
#endif

    //if( CsStringP(tag) && (CsStringChars(tag) == WCHARS("101")))
    //  log4::printf("getting 101\n");

    value p = CsObjectProperties(obj);
    if (CsHashTableP(p)) {
        int_t hashValue = CsHashValue(tag);
        int_t i = hashValue & (CsHashTableSize(p) - 1);
        p = CsHashTableElement(p,i);
        if (pHashValue) *pHashValue = hashValue;
        if (pIndex) *pIndex = i;
    }
    else {
        if (pIndex) *pIndex = -1;
    }
    for (; p != UNDEFINED_VALUE; p = CsPropertyNext(p))
    {
        value ptag = CsPropertyTag(p);
        if (CsEql(ptag,tag))
            return p;
    }
    return 0;
}

value FindFirstSymbol(VM *c, value obj, value& idx)
{
    value p = CsObjectProperties(obj);
    if (CsHashTableP(p))
    {
        for(int i = 0; i < CsHashTableSize(p); ++i)
        {
          value t = CsHashTableElement(p,i);
          if(t != UNDEFINED_VALUE)
          {
            idx = t;
#ifdef _DEBUG
            dispatch *pd = CsGetDispatch(CsPropertyTag(t));
#endif
            return CsPropertyTag(t);
          }
        }
    }
    if(p != UNDEFINED_VALUE)
    {
      idx = p;
      return CsPropertyTag(p);
    }
    else
    {
      idx = UNDEFINED_VALUE;
      return NOTHING_VALUE;
    }
}

value FindFirstSymbolValue(VM *c, value obj, value& idx)
{
    value p = CsObjectProperties(obj);
    if (CsHashTableP(p))
    {
        for(int i = 0; i < CsHashTableSize(p); ++i)
        {
          value t = CsHashTableElement(p,i);
          if(t != UNDEFINED_VALUE)
          {
            idx = t;
#ifdef _DEBUG
            dispatch *pd = CsGetDispatch(CsPropertyTag(t));
#endif
            CsSetRVal(c,1,CsPropertyTag(t));
            return CsPropertyValue(t);
          }
        }
    }
    if(p != UNDEFINED_VALUE)
    {
      idx = p;
      CsSetRVal(c,1,CsPropertyTag(p));
      return CsPropertyValue(p);
    }
    else
    {
      idx = UNDEFINED_VALUE;
      return NOTHING_VALUE;
    }
}


value FindNextSymbol(VM *c,value obj, value& idx)
{
    if( idx == UNDEFINED_VALUE )
      return NOTHING_VALUE;

    value np = CsPropertyNext(idx);
    if( np != UNDEFINED_VALUE ) 
    {
      idx = np; // easy case
      return CsPropertyTag(np);
    }

    value props = CsObjectProperties(obj);
    value tag = CsPropertyTag(idx);

    if (!CsHashTableP(props)) // this is simple prop list
                              // its end reached.
    {
      idx = UNDEFINED_VALUE;
      return NOTHING_VALUE;
    }
    
    int_t hashValue = CsHashValue(tag);
    int_t i = hashValue & (CsHashTableSize(props) - 1);

    for(++i; i < CsHashTableSize(props); ++i)
    {
      value p = CsHashTableElement(props,i);
      if(p != UNDEFINED_VALUE)
      {
        idx = p; 
        return CsPropertyTag(p);
      }
    }

    idx = UNDEFINED_VALUE; // end of table has been reached;
    return NOTHING_VALUE;
}

value FindNextSymbolValue(VM *c,value obj, value& idx)
{
    if( idx == UNDEFINED_VALUE )
      return NOTHING_VALUE;

    value np = CsPropertyNext(idx);
    if( np != UNDEFINED_VALUE ) 
    {
      idx = np; // easy case
      CsSetRVal(c,1,CsPropertyTag(np));
      return CsPropertyValue(np);
    }

    value props = CsObjectProperties(obj);
    value tag = CsPropertyTag(idx);

    if (!CsHashTableP(props)) // this is simple prop list
                              // its end reached.
    {
      idx = UNDEFINED_VALUE;
      CsSetRVal(c,1,NOTHING_VALUE);
      return NOTHING_VALUE;
    }
    
    int_t hashValue = CsHashValue(tag);
    int_t i = hashValue & (CsHashTableSize(props) - 1);

    for(++i; i < CsHashTableSize(props); ++i)
    {
      value p = CsHashTableElement(props,i);
      if(p != UNDEFINED_VALUE)
      {
        idx = p; 
        CsSetRVal(c,1,CsPropertyTag(p));
        return CsPropertyValue(p);
      }
    }
    idx = UNDEFINED_VALUE; // end of table has been reached;
    CsSetRVal(c,1,NOTHING_VALUE);
    return NOTHING_VALUE;
}


value CsObjectNextElement(VM *c, value* index, value obj, int nr)
{
  if( *index == NOTHING_VALUE ) // first
  {
    FETCH(c,obj);
    return nr > 1? 
           FindFirstSymbolValue(c,obj,*index):
           FindFirstSymbol(c,obj,*index);
  }
  else
    return nr > 1? 
      FindNextSymbolValue(c,obj,*index):
      FindNextSymbol(c,obj,*index);
}


/* CopyPropertyList - copy the property list of an obj */
static value CopyPropertyList(VM *c,value plist)
{
    CsCheck(c,2);
    CsPush(c,UNDEFINED_VALUE);
    CsPush(c,plist);
    for (; CsTop(c) != UNDEFINED_VALUE; CsSetTop(c,CsPropertyNext(CsTop(c)))) 
    {
        value tag = CsPropertyTag(CsTop(c));
        value val = CsPropertyValue(CsTop(c));
        value newo = CsMakeProperty(c,tag,val,CsPropertyFlags(CsTop(c)));
        CsSetPropertyNext(newo,c->sp[1]);
        c->sp[1] = newo;
    }
    CsDrop(c,1);
    return CsPop(c);
}

// used for deletion of properties
static value CopyPropertyListExcept(VM *c,value plist, value tag, bool& r)
{
    value p = 0;
    value t = plist;

    for (; t != UNDEFINED_VALUE; t = CsPropertyNext(t))
    {
        value pTag = CsPropertyTag(t);
        if( CsEql(tag,pTag))
        {
          if( CsPropertyIsConst(t) )
            CsThrowKnownError(c,CsErrReadOnlyProperty,tag);          
          r = true;
          if( p ) 
            CsSetPropertyNext(p,CsPropertyNext(t));
          else
            plist = CsPropertyNext(t);
          break;
        }
        p = t;
    }
    return plist;
}

/*
static value CopyPropertyListExcept(VM *c,value plist, value tag, bool& r)
{
    CsCheck(c,2);
    CsPush(c,UNDEFINED_VALUE);
    CsPush(c,plist);
    for (; CsTop(c) != UNDEFINED_VALUE; CsSetTop(c,CsPropertyNext(CsTop(c))))
    {
        value pTag = CsPropertyTag(CsTop(c));
        if( tag == pTag)
        {
          r = true;
          continue;
        }
        value pValue = CsPropertyValue(CsTop(c));
        value newo = CsMakeProperty(c,pTag,pValue,CsPropertyFlags(CsTop(c)));
        CsSetPropertyNext(newo,c->sp[1]);
        c->sp[1] = newo;
    }
    CsDrop(c,1);
    return CsPop(c);
}

*/


/* CopyPropertyTable - copy the property hash table of an obj */
static value CopyPropertyTable(VM *c,value table)
{
    int_t size = CsHashTableSize(table);
    int_t i;
    CsCheck(c,2);
    CsPush(c,CsMakeHashTable(c,size));
    CsPush(c,table);
    for (i = 0; i < size; ++i) {
        value properties = CopyPropertyList(c,CsHashTableElement(CsTop(c),i));
        CsSetHashTableElement(c->sp[1],i,properties);
    }
    CsDrop(c,1);
    return CsPop(c);
}

/* CopyPropertyTable - copy the property hash table of an obj */
static value CopyPropertyTableExcept(VM *c,value table, value tag, bool& r)
{
    r = false;
    int_t size = CsHashTableSize(table);
    int_t i;
    CsCheck(c,1);
    //CsPush(c,CsMakeHashTable(c,size));
    //CsPush(c,table);
    CsPush(c,table);
    //CsPush(c,CsMakeHashTable(c,size));
    //tool::swap( c->sp[1], c->sp[0] );
    
    for (i = 0; i < size; ++i) {
        value properties = CopyPropertyListExcept(c,CsHashTableElement(CsTop(c),i), tag, r );
        CsSetHashTableElement(CsTop(c),i,properties);
    }
    //CsDrop(c,1);
    return CsPop(c);
}

/* CsSetCObjectProperty - CObject set property handler */
static bool AddObjectConstant(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      CsAlreadyDefined(c,tag);
    }

    /* add a property */
    CsAddProperty(c,obj,tag,val,hashValue,i, PROP_CONST);
    return true;
}


/* CsAddProperty - add a property to an obj */
void CsAddProperty(VM *c,value obj,value tag,value val,int_t hashValue, int_t i, int_t flags)
{
    value p;
    CsCPush(c,obj);
    p = CsMakeProperty(c,tag,val, flags);
    obj = CsTop(c);
    if (i >= 0) {
        int_t currentSize = CsHashTableSize(CsObjectProperties(obj));
        if (CsObjectPropertyCount(obj) >= currentSize * CsHashTableExpandThreshold) {
            CsCheck(c,1);
            //CsPush(c,obj);
            CsPush(c,p);
            i = ExpandHashTable(c,obj,hashValue);
            p = CsPop(c);
            obj = CsTop(c);
        }
        CsSetPropertyNext(p,CsHashTableElement(CsObjectProperties(obj),i));
        CsSetHashTableElement(CsObjectProperties(obj),i,p);
    }
    else {
        if (CsObjectPropertyCount(obj) >= CsHashTableCreateThreshold)
            CreateHashTable(c,obj,p);
        else {
            CsSetPropertyNext(p,CsObjectProperties(obj));
            CsSetObjectProperties(obj,p);
        }
    }
    obj = CsPop(c);
    IncObjectPropertyCount(obj);
}

/* CreateHashTable - create an obj hash table and enter a property */
static void CreateHashTable(VM *c,value obj,value p)
{
    int_t i;
    value table;
    CsCheck(c,2);
    CsPush(c,p);
    CsPush(c,obj);
    table = CsMakeHashTable(c,CsHashTableCreateThreshold);
    obj = CsPop(c);
    p = CsObjectProperties(obj);
    CsSetObjectProperties(obj,table);
    while (p != UNDEFINED_VALUE) {
        value next = CsPropertyNext(p);
        i = CsHashValue(CsPropertyTag(p)) & (CsHashTableCreateThreshold - 1);
        CsSetPropertyNext(p,CsHashTableElement(table,i));
        CsSetHashTableElement(table,i,p);
        p = next;
    }
    p = CsPop(c);
    i = CsHashValue(CsPropertyTag(p)) & (CsHashTableCreateThreshold - 1);
    CsSetPropertyNext(p,CsHashTableElement(table,i));
    CsSetHashTableElement(table,i,p);
}

/* ExpandHashTable - expand an obj hash table and return the new hash index */
static int ExpandHashTable(VM *c,value obj,int_t hashValue)
{
    int_t oldSize,newSize;
    oldSize = CsHashTableSize(CsObjectProperties(obj));
    newSize = oldSize << 1;
    if (newSize <= CsHashTableExpandMaximum) {
        value oldTable,newTable;
        int_t j;
        CsPush(c,obj);
        newTable = CsMakeHashTable(c,newSize);
        oldTable = CsObjectProperties(CsTop(c));
        for (j = 0; j < oldSize; ++j) {
            value p = CsHashTableElement(oldTable,j);
            value new0 = UNDEFINED_VALUE;
            value new1 = UNDEFINED_VALUE;
            while (p != UNDEFINED_VALUE) {
                value next = CsPropertyNext(p);
                if (CsHashValue(CsPropertyTag(p)) & oldSize) {
                    CsSetPropertyNext(p,new1);
                    new1 = p;
                }
                else {
                    CsSetPropertyNext(p,new0);
                    new0 = p;
                }
                p = next;
            }
            CsSetHashTableElement(newTable,j,new0);
            CsSetHashTableElement(newTable,j + oldSize,new1);
        }
        CsSetObjectProperties(CsPop(c),newTable);
    }
    return hashValue & (newSize - 1);
}

/* PRBC_ERTY */

#define SetPropertyTag(o,v)             CsSetFixedVectorElement(o,0,v)

/* Property handlers */
static long PropertySize(value obj);
static void PropertyScan(VM *c,value obj);

/* Property pdispatch */
dispatch CsPropertyDispatch = {
    "PropertyTuple",
    &CsFixedVectorDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    PropertySize,
    CsDefaultCopy,
    PropertyScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* PropertySize - Property size handler */
static long PropertySize(value obj)
{
    return sizeof(CsFixedVector) + CsPropertySize * sizeof(value);
}

/* PropertyScan - Property scan handler */
static void PropertyScan(VM *c,value obj)
{
    long i;
    for (i = 0; i < CsPropertySize; ++i)
        CsSetFixedVectorElement(obj,i,CsCopyValue(c,CsFixedVectorElement(obj,i)));
}

/* CsMakeProperty - make a property */
value CsMakeProperty(VM *c,value& key,value& val, int_t flags)
{
    value newo;
    CsCheck(c,2);
    CsPush(c,val);
    CsPush(c,key);
    newo = CsMakeFixedVectorValue(c,&CsPropertyDispatch,CsPropertySize);
    key = CsPop(c);
    val = CsPop(c);
#ifdef _DEBUG
    if(key == 0x0542a6d800000403i64)
      key = key;
#endif
    SetPropertyTag(newo,key);
    CsSetPropertyValue(newo,val);
    CsSetPropertyFlags(newo,flags);
    return newo;
}



}
