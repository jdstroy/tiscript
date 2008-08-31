/* cobject.c - 'CObject' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <string.h>
#include "cs.h"

namespace tis 
{

/* COBJECT */

inline value CObjectNext(value o)
{
  c_object* po = ptr<c_object>(o);
  return po->next;
}
inline void SetCObjectNext(value o, value v)
{
  c_object* po = ptr<c_object>(o);
  po->next = v;
}

/* CObject handlers */
static value CObjectNewInstance(VM *c,value proto);
static value CPtrObjectNewInstance(VM *c,value proto);
static long CObjectSize(value obj);

/* CObject pdispatch */
dispatch CsCObjectDispatch = {
    "CObject",
    &CsObjectDispatch,
    CsGetCObjectProperty,
    CsSetCObjectProperty,
    CObjectNewInstance,
    CsDefaultPrint,
    CObjectSize,
    CsDefaultCopy,
    CsCObjectScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem,
    0,
    CsAddCObjectConstant
};

/* CsCObjectP - is this value a cobject? */
bool CsCObjectP(value val)
{
    dispatch *d = CsGetDispatch(val);
    return d->size == CObjectSize;
}

/* CsGetCObjectProperty - CObject get property handler */
bool CsGetCObjectProperty(VM *c,value obj,value tag,value *pValue)
{
    //return CsGetObjectProperty(c,obj,tag, pValue);
    value p;

    dispatch *d = CsGetDispatch(obj);

    /* look for a local property */
    if ((p = CsFindProperty(c,obj,tag,0,0)) != 0) 
    {
        //*pValue = CsPropertyValue(p);
        value propValue = CsPropertyValue(p);
		    if (CsVPMethodP(propValue)) 
        {
			    vp_method *method = ptr<vp_method>(propValue);
          if (method->get(c,obj,*pValue)) 
            return true;
			    else
				    CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
		    }
        else if(CsPropertyMethodP(propValue)) 
          *pValue = CsSendMessage(c,obj,propValue,1, c->nothingValue);
        else
          *pValue = propValue;

        return true;
    }

    /* look for a class property */
    else {
        dispatch *d;

        /* look for a method in the CObject proto chain */
        for (d = CsQuickGetDispatch(obj); d != 0; d = d->proto) {
            if ((p = CsFindProperty(c,d->obj,tag,0,0)) != 0) 
            {
		          value propValue = CsPropertyValue(p);
		          if (CsVPMethodP(propValue)) 
              {
			          vp_method *method = ptr<vp_method>(propValue);
                if (method->get(c,obj,*pValue)) 
                  return true;
			          else
				          CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
		          }
              else if(CsPropertyMethodP(propValue)) 
                *pValue = CsSendMessage(c,obj,propValue,1, c->nothingValue);
              else 
			          *pValue = propValue;
              return true;
            }
        }
	  }

    /* not found */
    return false;

}


/* CsSetCObjectProperty - CObject set property handler */
bool CsAddCObjectConstant(VM *c,value obj,value tag,value val)
{
    int_t hashValue,i;
    value p;

    if( p = CsFindProperty(c,obj,tag,&hashValue,&i))
    {
      //CsThrowKnownError(c,CsErrAlreadyDefined,tag);
      CsAlreadyDefined(c,tag);
    }

    /* add a property */
    CsAddProperty(c,obj,tag,val,hashValue,i, PROP_CONST);
    return true;
}

bool CsSetCObjectProperty(VM *c,value obj,value tag,value val)
{
    int_t hashValue = 0,i;
    value p;


    /* look for a local property */
    if ((p = CsFindProperty(c,obj,tag,&hashValue,&i)) != 0) {
        if( CsPropertyIsConst(p) )
          CsThrowKnownError(c,CsErrReadOnlyProperty,tag);

        value propValue = CsPropertyValue(p);

		    if (CsVPMethodP(propValue)) 
        {
			    vp_method *method = ptr<vp_method>(propValue);
          if (method->set(c,obj,val))
            return true;
			    else
				    CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
		    }
        else if(CsPropertyMethodP(propValue))
          CsSendMessage(c,obj,propValue,1, val );
        else
          CsSetPropertyValue(p,val);
        return true;
    }

    /* look for a class property */
    else {
        dispatch *d;

        /* look for a method in the CObject proto chain */
        for (d = CsQuickGetDispatch(obj); d != 0; d = d->proto) {
	        if ((p = CsFindProperty(c,d->obj,tag,0,0)) != 0) 
          {
		        value propValue = CsPropertyValue(p);

		        if (CsVPMethodP(propValue)) 
            {
			        vp_method *method = ptr<vp_method>(propValue);
              if (method->set(c,obj,val))
                return true;
			        else
				        CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
		        }
            else if(CsPropertyMethodP(propValue)) 
            {
              CsSendMessage(c,obj,propValue,1, val);
              return true;
            }

            //else if( CsPropertyIsConst(p) )
            //  CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
            //else 
            //{
			      //  CsAddProperty(c,obj,tag,val,hashValue,i);
            //  return true;
            //}
          }
        }
    }

    /* add a property */
    //if(!hashValue) hashValue = CsHashValue(tag);
    CsAddProperty(c,obj,tag,val,hashValue,i);
    return true;
}


/* CObjectNewInstance - CObject new instance handler */
static value CObjectNewInstance(VM *c,value proto)
{
    dispatch *d = (dispatch *)CsCObjectValue(proto);
    return CsMakeCObject(c,d);
}

value CsCObjectGetItem(VM *c,value obj,value tag)
{
   //value val;
   //if(!GetObjectProperty(c,obj,tag,&val))
   //  return c->undefinedValue;
   dispatch *d = (dispatch *)CsCObjectValue(obj);
   if(d->getItem != CsDefaultGetItem)
     return d->getItem(c,obj,tag);
   return CsObjectGetItem(c,obj,tag);
}
void     CsCObjectSetItem(VM *c,value obj,value tag,value value)
{
   //CsCObjectSetItem(c,obj,tag,value);

   dispatch *d = (dispatch *)CsCObjectValue(obj);
   if(d->setItem != CsDefaultSetItem)
     d->setItem(c,obj,tag,value);
   else
     CsObjectSetItem(c,obj,tag,value);

  //SetObjectProperty(c,obj,tag,value);
}



/* CPtrObjectNewInstance - CPtrObject new instance handler */
static value CPtrObjectNewInstance(VM *c,value proto)
{
    dispatch *d = (dispatch *)CsCObjectValue(proto);
    value obj = CsMakeCObject(c,d);
    CsSetCObjectValue(obj,NULL);
    return obj;
}

/* CObjectSize - CObject size handler */
static long CObjectSize(value obj)
{
    dispatch *d = CsQuickGetDispatch(obj);
    return sizeof(c_object) + d->dataSize;
}

/* CsCObjectScan - CObject scan handler */
void CsCObjectScan(VM *c,value obj)
{
    SetCObjectNext(obj,c->newSpace->cObjects);
    c->newSpace->cObjects = obj;
    CsObjectDispatch.scan(c,obj);
}

/* CsMakeCObjectType - make a new cobject type */
dispatch *CsMakeCObjectType(
     VM *c,
     dispatch *proto,
     char *typeName,
     c_method *methods,
     vp_method *properties,
     constant *constants,
     long size)
{
    dispatch *d;

    /* make and initialize the type pdispatch structure */
    if (!(d = CsMakeDispatch(c,typeName,&CsCObjectDispatch)))
        return NULL;
    d->proto = proto;
    d->dataSize = size;

    d->getItem = CsDefaultGetItem;
    d->setItem = CsDefaultSetItem;
    d->addConstant = CsAddCObjectConstant;

    /* make the type obj */
    d->obj = CsMakeCPtrObject(c,c->typeDispatch,d);

    /* enter the methods and properties */
    CsEnterCObjectMethods(c,d,methods,properties, constants);

    /* return the new type */
    return d;
}

/* CsMakeCPtrObjectType - make a new cobject type */
dispatch *CsMakeCPtrObjectType(VM *c,dispatch *proto,char *typeName,c_method *methods,vp_method *properties, constant *constants)
{
    dispatch *d = CsMakeCObjectType(c,proto,typeName,methods,properties,constants, sizeof(CsCPtrObject) - sizeof(c_object));
    if (d) d->newInstance = CPtrObjectNewInstance;
    return d;
}

/* CsEnterCObjectMethods - add methods and properties to a cobject type */
void CsEnterCObjectMethods(VM *c,dispatch *d,c_method *methods,vp_method *properties, constant *constants)
{
    /* enter the methods */
    if (methods)
        CsEnterMethods(c,d->obj,methods);

    /* enter the virtual properties */
    if (properties)
        CsEnterVPMethods(c,d->obj,properties);

    if (constants)
        CsEnterConstants(c,d->obj,constants);

}

/* CsMakeCObject - make a new cobject value */
value CsMakeCObject(VM *c,dispatch *d)
{
    value newo;
    newo = CsAllocate(c,sizeof(c_object) + d->dataSize);
    CsSetDispatch(newo,d);
    SetCObjectNext(newo,c->newSpace->cObjects);
    c->newSpace->cObjects = newo;
    CsSetObjectClass(newo,c->undefinedValue);
    CsSetObjectProperties(newo,c->undefinedValue);
    CsSetObjectPropertyCount(newo,0);
    //CsSetCObjectPrototype(newo,c->undefinedValue);
    return newo;
}

/* CsMakeCPtrObject - make a new pointer cobject value */
value CsMakeCPtrObject(VM *c,dispatch *d,void *ptr)
{
    value newo = CsMakeCObject(c,d);
    CsSetCObjectValue(newo,ptr);
    return newo;
}


/* CsDestroyUnreachableCObjects - destroy unreachable cobjects */
void CsDestroyUnreachableCObjects(VM *c)
{
    value obj = c->oldSpace->cObjects;
    while (obj != 0) 
    {
        if (!CsBrokenHeartP(obj)) 
        {
            dispatch *d = CsQuickGetDispatch(obj);
            if (d->destroy) 
            {
				        void *value = CsCObjectValue(obj);
                if (value)
					        (*d->destroy)(c,obj);
			      }
        }
        obj = CObjectNext(obj);
    }
    c->oldSpace->cObjects = 0;
}

/* CsDestroyAllCObjects - destroy all cobjects */
void CsDestroyAllCObjects(VM *c)
{
    value obj = c->newSpace->cObjects;
    while (obj != 0) {
        if (!CsBrokenHeartP(obj)) {
            dispatch *d = CsQuickGetDispatch(obj);
            if (d->destroy) {
				        void *value = CsCObjectValue(obj);
                if (value) {
					          (*d->destroy)(c,obj);
                    CsSetCObjectValue(obj,NULL);
                }
			}
        }
        obj = CObjectNext(obj);
    }
    c->newSpace->cObjects = 0;
}

/* VIRTUAL PRBC_ERTY METHOD */

inline void SetVPMethodName(value o, char* v)     { ptr<vp_method>(o)->name = v; }  
//inline void SetVPMethodGetHandler(value o, vp_get_t v)  { ptr<vp_method>(o)->getHandler = v; }
//inline void SetVPMethodSetHandler(value o, vp_set_t v)  { ptr<vp_method>(o)->setHandler = v; }

/* VPMethod handlers */
static bool VPMethodPrint(VM *c,value val,stream *s, bool toLocale);
static long VPMethodSize(value obj);
static value VPMethodCopy(VM *c,value obj);

/* VPMethod pdispatch */
dispatch CsVPMethodDispatch = {
    "native-property",
    &CsVPMethodDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    VPMethodPrint,
    VPMethodSize,
    VPMethodCopy,
    CsDefaultScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* VPMethodPrint - VPMethod print handler */
static bool VPMethodPrint(VM *c,value val,stream *s, bool toLocale)
{
    return s->put_str("[native-property ")
        && s->put_str(CsVPMethodName(val))
        && s->put(']');
}

/* VPMethodSize - VPMethod size handler */
static long VPMethodSize(value obj)
{
    return sizeof( value );
}

/* VPMethodCopy - VPMethod copy handler */
static value VPMethodCopy(VM *c,value obj)
{
    return obj;
}

/* CsMakeVPMethod - make a new c method value 
value CsMakeVPMethod(VM *c,char *name,vp_get_t getHandler,vp_set_t setHandler)
{
    value newo;
    newo = CsAllocate(c,sizeof(vp_method));
    CsSetDispatch(newo,&CsVPMethodDispatch);
    SetVPMethodName(newo,name);
    SetVPMethodGetHandler(newo,getHandler);
    SetVPMethodSetHandler(newo,setHandler);
    return newo;
}
*/


/* CsGetVirtualProperty - get a property value that might be virtual */
bool CsGetVirtualProperty(VM *c,value obj,value proto,value tag,value *pValue)
{
	  value p;
    if ((p = CsFindProperty(c,proto,tag,0,0)) != 0) {
		  value propValue = CsPropertyValue(p);
		  if (CsVPMethodP(propValue)) 
      {
			  vp_method *method = ptr<vp_method>(propValue);
        
        if (method->get(c,obj,*pValue)) 
          return true;
			  else
				  CsThrowKnownError(c,CsErrWriteOnlyProperty,tag);
		  }
      else if(CsPropertyMethodP(propValue)) 
        *pValue = CsSendMessage(c,obj,propValue,0);
      else 
			  *pValue = propValue;
      return true;
	  }
    return false;
}

/* CsSetVirtualProperty - set a property value that might be virtual */
bool CsSetVirtualProperty(VM *c,value obj,value proto,value tag,value val)
{
  int_t hashValue,i;
	value p;
	if ((p = CsFindProperty(c,proto,tag,&hashValue,&i)) != 0) {

		value propValue = CsPropertyValue(p);
		if (CsVPMethodP(propValue)) 
    {
			vp_method *method = ptr<vp_method>(propValue);
      if (method->set(c,obj,val)) 
        return true;
			else
				CsThrowKnownError(c,CsErrReadOnlyProperty,tag);
		}
    else if(CsPropertyMethodP(propValue)) 
    {
      CsSendMessage(c,obj,propValue,1, val );
      return true;
    }
    //if(CsPropertyIsConst(p))
		//  CsThrowKnownError(c,CsErrReadOnlyProperty,tag);

	}
  return false;
}


/* CONTANT 

// Constant handlers 
static bool  ConstantPrint(VM *c,value val,stream *s, bool toLocale);
static long  ConstantSize(value obj);
static value ConstantCopy(VM *c,value obj);

// Constant pdispatch 
dispatch CsConstantDispatch = {
    "Constant",
    &CsConstantDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    ConstantPrint,
    ConstantSize,
    ConstantCopy,
    CsDefaultScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

// ConstantPrint - VPMethod print handler 
static bool ConstantPrint(VM *c,value val,stream *s, bool toLocale)
{
    return s->put_str("[constant ")
        && s->put_str(CsConstantName(val))
        && s->put(':')
         && CsPrint(c,CsConstantValue(val),s)
        && s->put(']');
}

// ConstantSize - VPMethod size handler 
static long ConstantSize(value obj)
{
    return sizeof( value );
}

// ConstantCopy - VPMethod copy handler 
static value ConstantCopy(VM *c,value obj)
{
    return obj;
}

*/








}

