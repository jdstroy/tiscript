/*
        Copyright (c) 2006 Alexei Marinets marinets@gmail.com
        All rights reserved
*/

#include "cs.h"
#include "dybase.h"
#include "tl_value.h"
#include "tl_hash.h"

#ifdef _DEBUG
#define trace   c->standardOutput->printf
#else
#define trace
#endif

//#pragma optimize( "", off )

namespace tis
{

char* strErrCantSaveObj = "Can't save object.";
char* strErrCorruptPersistent = "Object is corrupted.";


/* 'storage' pdispatch */

/* method handlers */
static value CSF_open(VM *c);
static value CSF_close(VM *c);
static value CSF_commit(VM *c);
static value CSF_rollback(VM *c);
static value CSF_removeObject(VM *c);
static value CSF_createIndex(VM *c);
static value CSF_registerClass(VM *c);
static value CSF_get_root(VM *c, value obj);
static void  CSF_set_root(VM *c, value obj, value val);
static value CSF_get_autocommit(VM *c, value obj);

/* Storage methods */
static c_method methods[] = {
//  C_METHOD_ENTRY( "this",      CSF_ctor            ),
C_METHOD_ENTRY( "open",             CSF_open            ),
C_METHOD_ENTRY( "close",            CSF_close           ),
C_METHOD_ENTRY( "commit",           CSF_commit          ),
C_METHOD_ENTRY( "rollback",         CSF_rollback        ),

C_METHOD_ENTRY( "deallocateObject", CSF_removeObject),
C_METHOD_ENTRY( "createIndex",      CSF_createIndex     ),

/* bonus features */
//C_METHOD_ENTRY( "registerClass",    CSF_registerClass   ),

C_METHOD_ENTRY( 0,                  0                   )
};

/* storage properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "root",        CSF_get_root,         CSF_set_root        ),
VP_METHOD_ENTRY( "autocommit",  CSF_get_autocommit,   0                   ),
VP_METHOD_ENTRY( 0,             0,                    0                   )
};


#define dybase_date_type      dybase_long_type

/* prototypes */
class storage;
/* returns true if obj is persistent */
/* persistent objects:
    - object
    - vector
    - dbIndex
*/

//typedef dybase_storage_t storage;
void DestroyStorage(VM *c,value obj);


static value FetchValue( VM *c, value vs, dybase_handle_t h );
static void  StoreValue( VM *c, value vs, dybase_handle_t h, value v );
tool::string FetchClassName(storage* s, oid_t oid);

/* --- external interfaces --- */
/* check for initialized persistent object,
i.e. there is a storage assosiated with this obj */
bool CsIsPersistInit(VM *c, value obj)
{ return (CsPersistentP(c, obj) && ptr<persistent_header>(obj)->vstorage); }


/* restore persistent obj from an assosiated storage */
value CsRestoreObj( VM *c, value obj )
{
  if( !CsIsPersistInit(c, obj) )
  { return FALSE_VALUE; }

  value vs = ptr<persistent_header>(obj)->vstorage;
  assert( vs );
  //return ReloadObj( c, vs, obj );
  CsThrowKnownError(c, CsErrPersistError, "'restore' method is not implemented yet");
//#pragma TODO("do reload!")
  return FALSE_VALUE;
}

/* storage */
storage::~storage()
{
  DetachAllObjs(0);
  if( dbS )
  { dybase_close( dbS ); }

  hashNameProto.clear();
}

  // remove obj from hash
void storage::DetachObj( dybase_oid_t oid )
{
  this->resetPersistHdr( this->hashS[oid] );
  this->hashS.remove(oid);
}


void storage::CommitHash(VM* c)
{
  if(!this->hashS.size() || !c)
    return;

  // make snapshot of the hash, this is needed
  // because this->hashS can be changed (indirectly) by CsStoreObjectData
  // and CsStoreVectorData functions

  tool::array<value> objs = this->hashS.elements();
  for( int n = objs.last_index(); n >= 0 ; --n)
    {
      value obj = objs[n];
      if( CsObjectP(obj) )
        CsStoreObjectData(c,obj);
      else if( CsVectorP(obj) )
        CsStoreVectorData(c,obj);
      else if( CsMovedVectorP(obj) )
        CsStoreVectorData(c,obj);
      else if( CsDbIndexP(c, obj) )
        continue; //?
    }
  //assert( objs.size() == this->hashS.size() );
}

void storage::DetachAllObjs(VM* c)
{
  if(!this->hashS.size())
    return;

  CommitHash(c);

  tool::array<value>& objs = this->hashS.elements();

  for( int_t i = 0; i < objs.size(); i++)
    resetPersistHdr( objs[i] );

  hashS.clear();
}

// set persistent obj attributes to 0:0 (oid:storage)
void storage::resetPersistHdr( value& obj )
{
  ptr<persistent_header>(obj)->oid = 0;
  ptr<persistent_header>(obj)->vstorage = 0;
  ptr<persistent_header>(obj)->status = 0;
}


/* return object from storage hash or '0' if not present */
value storage::GetFromHash( oid_t oid )
{
  value v = 0; this->hashS.find(oid,v); return v;
}

/* insert 'obj' object into storage hash */
void storage::InsertInHash( oid_t oid, value obj )
{
  //Why you need this?
  //       this->hashS.get_index ( oid, true );
  assert(!CsBrokenHeartP(obj));
  this->hashS[oid] = obj;
  //printf("st=%x value=%I64x oid=%x size=%d\n", this, obj, oid, this->hashS.size());

}

tool::string storage::GetNameByProto(VM* c, value proto)
{
  tool::string str = CsClassClassName(c,proto);
  hashNameProto[str] = proto;
  return str;
}

/* end of storage */

/* db_tripletTag */
db_triplet::db_triplet()
{
  data.s = 0;
  type = dybase_string_type;
  len = 0;
}
db_triplet::~db_triplet()
{
  if( dybase_string_type == type && data.s )   // dybase_string_type
  { delete[] data.s; }
}

void db_triplet::operator=(db_triplet& obj)
{
  type = obj.type;
  len = obj.len;
  if(dybase_string_type == type && len && obj.data.s) // not null string
  {
    data.s = new byte[len];
    ::memcpy( data.s, obj.data.s, len );
  }
  else
  { data = obj.data; }
}

bool db_triplet::is_null() const 
{ 
  return type == dybase_string_type && data.s == 0; 
}

/* end of db_triplet */


/* gc is about to run
  commit hash to storage...
*/
void StoragePreGC(VM* c, value vs )
{
  storage* s = (storage*)CsCObjectValue(vs);
  assert(s && s->dbS);

  if(s->autocommit)
  {
    s->CommitHash(c);
    dybase_commit( s->dbS );
  }

}

/* gc completed =>
  cleanup storage hash
*/
void StoragePostGC(VM* c, value vs)
{
  storage* s = (storage*)CsCObjectValue(vs);
  assert(s && s->dbS);

  trace(L"Original Total number of objects in hash: %ld\n", s->hashS.size() );

  //if( !s->hashS.size() )
  //   return;

  int i = 0;

  // maintenance of hashNameProto
  for(i = s->hashNameProto.size() - 1; i >= 0 ; i--)
    s->hashNameProto(i) = CsCopyValue(c, s->hashNameProto(i) );

  storageHash newHashT;

  int n_toremove = 0;
  for(i = s->hashS.size() - 1; i >= 0 ; i--)
  {
    value obj = s->hashS(i);
    if( CsBrokenHeartP(obj) ) // if it was copied into new half so it is used by someone.
    {
      value nobj = CsBrokenHeartForwardingAddr(obj);
      assert(!CsBrokenHeartP(nobj));

      persistent_header* oph = ptr<persistent_header>( obj );
      persistent_header* nph = ptr<persistent_header>( nobj );

      nph->oid = oph->oid;
      nph->vstorage = vs;

      assert(oph->oid);

      newHashT[nph->oid] = nobj;
//trace(L"Broken heart: oid 0x%x\n", oid);
    }
  }

  tool::swap(newHashT, s->hashS);

  trace(L"Total number of objects in storage hash: %ld\n", s->hashS.size() );



}

bool IsEmptyStorage(value vs)
{
    storage* s = (storage*)CsCObjectValue(vs);
    return (0 == s->hashS.size());
}


/* globals */


// external interface
/*void detachObjFromStorage( value& obj )
{
  if( CsPersistentP(obj) && ptr<persistent_header>(obj)->vstorage )
  {
    assert( ptr<persistent_header>(obj)->oid );
    storage* s = (storage*)CsCObjectValue(ptr<persistent_header>(obj)->vstorage);
    assert( s );
    s->DetachObj(ptr<persistent_header>(obj)->oid );
  }
}
*/


/* error handler */
void errHandler(int error_code, char const* msg)
{
printf( "DyBase error: %d - '%s'\n", error_code, msg );
  //CsThrowKnownError(c, CsErrPersistError, msg);
}

value CsMakeStorage(VM* c, storage* s)
{
  return CsMakeCPtrObject(c, c->storageDispatch, s);
}


/* CsInitStorage - initialize the 'persistent' obj */
void CsInitStorage(VM *c)
{
  // create the 'Storage' type
  if (!(c->storageDispatch = CsEnterCPtrObjectType(CsGlobalScope(c), NULL, "Storage", methods, properties)))
  { CsInsufficientMemory(c); }

  // setup alternate handlers
  c->storageDispatch->destroy = DestroyStorage;
        c->storageDispatch->baseType = &CsCObjectDispatch;
}


/* usage:
    var s = storage.open(FileName[, autocommit])
*/
static value CSF_open(VM *c)
{
  tool::wchars wfname;
  bool autocommit = true;
  CsParseArguments(c, "**S#|B", &wfname.start, &wfname.length, &autocommit);

  storage* s = new storage();
  assert(s);
  s->autocommit = autocommit;

  if(tool::is_like(wfname,L"file://*"))
    wfname.prune(7);

  if (!(s->dbS = dybase_open( wfname.start, 4*1024*1024, errHandler )))
  {
    delete s;
    return NULL_VALUE;
  }

  dybase_gc(s->dbS);

  value vs = CsMakeCPtrObject(c, c->storageDispatch, s);

  /* save new storage in VM::storages */
  c->storages.push(vs);
  return vs;
}

/* close storage
  and commit all attached objects beforehand
*/
static value CSF_close(VM *c)
{
  value val;
  CsParseArguments(c, "V=*", &val, c->storageDispatch);
  storage* s = (storage*)CsCObjectValue(val);
  if(!s || !s->dbS) { return FALSE_VALUE; }

  if(s->autocommit)
    s->CommitHash(c);
  s->DetachAllObjs(c);

  // remove storage from the VM array
  int_t idx = c->storages.get_index(val);
  if( idx >= 0 )
  { c->storages.remove(idx); }

  /* storage will be closed in destructor */
  delete s;
  s = NULL;
  trace( L"Storage closed\n" );

  CsSetCObjectValue(val, 0);
  return TRUE_VALUE;
}


/* Close storage and destroy it
  NO storage hash commit */
void DestroyStorage(VM *c, value obj)
{
  storage* s = (storage*)CsCObjectValue(obj);

  if(s->autocommit)
    s->CommitHash(c);

  /* storage will be closed in destructor */
  // remove storage from the VM array
  int_t idx = c->storages.get_index(obj);
  if( idx >= 0 )
  { c->storages.remove(idx); }

  delete s;
  s = NULL;

  CsSetCObjectValue(obj, 0);
  trace( L" Storage destroyed\n" );
}


/* commit storage hash
  & commit data to storage
*/
static value CSF_commit(VM *c)
{
  value val;
  storage* s;
  CsParseArguments(c, "V=*", &val, c->storageDispatch);
  s = (storage*)CsCObjectValue(val);
  if(!s || !s->dbS) { return FALSE_VALUE; }

  s->CommitHash(c);

  dybase_commit( s->dbS );

  trace( L"Storage commit\n" );

  return TRUE_VALUE;
}

// clear all objects from storage cache
// => for all subsequent calls, objects will be loaded from Storage.
// Note: loaded / saved objects will retain oids and ref to the parent storage.
static value CSF_rollback(VM *c)
{
  value val;
  storage* s;
  CsParseArguments(c, "V=*", &val, c->storageDispatch);
  s = (storage*)CsCObjectValue(val);
  if(!s || !s->dbS) { return FALSE_VALUE; }

  s->hashS.clear();

//  dybase_rollback( s->dbS );
trace( L"Storage rollback\n" );
  return TRUE_VALUE;
}

/* return 'autocommit' property */
static value CSF_get_autocommit(VM *c, value vs)
{
  storage* s = (storage*)CsCObjectValue(vs);
  if(!s || !s->dbS)
  {
    CsThrowKnownError(c, CsErrPersistError, strErrCorruptPersistent);
    return FALSE_VALUE;
  }

  return CsMakeBoolean( c, s->autocommit);
}

/* set 'autocommit' property */
/*static void CSF_set_autocommit(VM *c, value vs, value val)
{
  storage* s = (storage*)CsCObjectValue(vs);
  if(!s || !s->dbS)
    CsThrowKnownError(c, CsErrPersistError, strErrCorruptPersistent);

  s->autocommit = (CsBooleanP(c, val) && CsTrueP(c, val));
}*/

static value CSF_removeObject(VM *c)
{
  value obj;
  value val;
  storage* s;
  CsParseArguments(c, "V=*V", &obj, c->storageDispatch, &val);
  s = (storage*)CsCObjectValue(obj);
  if(!s || !s->dbS) { return FALSE_VALUE; }

  if( !CsIsPersistInit(c, val) )
  { return FALSE_VALUE; }

  value vsVal = ptr<persistent_header>(val)->vstorage;
  storage* sVal = (storage*)CsCObjectValue(vsVal);
  oid_t oidVal = ptr<persistent_header>(val)->oid;

  if( sVal == s )
  {
    dybase_deallocate_object(s->dbS, oidVal);
    s->hashS.remove(oidVal);
    return TRUE_VALUE;
  }

  return FALSE_VALUE;
}

/*
  return DbIndex object OR nullValue if can't create an Index
Usage:
  var intIndex = Storage.createIndex( #integer [,unique] );
*/
static value CSF_createIndex(VM *c)
{
  value val;
  storage* s;
  int  typeSym = 0;
  bool unique = true;
  CsParseArguments(c, "V=*L|B", &val, c->storageDispatch, &typeSym, &unique);
  s = (storage*)CsCObjectValue(val);
  if(!s || !s->dbS) { return NULL_VALUE; }

  dybase_oid_t oidIdx = 0;
  switch(typeSym)
  {
    case S_COLOR:
      oidIdx = dybase_create_index(s->dbS, dybase_color_type, unique);
      break;

    case S_INTEGER:
      oidIdx = dybase_create_index(s->dbS, dybase_int_type, unique);
      break;

    case S_FLOAT:
      oidIdx = dybase_create_index(s->dbS, dybase_real_type, unique);
      break;

    case S_DATE:
      oidIdx = dybase_create_index(s->dbS, dybase_date_type, unique);
      break;

    case S_STRING:
      oidIdx = dybase_create_index(s->dbS, dybase_string_type, unique);
      break;

    default:
      CsThrowKnownError(c, CsErrUnexpectedTypeError, "supported types: int, float, string");
      break;
  }

  value obj = CsMakeDbIndex( c, val, oidIdx );

  return obj;
}


/*
  usage:
      s.registerClass( name, prototype );
  where:
      name        - is a string representation of a prototype
      prototype   - is a prototype of a persistent object
*/

/*static value CSF_registerClass(VM *c)
{
  value val;
  storage* s;
  wchar* protoName = NULL;
  value  proto;
  CsParseArguments(c, "V=*SV", &val, c->storageDispatch, &protoName, &proto);

  s = (storage*)CsCObjectValue(val);
  if(!s || !s->dbS || !protoName) { return FALSE_VALUE; }

  // add proto name as the obj property
  CsSetObjectPropertyNoLoad( c, proto, symbol_value( _class ), CsMakeCString(c,protoName) );
  s->hashNameProto[protoName] = proto;

  return TRUE_VALUE;
}*/

static value CSF_get_root(VM* c, value vs)
{
  storage* s = (storage*)CsCObjectValue(vs);
  if(!s || !s->dbS)
  {
    CsThrowKnownError(c, CsErrPersistError, strErrCorruptPersistent);
    return NULL_VALUE;
  }

  dybase_oid_t oid = dybase_get_root_object(s->dbS);
  if( !oid )
        return NULL_VALUE;

  return CsFetchObject(c,vs,oid);

  //value obj = CsMakeObject(c,UNDEFINED_VALUE);
  //ptr<persistent_header>(obj)->oid = oid;
  //ptr<persistent_header>(obj)->vstorage = vs;
  //ptr<persistent_header>(obj)->loaded(false); // clear loaded flag
  //return obj;
}

tool::string FetchClassName(storage* s, oid_t oid)
{
  assert(s && s->dbS);

  dybase_handle_t h = dybase_begin_load_object(s->dbS, oid);
  assert(h);

  char* strClassName = dybase_get_class_name(h);
  tool::string className = strClassName;

  dybase_end_load_object(h);

  return className;
}

value CsFetchObject( VM *c, value vs, oid_t oid )
{
    storage* s = (storage*)CsCObjectValue(vs);
    assert(s);

    value obj = 0;

    if( s->IsInHash(oid,obj) )
    {
//      trace( L"\nobj returned from storage hash, oid = 0x%x\n", oid );
      return obj;
    }

    tool::string className = FetchClassName(s, oid);
    value proto = UNDEFINED_VALUE;
    if(className.length())
    {
      proto = s->GetProtoByName(c,className);
    }
    if(proto == UNDEFINED_VALUE) proto = c->objectObject;

    CsPush(c,vs);
    obj = CsMakeObject( c, proto );
    vs = CsPop(c);

    // delayed obj loading
    ptr<persistent_header>(obj)->oid = oid;
    ptr<persistent_header>(obj)->vstorage = vs;
    ptr<persistent_header>(obj)->loaded(false); // clear loaded flag
    ptr<persistent_header>(obj)->modified(false); // clear modified flag

    s->InsertInHash(oid, obj);

    return obj;
}

value CsFetchVector( VM *c, value vs, dybase_oid_t oid )
{
    storage* s = (storage*)CsCObjectValue(vs);
    assert(s);

    value vec = 0;

    if( s->IsInHash(oid,vec) )
      return vec;

    // load length for the vector
    dybase_handle_t h = dybase_begin_load_object(s->dbS, oid);
    assert(h);

    char* className = dybase_get_class_name(h);
    if( !className )
    {
      assert(false);
      return FALSE_VALUE;
    }

    char* fieldName = dybase_next_field(h);
    if( !fieldName )
    {
      assert(false);
      return FALSE_VALUE;
    }

    int_t type;
    void* value_ptr = NULL;
    int_t value_length = 0;
    dybase_get_value(h, &type, &value_ptr, &value_length);
    dybase_end_load_object(h);

    CsPush(c,vs);
    vec = CsMakeVector(c, value_length);
    vs = CsPop(c);
    assert(oid);

    ptr<persistent_header>(vec)->oid = oid;
    ptr<persistent_header>(vec)->vstorage = vs;
    ptr<persistent_header>(vec)->loaded(false); // clear loaded flag
    ptr<persistent_header>(vec)->modified(false); // clear modified flag

    s->InsertInHash(oid, vec);

    return vec;
}

extern bool CsSetObjectPersistentProperty(VM *c,value theobj,value tag,value val);

// fetch object data
value  CsFetchObjectData( VM *c, value theobj )
{
  if(ptr<persistent_header>(theobj)->loaded())
    return theobj; // already loaded, nothing to do.

  //dispatch* pd = CsGetDispatch(obj);

  pvalue obj(c,theobj);
  pvalue vs(c);

  dybase_oid_t  oid = ptr<persistent_header>(obj)->oid;
                vs = ptr<persistent_header>(obj)->vstorage;
  storage*      s = (storage*)CsCObjectValue(vs);
//  assert(s && s->dbS);

  dybase_handle_t h = 0;
  try
  {
    h = dybase_begin_load_object(s->dbS, oid);
    assert(h);
  }
  catch(...)
  {
    CsThrowKnownError(c, CsErrPersistError, strErrCorruptPersistent);
  }

  char* className = dybase_get_class_name(h);
  if( !className )
  {
    assert(false);
    return obj;
  }

  char* fieldName = dybase_next_field(h);
  if( !fieldName )
  {
    assert(false);
    return obj;
  }

  int_t type;
  void* value_ptr = NULL;
  int_t value_length = 0;
  dybase_get_value(h, &type, &value_ptr, &value_length);

  assert( type == dybase_map_type );

  for( int_t i = 0; i < value_length; i++ )
  {
      dybase_next_element( h );
      value key = FetchValue( c, vs, h );
      dybase_next_element( h );
      CsPush(c,key);
        value val = FetchValue( c, vs, h );
      key = CsPop(c);
      CsSetObjectPersistentProperty( c, obj, key, val );
  }

  dybase_end_load_object(h);

  ptr<persistent_header>(obj)->loaded(true);
  ptr<persistent_header>(obj)->modified(false); // BUT NOT MODIFIED!

  return obj;

}

value  CsFetchVectorData( VM *c, value theobj )
{
  if(ptr<persistent_header>(theobj)->loaded())
    return theobj; // already loaded, nothing to do.

  pvalue obj(c,theobj);
  pvalue vs(c);

  dybase_oid_t  oid = ptr<persistent_header>(obj)->oid;
                vs = ptr<persistent_header>(obj)->vstorage;
  storage*      s = (storage*)CsCObjectValue(vs);

  assert(oid);

  dybase_handle_t h = dybase_begin_load_object(s->dbS, oid);
  assert(h);

  char* className = dybase_get_class_name(h);
  if( !className )
  {
    assert(false);
    return obj;
  }

  char* fieldName = dybase_next_field(h);
  if( !fieldName )
  {
    assert(false);
    return obj;
  }

  int_t type;
  void* value_ptr = NULL;
  int_t value_length = 0;
  dybase_get_value(h, &type, &value_ptr, &value_length);

  assert( type == dybase_array_type );

  CsCheck(c,1);
  obj = CsResizeVectorNoLoad(c,obj,value_length);

  for( int_t i = 0; i < value_length; i++ )
  {
    dybase_next_element( h );
    value val = FetchValue( c, vs, h );
    CsSetVectorElementNoLoad(c, obj,i,val);
  }

  dybase_end_load_object(h);

  ptr<persistent_header>(obj)->loaded(true);
  ptr<persistent_header>(obj)->modified(false); // BUT NOT MODIFIED!

  return obj;
}

value FetchValue( VM *c, value vs, dybase_handle_t h )
{

  storage* s = (storage*)CsCObjectValue(vs);

  int_t type;
  void* value_ptr = NULL;
  int_t value_length = 0;
  dybase_get_value(h, &type, &value_ptr, &value_length);

  switch(type)
  {
  case dybase_object_ref_type:
    {
      dybase_oid_t oid = *((dybase_oid_t*)value_ptr);
      return CsFetchObject(c, vs, oid);
    }
  case dybase_array_ref_type:
    {
      dybase_oid_t oid = *((dybase_oid_t*)value_ptr);
      return CsFetchVector(c, vs, oid);
    }
  case dybase_index_ref_type:
    {
      dybase_oid_t oid = *((dybase_oid_t*)value_ptr);
/* AM: it breaks reading of index from storage */
      if( !s->IsHashEmpty() && s->IsInHash(oid) )
      {
        trace( L"\nDbIndex obj returned from storage hash, oid = 0x%x\n", oid );
        return s->GetFromHash(oid);
      }
      return CsMakeDbIndex(c, vs, oid);
    }
  case dybase_bool_type:
    return CsMakeBoolean(c, *((bool*)value_ptr) );

  case dybase_color_type:
    return CsMakeColor(*((int_t*)value_ptr));

  case dybase_int_type:
    return CsMakeInteger(*((int_t*)value_ptr));

  case dybase_date_type:
    {
      datetime_t dt = *((datetime_t*)value_ptr);
      return CsMakeDate(c, dt);
    }

  case dybase_real_type:
    return CsMakeFloat(c,*(double*)value_ptr);
//trace( L"long %d - ", *((long*)value_ptr) );

  case dybase_string_type:
    if(!value_length)
      return NULL_VALUE;
    else
    {
      byte strType = *(byte*)value_ptr;
      switch(strType)
      {
      case db_char:
        {
          tool::string str( (char*)value_ptr + 1, value_length - 1);
          return CsMakeSymbol(c,str, str.length());
        }
      case db_wchar:
        return CsMakeCharString( c, (wchar*)((byte*)value_ptr + 1), (value_length-1)/2 );
      case db_blob:
        //assert(false);
        return CsMakeByteVector( c, (byte*)value_ptr + 1, value_length - 1);
        break;
      default:
        assert(false);
        break;
      }
    }
//trace( L"string %s - ", (wchar*)value_ptr );
    break;

  case dybase_map_type:
    // element couldn't be a map
    // map =(by default) to object
    assert(false);
    break;

  default:
    assert(false);
    break;
  }

  return UNDEFINED_VALUE;
}


/* usage:
    storage.root = val;
*/
static void CSF_set_root(VM *c, value vs, value val)
{
  storage* s = (storage*)CsCObjectValue(vs);
  if(!s)
  {
    CsThrowKnownError(c, CsErrPersistError, strErrCorruptPersistent);
    return;
  }

  dybase_oid_t oid = 0;

  if( CsObjectP(val) )
    oid = CsSetPersistent( c, vs, val );
  else if( CsVectorP(val) || CsMovedVectorP(val) )
    oid = CsSetPersistent( c, vs, val );
  //CsVectorForwardingAddr(val)
  else
    CsThrowKnownError(c, CsErrUnexpectedTypeError, "root can be either object or array");

  if( oid )
  {
    dybase_set_root_object(s->dbS, oid);
  }
  else
  { CsThrowKnownError(c, CsErrPersistError, "Can not save root object"); }

  return;
}

dybase_oid_t CsSetPersistent( VM *c, value vs, value obj )
{
  storage* s = (storage*)CsCObjectValue(vs);
  assert( s && s->dbS );

  dybase_oid_t oid;

  if( ptr<persistent_header>(obj)->vstorage == vs )
  {
    oid = ptr<persistent_header>(obj)->oid;
    return oid;
    //if( !ptr<persistent_header>(obj)->modified() // if it is not modified
    // &&  ptr<persistent_header>(obj)->loaded())  // and if it is loaded
    //{
      // as it is already stored, nothing to do
    //}
  }
  else if(ptr<persistent_header>(obj)->vstorage)
  {
      // it is attached to another storage
      CsPush(c,vs);
      if( CsObjectP( obj ))
        obj = CsFetchObjectData(c, obj);
      else if( CsVectorP( obj ) || CsMovedVectorP(obj) )
        obj = CsFetchVectorData(c, obj);
      else 
        assert(false);
      vs = CsPop(c);
  }
  // it is detached
  oid = dybase_allocate_object( s->dbS );

  assert(oid);

  ptr<persistent_header>(obj)->oid = oid;
  ptr<persistent_header>(obj)->vstorage = vs;
  ptr<persistent_header>(obj)->loaded(true); // set loaded flag
  ptr<persistent_header>(obj)->modified(true); // and set modified flag

  s->InsertInHash(oid, obj);

  return oid;

}

dybase_oid_t CsStoreObject( VM *c, value vs, value obj )
{
  assert( CsObjectP(obj) );

  dybase_oid_t oid = CsSetPersistent(c, vs,obj);
  CsStoreObjectData( c, obj );

  return oid;
}

void PersistValue(VM *c, value vs, value obj)
{
  if( CsObjectP(obj) )
    CsStoreObject(c,vs,obj);
  else if( CsVectorP(obj) )
    CsStoreVector(c,vs,obj);
  else if( CsMovedVectorP(obj) )
    CsStoreVector(c,vs,obj);
}

void CsStoreObjectData( VM *c, value obj )
{
  assert( CsObjectP(obj) );

  bool isPersistent = CsIsPersistent(c, obj);
  bool isModified = CsIsModified(obj);

  value vs = ptr<persistent_header>(obj)->vstorage;

  if( isPersistent && !isModified )
  {
    CsPush(c,vs);
    each_property gen(c, obj, false);
    value key,val;
    while(gen(key,val))
    {
      PersistValue(c, vs, key); vs = CsTop(c);
      PersistValue(c, vs, val); vs = CsTop(c);
    }
    CsPop(c);
    return; // done
  }

  storage* s = (storage*)CsCObjectValue(vs);
  assert( s && s->dbS );

  dybase_oid_t oid = ptr<persistent_header>(obj)->oid;

  ptr<persistent_header>(obj)->loaded(true); // set loaded flag
  ptr<persistent_header>(obj)->modified(false); // and clear modified flag

  value proto = CsObjectClass(obj);

  //tool::ustring uProtoName = s->GetNameByProto(c, proto);
  //tool::string str = uProtoName.utf8();
  //const char* strClassName = (uProtoName.length() ? str.c_str() : CsTypeName(obj) );

  tool::string strClassName = s->GetNameByProto(c, proto);

  dybase_handle_t h = dybase_begin_store_object( s->dbS, oid, strClassName );
  assert(h);
  if(!h) { CsThrowKnownError(c, CsErrPersistError, strErrCantSaveObj); }

  int counter = 0;

  
  each_property gen(c, obj, false);
  value key,val;
  while(gen(key,val))
     ++counter;
  
  /*
  struct oscanner_counter: object_scanner
  {
    int counter;
    dybase_handle_t h;
    value vs;
    bool item( VM *c, value key, value val )
    {
      //if( CsSymbolP(key) && CsSymbolName(key)[0] == '_' )
      //  return true; // this is not persistable member.
      ++counter;
      return true; // continue scan
    }
  } oscounter;

  oscounter.counter = 0;
  oscounter.h = h;
  oscounter.vs = vs;
  CsScanObjectNoLoad(c,obj,oscounter);
  */



  // -- store all properties --
  //int countProperties = CsObjectPropertyCount(obj);
  // saving pairs: tag + value

  dybase_store_object_field(h, ".", dybase_map_type, 0, counter);

  // scan all obj properties and store every (key val) pair
  /*
  struct oscanner: object_scanner
  {
    dybase_handle_t h;
    value vs;
    bool item( VM *c, value key, value val )
    {
      //if( CsSymbolP(key) && CsSymbolName(key)[0] == '_' )
      //  return true; // this is not persistable member.
      StoreValue(c, vs, h,key);
      StoreValue(c, vs, h,val);
      return true; // continue scan
    }
  } osc;

  osc.h = h;
  osc.vs = vs;

  CsScanObjectNoLoad(c,obj,osc);
  */

  //if(0)
  while(gen(key,val))
  {
    StoreValue(c, vs, h,key);
    StoreValue(c, vs, h,val);
  }
  
    // end store obj
  dybase_end_store_object(h);

}

dybase_oid_t CsStoreVector( VM *c, value vs, value obj )
{
  assert( CsVectorP(obj) || CsMovedVectorP(obj) );

  dybase_oid_t oid = CsSetPersistent(c,vs,obj);
  CsStoreVectorData( c, obj );

  return oid;
}

void CsStoreVectorData( VM *c, value obj )
{
  dispatch* d = CsGetDispatch(obj);

  value vs = ptr<persistent_header>(obj)->vstorage;

  if( CsIsPersistent(c,obj) && !CsIsModified(obj) )
  {
    CsPush(c,vs);
    int_t v_length = CsVectorSizeNoLoad(c,obj);
    for( int_t i = 0; i < v_length; i++ )
    {
      value vel = CsVectorElementNoLoad(c, obj, i );
      PersistValue( c, vs, vel ); vs = CsTop(c);
    }
    CsPop(c);
    return; 
  }

  const char* type_name = "Array";

#ifdef _DEBUG
  if(CsMovedVectorP(obj))
    type_name = type_name;
#endif

  storage* s = (storage*)CsCObjectValue(vs);
  assert( s && s->dbS );

  dybase_oid_t oid = ptr<persistent_header>(obj)->oid;

  ptr<persistent_header>(obj)->loaded(true); // set loaded flag
  ptr<persistent_header>(obj)->modified(false); // and clear modified flag

  //CsVectorP(obj) || CsMovedVectorP(obj)

  dybase_handle_t h = dybase_begin_store_object( s->dbS, oid, type_name  );
  assert(h);
  if(!h) { CsThrowKnownError(c, CsErrPersistError, strErrCantSaveObj); }

  // -- store all elements --
  int_t v_length = CsVectorSizeNoLoad(c,obj);
  dybase_store_object_field(h, ".", dybase_array_type, 0, v_length);
  for( int_t i = 0; i < v_length; i++ )
  {
    value vel = CsVectorElementNoLoad(c, obj, i );
    StoreValue( c, vs, h, vel );
  }
  dybase_end_store_object(h);

}


void Transform(VM* c, value vs, value val, db_triplet& db_v)
{
//trace( L"Transform: \n" );
  //storage* s = (storage*)CsCObjectValue(vs);

  db_v.len = 0;
  db_v.data.i64 = 0;

  if( val == NULL_VALUE )
  {
    db_v.type = dybase_string_type;
    db_v.len = 0;
    db_v.data.i64 = 0;
  }
  else if( CsBooleanP(c, val) )
  {
    db_v.data.b = CsTrueP( c, val );
    db_v.type = dybase_bool_type;
  }
  else if( CsIntegerP( val ) )
  {
    db_v.data.i = to_int( val );
    db_v.type = dybase_int_type;
    //trace( L" int %d ", db_v.data.i );
  }
  else if( CsSymbolP( val ) )
  {
    tool::string str = CsSymbolName(val);
    db_v.len = 1 + str.length() * sizeof(char); // length in bytes + 1 byte for the type
    db_v.data.s = new byte[db_v.len];
    byte strType = db_char;
    ::memcpy( db_v.data.s, &strType, 1);
    ::memcpy( db_v.data.s + 1, (byte*)str.c_str(), db_v.len - 1);
    db_v.type = dybase_string_type;
  }
  else if( CsFloatP( val ) )
  {
    db_v.data.d = to_float( val );
    db_v.type = dybase_real_type;
//trace( L" float %u ", db_v.data.d );
  }
  else if( CsStringP( val ) )
  {
    //array<byte> utf8;
    //to_utf8(CsStringAddress(val), CsStringSize(val), array<byte>& utf8out)

    db_v.len = 1 + CsStringSize(val) * sizeof(wchar); // length in bytes + 1 byte for the type
    db_v.data.s = new byte[db_v.len];
    byte strType = db_wchar;
    ::memcpy( db_v.data.s, &strType, 1);
    ::memcpy( db_v.data.s + 1, (byte*)CsStringAddress(val), db_v.len - 1);
    db_v.type = dybase_string_type;
//trace( L" string %s ", (wchar*)db_v.data.s );
  }
  else if( CsDateP(c, val) )
  {
    datetime_t ft = CsDateValue(c,val);
    db_v.data.i64 = ft;
    db_v.type = dybase_date_type;
//trace( L" date 0x%lx %lx\n ", ft.dwHighDateTime, ft.dwLowDateTime );

  }
  else if( CsVectorP(val) || CsMovedVectorP(val) )
  {
    db_v.type = dybase_array_ref_type;
    db_v.data.oid = CsStoreVector( c, vs, val );
//    db_v.data.oid = StoreObj( c, vs, val );
//trace( L" vector 0x%x ", db_v.data.oid );
  }
  else if(CsDbIndexP(c, val))
  {
    db_v.type = dybase_index_ref_type;
    db_v.data.oid = ptr<persistent_header>(val)->oid;
  }
  else if(CsObjectP(val))
  {
    db_v.type = dybase_object_ref_type;
    db_v.data.oid = CsStoreObject( c, vs, val );
//trace( L" obj 0x%x ", db_v.data.oid );
  }
  else if(CsByteVectorP(val))
  {
    byte* b   = CsByteVectorAddress(val);
    int   len = CsByteVectorSize(val);
    //tool::string str = CsSymbolName(val);
    db_v.len = 1 + len; // length in bytes + 1 byte for the type
    db_v.data.s = new byte[db_v.len];
    byte strType = db_blob;
    ::memcpy( db_v.data.s, &strType, 1);
    ::memcpy( db_v.data.s + 1, b, len);
    db_v.type = dybase_string_type;
  }
  else if( CsColorP( val ) )
  {
    db_v.data.i = CsColorValue( val );
    db_v.type = dybase_color_type;
  }
  else
  {
    // TODO: not supported?
trace( L"unknown type in Transform()\n");
    assert(false);
  }
//trace( L"End of Transform\n" );
  return;
}

void StoreValue( VM *c, value vs, dybase_handle_t h, value val)
{
  db_triplet db_val;
  Transform(c, vs, val, db_val);
  dybase_store_array_element(h,
          db_val.type,
         (db_val.type == dybase_string_type) ? (void*)db_val.data.s : &db_val.data,
          db_val.len);
}


}

//#pragma optimize( "", on )
