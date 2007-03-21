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

static value CSF_length(VM *c,value obj);
static value CSF_prototype(VM *c,value obj);
static void CSF_set_prototype(VM *c,value obj, value pro);

#define FETCH(c,obj) if( _CsIsPersistent(obj) ) obj = CsFetchObjectData(c, obj);

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
C_METHOD_ENTRY(	0,                  0                   )
};

/* Object properties */
static vp_method properties[] = {
//VP_METHOD_ENTRY( "class",    CSF_prototype, CSF_set_prototype ),
VP_METHOD_ENTRY( "length",   CSF_length, 0 ),
VP_METHOD_ENTRY( 0,          0,					0					)
};

/* CsInitObject - initialize the 'Object' obj */
void CsInitObject(VM *c)
{
    /* make the base of the obj inheritance tree */
    c->objectObject = CsEnterObject(CsGlobalScope(c),"Object",c->undefinedValue,methods, properties);
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
    return CsMakeInteger(c,CsObjectPropertyCount(obj));
}


static value CSF_prototype(VM *c,value obj)
{
    FETCH( c, obj );
    return CsObjectClass(obj);
}

static void CSF_set_prototype(VM *c,value obj, value pro)
{
    FETCH( c, obj );
    if( CsObjectOrMethodP(pro) )
      CsSetObjectClass(obj, pro);
    else
      CsTypeError(c, pro);
}


/* CSF_vlone - built-in method 'Clone' */
static value CSF_clone(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsObjectDispatch);
    FETCH( c, obj );
    return CsCloneObject(c,obj);
}

/* CSF_Exists - built-in method 'Exists' */
static value CSF_exists(VM *c)
{
    value obj,tag;
    bool deep = false;
    CsParseArguments(c,"V=*V|B",&obj,&CsObjectDispatch,&tag);
    FETCH( c, obj );
    while (CsObjectP(obj)) {
        if (CsFindProperty(c,obj,tag,NULL,NULL))
            return c->trueValue;
        if( !deep ) break;
        obj = CsObjectClass(obj);
    }
    return c->falseValue;
}

/* CSF_remove - built-in method 'Remove' */
static value CSF_remove(VM *c)
{
    value obj,tag;
    CsParseArguments(c,"V=*V",&obj,&CsObjectDispatch,&tag);
    FETCH( c, obj );
    CsRemoveObjectProperty(c,obj,tag);
    return c->undefinedValue;
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
            for (; prop != c->undefinedValue; prop = CsPropertyNext(prop))
            {
              if(!osc.item(c,CsPropertyTag(prop),CsPropertyValue(prop)))
                 return;
            }
        }
    }
    else
        for (; props != c->undefinedValue; props = CsPropertyNext(props))
        {
            if(!osc.item(c,CsPropertyTag(props),CsPropertyValue(props)))
              return;
        }
}

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
                for (; prop != c->undefinedValue; prop = CsPropertyNext(prop)) {
                    s->put_str("  ");
                    CsPrint(c,CsPropertyTag(prop),s);
                    s->put_str(": ");
                    CsPrint(c,CsPropertyValue(prop),s);
                    s->put('\n');
                }
            }
        }
        else {
            for (; props != c->undefinedValue; props = CsPropertyNext(props)) {
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
   return c->falseValue;
  if( !CsIsModified( obj ) )
   return c->trueValue;

  CsStoreObjectData(c, obj);
  return c->trueValue;
}

/* CSF_rollback - built-in method 'Rollback' for persistent objects */
/* usage: obj.restore(); */
value CSF_restore(VM *c)
{
  value obj;
  CsParseArguments( c, "V=*", &obj, &CsObjectDispatch );

  if( !CsIsPersistent(c,obj) )
  { return c->falseValue; }

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
static value ObjectNextElement(VM *c, value* index, value obj);
static bool AddObjectConstant(VM *c,value obj,value tag,value val);


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
    ObjectNextElement,
    AddObjectConstant,
};


value CsObjectGetItem(VM *c,value obj,value tag)
{
   value val;
   if(!CsGetObjectProperty(c,obj,tag,&val))
     return c->undefinedValue;

   return val;
}

void CsObjectSetItemNoLoad(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( CsSymbolP(tag) && tag == c->prototypeSym )
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
        CsSendMessage(c,obj,t,1, val );
      else
        CsSetPropertyValue(p,val);
    }
}

void     CsObjectSetItem(VM *c,value obj,value tag,value val)
{
   FETCH(c,obj);
   CsSetModified(obj, true);
   CsObjectSetItemNoLoad(c,obj, tag,val);
  //CsSetObjectProperty(c,obj,tag,value);
}

/* CsGetProperty - recursively search for a property value */
bool CsGetProperty(VM *c,value obj,value tag,value *pValue)
{
    value self = obj;
    while (!CsGetProperty1(c,obj,tag,pValue))
    {
        if (!CsObjectOrMethodP(obj) || (obj = CsObjectClass(obj)) == 0)
            return false;
    }
    if(CsPropertyMethodP(*pValue))
    {
      *pValue = CsSendMessage(c,self,*pValue,1, c->nothingValue );
    }

    return true;
}


/* CsGetObjectProperty - Object get property handler */
bool CsGetObjectProperty(VM *c,value obj,value tag,value *pValue)
{
    FETCH(c,obj);
    value p;

    if( CsSymbolP(tag) && tag == c->prototypeSym )
    {
      *pValue = CsObjectClass(obj);
      return true;
    }

    value self = obj;

    do if( p = CsFindProperty(c,obj,tag,NULL,NULL))
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
          *pValue = CsSendMessage(c,self,propValue,1, c->nothingValue);
        else
          *pValue = propValue;
        return true;
      }
    while ( ((obj = CsObjectClass(obj)) != 0) && CsObjectOrMethodP(obj) );

    /*if (p = CsFindProperty(c,obj,tag,NULL,NULL))
    {
      *pValue = CsPropertyValue(p);
      return true;
    }*/

    /* look for a class property */
    return false;
}


bool CsSetObjectPropertyNoLoad(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( tag == c->prototypeSym )
    {
      if( !CsObjectP(val) )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
      return true;
    }

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
    }
    CsAddProperty(c,self,tag,val,hashValue,i);
    return true;
}

bool CsSetObjectPersistentProperty(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( tag == c->prototypeSym )
    {
      if( !CsObjectP(val) )
        CsUnexpectedTypeError(c,val, "instance of Object class");
      CsSetObjectClass(obj, val);
      return true;
    }

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      value propValue = CsPropertyValue(p);
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
    FETCH(c,obj);
    CsSetModified(obj, true);
    return CsSetObjectPropertyNoLoad(c,obj,tag,val);
}

const char* CsClassClassName(VM* c, value cls)
{

  static value classNameTag = 0;
  if(!classNameTag)
    classNameTag = symbol_value(".className");

  value className = 0;
  if(CsGetProperty1(c,cls, classNameTag, &className))
  {
    if( CsSymbolP(className))
      return CsSymbolPrintName(className);

    else
      dprintf("", CsTypeName(className));
  }
  return "";
}

const char* CsObjectClassName(VM* c, value obj)
{
  value cls = CsObjectClass(obj);
  if(cls == c->undefinedValue)
    return "";
  return CsClassClassName(c,cls);
}


bool CsAssignMethod(VM *c, value obj,value tag,value val )
{
/*
    int_t hashValue = 0,i = 0;
    value p;

    if( !CsObjectP(obj) && !CsCObjectP(obj) )
      CsUnexpectedTypeError(c,val, "instance of Object class");

    if( CsSymbolP(tag) && tag == c->prototypeSym )
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

    if( CsSymbolP(tag) && tag == c->prototypeSym )
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

    if( CsSymbolP(tag) && tag == c->prototypeSym )
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

/* CsMakeObject - make a new obj */
value CsMakeObject(VM *c,value proto)
{
    value newo;
    CsCPush(c,proto);
    newo = CsAllocate(c,sizeof(object));
    CsSetDispatch(newo,&CsObjectDispatch);
    CsSetObjectClass(newo,CsPop(c));
    CsSetObjectProperties(newo,c->undefinedValue);
    CsSetObjectPropertyCount(newo,0);
    _CsInitPersistent(newo);
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

void CsRemoveObjectProperty(VM *c,value obj, value tag)
{
    FETCH( c, obj );
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
      CsSetObjectPropertyCount(CsTop(c),CsObjectPropertyCount(c->sp[1]) - 1);
    }
    CsPop(c);
}


/* CsFindProperty - find a property of a non-inherited obj property */
value CsFindProperty(VM *c,value obj,value tag,int_t *pHashValue,int_t *pIndex)
{
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
    for (; p != c->undefinedValue; p = CsPropertyNext(p))
        if (CsEql(CsPropertyTag(p),tag))
            return p;
    return 0;
}

value FindFirstSymbol(VM *c,value obj)
{
    value p = CsObjectProperties(obj);
    if (CsHashTableP(p))
    {
        for(int i = 0; i < CsHashTableSize(p); ++i)
        {
          value t = CsHashTableElement(p,i);
          if(t != c->undefinedValue)
            return CsPropertyTag(t);
        }
    }
    if(p != c->undefinedValue)
      return CsPropertyTag(p);
    else
      return c->nothingValue;
}

value FindNextSymbol(VM *c,value obj,value tag)
{
    value p = CsObjectProperties(obj);
    value objprops = p;

    int_t hashValue;
    int_t i;

    if (CsHashTableP(p)) {
        hashValue = CsHashValue(tag);
        i = hashValue & (CsHashTableSize(p) - 1);
        p = CsHashTableElement(p,i);
    }
    else
        i = -1;

    // find current tag
    for (; p != c->undefinedValue; p = CsPropertyNext(p))
        if (CsEql(CsPropertyTag(p),tag))
            break;
    if(p == c->undefinedValue)
    {
      assert(false);
      return c->nothingValue;
    }
    p = CsPropertyNext(p);
    if(p != c->undefinedValue)
        return CsPropertyTag(p);

    if(i >= 0)
    {
      for(++i; i < CsHashTableSize(objprops); ++i)
      {
        p = CsHashTableElement(objprops,i);
        if(p != c->undefinedValue)
          return CsPropertyTag(p);
      }
    }
    return c->nothingValue;
}

value ObjectNextElement(VM *c, value* index, value obj)
{
  if( *index == c->nothingValue ) // first
  {
    FETCH(c,obj);
    return *index = FindFirstSymbol(c,obj);
  }
  else
    return *index = FindNextSymbol(c,obj,*index);
}

/* CopyPropertyList - copy the property list of an obj */
static value CopyPropertyList(VM *c,value plist)
{
    CsCheck(c,2);
    CsPush(c,c->undefinedValue);
    CsPush(c,plist);
    for (; CsTop(c) != c->undefinedValue; CsSetTop(c,CsPropertyNext(CsTop(c)))) {
        value newo = CsMakeProperty(c,CsPropertyTag(CsTop(c)),CsPropertyValue(CsTop(c)), CsPropertyFlags(CsTop(c))  );
        CsSetPropertyNext(newo,c->sp[1]);
        c->sp[1] = newo;
    }
    CsDrop(c,1);
    return CsPop(c);
}

static value CopyPropertyListExcept(VM *c,value plist, value tag, bool& r)
{
    CsCheck(c,2);
    CsPush(c,c->undefinedValue);
    CsPush(c,plist);
    for (; CsTop(c) != c->undefinedValue; CsSetTop(c,CsPropertyNext(CsTop(c))))
    {
        value pTag = CsPropertyTag(CsTop(c));
        if( CsEqualOp(c,tag, pTag))
          continue;
        value pValue = CsPropertyValue(CsTop(c));
        value newo = CsMakeProperty(c,pTag,pValue,CsPropertyFlags(CsTop(c)));
        CsSetPropertyNext(newo,c->sp[1]);
        c->sp[1] = newo;
    }
    CsDrop(c,1);
    return CsPop(c);
}




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
    CsCheck(c,2);
    CsPush(c,CsMakeHashTable(c,size));
    CsPush(c,table);
    for (i = 0; i < size; ++i) {
        value properties = CopyPropertyListExcept(c,CsHashTableElement(CsTop(c),i), tag, r );
        CsSetHashTableElement(c->sp[1],i,properties);
    }
    CsDrop(c,1);
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
    obj = CsPop(c);
    if (i >= 0) {
        int_t currentSize = CsHashTableSize(CsObjectProperties(obj));
        if (CsObjectPropertyCount(obj) >= currentSize * CsHashTableExpandThreshold) {
            CsCheck(c,2);
            CsPush(c,obj);
            CsPush(c,p);
            i = ExpandHashTable(c,c->sp[1],hashValue);
            p = CsPop(c);
            obj = CsPop(c);
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
    while (p != c->undefinedValue) {
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
            value new0 = c->undefinedValue;
            value new1 = c->undefinedValue;
            while (p != c->undefinedValue) {
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
    &CsPropertyDispatch,
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
value CsMakeProperty(VM *c,value key,value val, int_t flags)
{
    value newo;
    CsCheck(c,2);
    CsPush(c,val);
    CsPush(c,key);
    newo = CsMakeFixedVectorValue(c,&CsPropertyDispatch,CsPropertySize);
    SetPropertyTag(newo,CsPop(c));
    CsSetPropertyValue(newo,CsPop(c));
    CsSetPropertyFlags(newo,flags);
    return newo;
}

//--------
/* Property handlers */
static long NamespaceSize(value obj);
static void NamespaceScan(VM *c,value obj);

/* Property pdispatch */
dispatch CsNamespaceDispatch = {
    "NamespaceTuple",
    &CsPropertyDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    NamespaceSize,
    CsDefaultCopy,
    NamespaceScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* PropertySize - Property size handler */
static long NamespaceSize(value obj)
{
    return sizeof(CsFixedVector) + CsNamespaceSize * sizeof(value);
}

/* PropertyScan - Property scan handler */
static void NamespaceScan(VM *c,value obj)
{
    long i;
    for (i = 0; i < CsNamespaceSize; ++i)
        CsSetFixedVectorElement(obj,i,CsCopyValue(c,CsFixedVectorElement(obj,i)));
}

/* CsMakeProperty - make a property */
value CsMakeNamespace(VM *c,value globals,value next)
{
    value newo;
    CsCheck(c,2);
    CsPush(c,globals);
    CsPush(c,next);
    newo = CsMakeFixedVectorValue(c,&CsNamespaceDispatch,CsNamespaceSize);
    CsSetNamespaceNext(newo,CsPop(c));
    CsSetNamespaceGlobals(newo,CsPop(c));
    return newo;
}


}
