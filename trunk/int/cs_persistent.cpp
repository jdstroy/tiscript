/*
        Copyright (c) 2006 Alexei Marinets marinets@gmail.com
        All rights reserved
*/

#include "cs.h"
#include "dybase.h"

#ifdef _DEBUG
#define trace c->standardOutput->printf
#else
#define trace
#endif

//#pragma optimize( "", off )

namespace tis 
{

// global Err messages
char* strErrIndexInit = "Index is not initialized";


class DbIndexData
{
public:
  iterator_t  iterator;
  db_triplet  start;
  db_triplet  end;
  bool        asc;
  bool        startInclusive;
  bool        endInclusive;

  DbIndexData() 
    : iterator(NULL), asc(true), startInclusive(true), endInclusive(true)
      {}

  ~DbIndexData()
  { 
    if( iterator )  { dybase_free_index_iterator(iterator); }
  }

  void operator=(DbIndexData* data)
  { 
    assert(data);
    start = data->start;
    end = data->end;
    asc = data->asc;
    startInclusive = data->startInclusive;
    endInclusive = data->endInclusive;
  }
} ;



bool CsDbIndexP(VM* c, value obj) 
{ 
  return CsIsType(obj, c->dbIndexDispatch );
}


/* CsMakeDbIndex - make a new DbIndex value */
value CsMakeDbIndex(VM *c, value vs, oid_t oid)
{
  DbIndexData* data = new DbIndexData();
  value obj = CsMakeCPtrObject(c, c->dbIndexDispatch, (void*)data);

  ptr<persistent_header>(obj)->oid = oid;
  ptr<persistent_header>(obj)->vstorage = vs;
  ptr<persistent_header>(obj)->loaded(true);

  storage* pst = (storage*)CsCObjectValue(vs);

  pst->InsertInHash(oid, obj);
  return obj;
}


/* slice like functionlity for DB Index
 * the functionality is identical to the Search method:
 *    var iter = index.select(minVal, maxVal[, ascent, startInclusive, endInclusive]);  
      for(var i in iter)
      { ... }
 */
value CsDbIndexSlice(VM* c, value obj, value start, value end, bool ascent, bool startInclusive, bool endInclusive)
{
  assert( CsDbIndexP(c, obj) );

  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  value vs = ptr<persistent_header>(obj)->vstorage;
	storage* s = (storage*)CsCObjectValue(vs);
  oid_t oidIdx = ptr<persistent_header>(obj)->oid;

  // create new object (a.k.a. iterator)
  value retObj = CsMakeDbIndex(c, vs, oidIdx);
  DbIndexData* data = (DbIndexData*)CsCObjectValue(retObj);
  assert(data);

  data->asc = ascent;
  data->startInclusive = startInclusive;
  data->endInclusive = endInclusive;
  Transform(c, vs, start, data->start);
  Transform(c, vs, end, data->end);
  if(data->start.type != data->end.type)
  { CsThrowKnownError(c, CsErrPersistError, "Min and max keys are of different types"); }


  return retObj;
}



/* 'dbIndex' pdispatch */

/* method handlers */
static value CSF_dbindexRemove(VM *c);
static value CSF_dbindexSelect(VM *c);
static value CSF_dbindexNext(VM *c);
static value CSF_dbindexAdd(VM *c);
static value CSF_get_length(VM *c, value obj);
static void CSF_set_asc(VM *c, value obj, value val);
static value CSF_dbindexFind(VM *c);

static void DestroyDbIndex(VM *c,value obj);
static value DbIndexGetItem(VM *c,value obj,value tag);
static void  DbIndexSetItem(VM *c,value obj,value tag,value val);
static void _DbIndexSetItem(VM *c, value obj, value tag, value val, bool replace);
static value DbIndexGetNextElement(VM *c, value* index, value obj);


/* Storage methods */
static c_method methods[] = 
{
    C_METHOD_ENTRY( "remove",       CSF_dbindexRemove     ),
    C_METHOD_ENTRY( "select",       CSF_dbindexSelect     ), // search criteria, similar to SLICE
    C_METHOD_ENTRY( "next",         CSF_dbindexNext       ), // Next element in the iterator
    C_METHOD_ENTRY( "add",          CSF_dbindexAdd        ),
    C_METHOD_ENTRY( "find",         CSF_dbindexFind       ), // search in the index

    C_METHOD_ENTRY(	0,              0                     )
};

/* storage properties */
static vp_method properties[] = 
{
    VP_METHOD_ENTRY( "length",      CSF_get_length,   0           ),
    VP_METHOD_ENTRY( "asc",         0,                CSF_set_asc ),
    VP_METHOD_ENTRY( 0,             0,					      0	          )
};


/* CsInitRegExp - initialize the 'DbIndex' obj */
void CsInitDbIndex(VM *c)
{
    /* create the 'DbIndex' type */
    if (!(c->dbIndexDispatch = CsEnterCPtrObjectType(CsGlobalScope(c), NULL, "Index", methods, properties)))
        CsInsufficientMemory(c);

    /* setup alternate handlers */
    c->dbIndexDispatch->destroy = DestroyDbIndex;
    c->dbIndexDispatch->getItem = DbIndexGetItem;
    c->dbIndexDispatch->setItem = DbIndexSetItem;
    c->dbIndexDispatch->getNextElement = DbIndexGetNextElement;
}


// index.remove( key[,value] );
value CSF_dbindexRemove(VM *c)
{
  value obj;
  value key;
  value val = 0;
  oid_t oidVal = 0;
  CsParseArguments(c, "V=*V|V", &obj, c->dbIndexDispatch, &key, &val);

  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  // todo: verify error type
  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  value vs = ptr<persistent_header>(obj)->vstorage;
	storage* s = (storage*)CsCObjectValue(vs);
  oid_t oidIdx = ptr<persistent_header>(obj)->oid;

  // transform 'key' into triplet
  db_triplet db_key;
  Transform(c, vs, key, db_key);
  if(val && CsIsPersistent(c, val) )
  {
    oidVal = ptr<persistent_header>(val)->oid;
  }
  int ret = dybase_remove_from_index( s->dbS, oidIdx, 
        (db_key.type == dybase_string_type) ? (void*)db_key.data.s : &db_key.data, db_key.type, db_key.len, 
        oidVal);

  return (ret ? c->trueValue : c->falseValue); 
}

// usage:
//    @param: asc - optional
//    var iter = index.select(minVal, maxVal[, true|false, true|false, true|false]); 
//
//    for( var obj in iter ) { ... }
value CSF_dbindexSelect(VM *c)
{
  value obj;
  value keyMin;
  value keyMax;
  bool ascent = true;
  bool startInclusive = true;
  bool endInclusive = true;
  CsParseArguments(c, "V=*VV|B|B|B", &obj, c->dbIndexDispatch, &keyMin, &keyMax, &ascent, &startInclusive, &endInclusive);

  return CsDbIndexSlice(c, obj, keyMin, keyMax, ascent, startInclusive, endInclusive );
}

// usage:
//    var iter = index.search(minVal, maxVal[, true|false]); 
//    for( var obj in iter ) { ... }
//  return nothingValue when the end is reached
value DbIndexGetNextElement(VM *c, value* index, value obj)
{
  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  DbIndexData* data = NULL;
  value vs = ptr<persistent_header>(obj)->vstorage;

  if( *index == c->nothingValue ) // first
  {
    DbIndexData* dataObj = (DbIndexData*)CsCObjectValue(obj);
    assert(dataObj);

	  storage* s = (storage*)CsCObjectValue(vs);
    oid_t oidIdx = ptr<persistent_header>(obj)->oid;

    *index = CsMakeDbIndex( c, vs, oidIdx );
    data = (DbIndexData*)CsCObjectValue(*index);
    assert(data);
    // copy filter
    *data = *dataObj;

    data->iterator = dybase_create_index_iterator( s->dbS, oidIdx, data->start.type, 
          (data->start.type == dybase_string_type) ? (void*)data->start.data.s : &data->start.data, data->start.len, data->startInclusive, 
          (data->end.type == dybase_string_type) ? (void*)data->end.data.s : &data->end.data, data->end.len, data->endInclusive, 
          data->asc);
    assert(data->iterator);
  }
  else
  {
    data = (DbIndexData*)CsCObjectValue(*index);
    assert(data && data->iterator);

    if( !data->iterator )
    { CsThrowKnownError(c, CsErrPersistError, "Iterator is not initialized"); }
    vs = ptr<persistent_header>(*index)->vstorage;
  }

  oid_t oid = dybase_index_iterator_next(data->iterator);
  if(!oid) // end of the iterator
  { return c->nothingValue; }
  
  //AM: very week assumption that the obj in index is of type 'Object'
  //AM: It is not weak now - all adding/insertion methods enforce it to be an object.

  //AF: Alex, why do you need to fetch it here? What if I just need to count objects in the index?
  //AM: to count objects you have 'length' property...")
  return CsFetchObject( c, vs, oid );
}

value CSF_dbindexNext(VM *c)
{
  value obj;
  CsParseArguments(c, "V=*", &obj, c->dbIndexDispatch);

  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  value vs = ptr<persistent_header>(obj)->vstorage;
	storage* s = (storage*)CsCObjectValue(vs);
  oid_t oidIdx = ptr<persistent_header>(obj)->oid;

  if( !data->iterator )
  { 
    assert(false);
    // create iterator on fly                                 // for debug only
    data->iterator = dybase_create_index_iterator( s->dbS, oidIdx, dybase_int_type, 
        NULL, 0, 0, 
        NULL, 0, 0, 
        true);
  }

  oid_t oid = dybase_index_iterator_next(data->iterator);
  if(!oid) // end of the iterator
  { return c->nothingValue; }

  return CsFetchObject( c, vs, oid );
}


/* usage:
      idx.set(tag, val[, true|false]);

*/
value CSF_dbindexAdd(VM *c)
{
  value obj;
  value tag;
  value val;
  bool  replace = false;
  CsParseArguments(c, "V=*VV|B", &obj, c->dbIndexDispatch, &tag, &val, &replace);

//#pragma TODO("I think that we need to enforce val here to be the of type 'Object'... no?")
  if( !CsObjectP( val ) )
    CsThrowKnownError(c, CsErrUnexpectedTypeError, val, "Index can hold only Objects");

  _DbIndexSetItem(c, obj, tag, val, replace);
  return c->trueValue;
}


/* usage:
    var arr = idx.find(what, where);
      what  - criteria of search
      where - property name of the object where to search

      returns array of objs

    Note: applies to current select criteria, see class DbIndexData.
*/
value CSF_dbindexFind(VM *c)
{
  value obj;
  value what;
  value where;
  CsParseArguments(c, "V=*VV", &obj, c->dbIndexDispatch, &what, &where);

#pragma TODO("CSF_dbindexFind is not implemented yet")
  CsThrowKnownError(c, CsErrPersistError, "Not implemented yet.");

  return c->nullValue;
}


value CSF_get_length(VM *c, value obj)
{
  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  value vs = ptr<persistent_header>(obj)->vstorage;
	storage* s = (storage*)CsCObjectValue(vs);
  assert(s);
  oid_t oidIdx = ptr<persistent_header>(obj)->oid;
  assert(oidIdx);

  dybase_oid_t* selected_objects = NULL;
  int_t numSelected = dybase_index_search( s->dbS, oidIdx, data->start.type, 
          (data->start.type == dybase_string_type) ? (void*)data->start.data.s : &data->start.data, data->start.len, 1, 
          (data->end.type == dybase_string_type) ? (void*)data->end.data.s : &data->end.data, data->end.len, 1,
          &selected_objects);

  dybase_free_selection(s->dbS, selected_objects, numSelected);

  return CsMakeInteger( numSelected );
}

static void CSF_set_asc(VM *c, value obj, value val)
{
  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  if( !CsBooleanP(c, val) )
  { CsThrowKnownError(c, CsErrPersistError, "Boolean expected"); }

  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  data->asc = CsTrueP(c, val);
}

void DestroyDbIndex(VM *c, value obj)
{
    DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
    assert(data);

    if(data->iterator)
    { 
      dybase_free_index_iterator(data->iterator); 
      data->iterator = NULL;
    }

    delete data;
    CsSetCObjectValue(obj, 0);
}

value DbIndexGetItem(VM *c, value obj, value tag)
{
  value key = tag;

  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  if( !CsIsPersistent(c, obj) )
  { CsThrowKnownError(c, CsErrPersistError, strErrIndexInit); }

  value vs = ptr<persistent_header>(obj)->vstorage;
	storage* s = (storage*)CsCObjectValue(vs);
  oid_t oidIdx = ptr<persistent_header>(obj)->oid;

  db_triplet db_key;
  Transform(c, vs, key, db_key);
  dybase_oid_t* selected_objects = NULL;

  int numSelected = dybase_index_search( s->dbS, oidIdx, db_key.type, 
      (db_key.type == dybase_string_type) ? (void*)db_key.data.s : &db_key.data, db_key.len, 1 /*min_key_inclusive*/,
      (db_key.type == dybase_string_type) ? (void*)db_key.data.s : &db_key.data, db_key.len, 1 /*min_key_inclusive*/,
      &selected_objects);

  if(numSelected)
  { 
    // return the 1st element
    value val = CsFetchObject( c, vs, *selected_objects );
    // free selected array
    dybase_free_selection(s->dbS, selected_objects, numSelected);
    return val;
  }
  else
  { 
    return c->undefinedValue; 
  }
}

void _DbIndexSetItem(VM *c, value obj, value tag, value val, bool replace)
{
  DbIndexData* data = (DbIndexData*)CsCObjectValue(obj);
  assert(data);

  if( !CsObjectP(val) )
    CsThrowKnownError(c, CsErrUnexpectedTypeError, val, "instance of object");

  /* --- save 'obj' in storage --- */

  value vs = ptr<persistent_header>(obj)->vstorage;  
	storage* s = (storage*)CsCObjectValue(vs);
  //printf("_DbIndexSetItem st=%x %d",s, s->hashS.size());

  oid_t oidIdx = ptr<persistent_header>(obj)->oid;

  oid_t oidObj = CsStoreObject( c, vs, val );

  // transform 'key' into triplet
  db_triplet db_key;
  Transform(c, vs, tag, db_key);
  int ret = dybase_insert_in_index( s->dbS, oidIdx, (db_key.type == dybase_string_type) ? (void*)db_key.data.s : &db_key.data, db_key.type, db_key.len, oidObj, replace);
  if(!ret)
    CsThrowKnownError(c, CsErrPersistError, "Cannot insert object into index"); 

//  dybase_commit(s->dbS);

}

void DbIndexSetItem(VM *c, value obj, value tag, value val)
{
  _DbIndexSetItem(c, obj, tag, val, true);
  return;
}

}

//#pragma optimize( "", on )
