#define TISCRIPT_VERSION 4,0,8,56
#define TISCRIPT_VERSION_STR "4,0,8,56"

/*
  Version 4,0,8,56

  - corresponds to Sciter v. 1.0.8.56

  - changes:
  
     - this function
        compound keyword, refers to the current function object.
     - this super+
        compound keyword, for inner functions refers to the 'this' object in outer scope.
     - multi return feture
        functions are alowed to return mutiple values now.      
        for(... in object) is extended to return keys and values while enumeration:
          
          var obj = {one:1, two:2}; 
          for(var (key,val) in obj)
            stdout.printf("%s : %s\n", key, val); // enumeartion of keys and values
     - stringizers
       function starting of '$' are stringizers. See: http://www.terrainformatica.com/2009/10/stringizer-functions-in-tiscript/
     
     - 'include' statement is extended to support byte code files too (produced by the compile()).
  
     - ATTN: file format of bytecodes produced by the compile() function is changed in this version.
     
     - color as a persistent type now - can be stored and indexed in Storage as it is.
     
     - Various fixes in persistent module.
     
     - Heap is using Virtual Memory on Windows. Makes sense especially on Mobile.
  
 */
