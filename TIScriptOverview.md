# Source code organization #

TIScript has following major modules:

  1. compiler, (`cs_com.cpp`) that takes sequence of tokens from tokenizer (`cs_scn.cpp`),   produces byte codes.
  1. virtual machine (VM) that interprets (`cs_int.cpp`) sequence of byte codes.
  1. heap manager (`cs_heap.cs`) that is an implementation of copyable GC - uses two "halves" - two memory blocks.
  1. persistent DB engine (`cs_persistent.cpp`, `cs_storage.cpp`) that is integrated with the GC.
  1. and implementation of various runtime classes like `String` (`cs_string.cpp`), `Array` (`cs_vector.cpp`), `RegExp` (`cs_regexp.cpp`), `Integer` (`cs_integer.cpp`), `Float` (`cs_float.cpp`),`Math` (`cs_math.cpp`), etc.

# Persistent data base (OODB) #

Database (persistent storage) is presented in the script by two runtime classes: [Storage](http://www.terrainformatica.com/tiscript/Storage.htm) and [Index](http://www.terrainformatica.com/tiscript/Index.htm)

Each instance of the `Storage` has property `root` (rw) that can be either `object` or `array`.

All objects that are reachable from the `root` are persistent. Physical writing of data to the disk occurs when:
  * `storage.commit()` is called or
  * as a result of garbage collection - when there is not enough memory for allocation of some object. See: `CsCollectGarbage` function in `cs_heap.cpp`.

[Index object](http://www.terrainformatica.com/tiscript/Index.htm) represents **ordered** collection of items - key/value pairs. Index supports range selections.



