## libcute 
libcute is a c-utility library.

memory-features:
 - [X] Allocator interface
 - [X] Page Allocator 
 - [X] C Allocator
 - [ ] GP Allocator 
 - [ ] Slab Allocator
 - [ ] Arena Allocator 

macro-features:
 - [X] IF_NULL
 - [X] IF_NOT_NULL
 - [X] DIE 
 - [X] UNUSED
 - [X] ALIGN_UP
 - [ ] BIT 
 - [ ] PLAT_X (platform macros)
 - [ ] ARRAY_LEN

object-features:
 - [X] generic optional
 - [X] generic result
 - [X] slice
 - [ ] configurable passable (non global) logger 
 - [ ] string builder
 - [ ] generic error interface

 string-features:
 - [ ] string buffer
 - [ ] string views
 - [ ] string utility methods (maybe powered by simd)
 - [ ] utf8 utility methods

collection-features:
 - [ ] vector
 - [ ] hashmap
 - [ ] bitmap/bitset
 - [ ] linked and doubly linked 
 - [ ] ring buffer 

 method-features:
 - [ ] hashing methods FNV-1A, Murmur3, SipHash (tied to hashmap, stil separate)
 



