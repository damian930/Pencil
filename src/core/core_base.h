#ifndef CORE_BASE_H
#define CORE_BASE_H

#include "stdlib.h" // For memset, memcopy, ...
#include "stdint.h" // For int types
#include "stdio.h"  // For printf
#include "stdarg.h" // For va_args
#include "string.h" // For memset
#include "math.h"   // For math
#include "float.h"  // For FLT_MAX and such

/* todo: Maybe use this somewhere if it makes sense, this is a nice way to have this.
				 This is taken from RadDBG base/core.h file.
typedef enum Compiler
{
  Compiler_Null,
  Compiler_msvc,
  Compiler_gcc,
  Compiler_clang,
  Compiler_COUNT,
}
Compiler;
#if COMPILER_MSVC
# define Compiler_CURRENT Compiler_msvc
#elif COMPILER_GCC
# define Compiler_CURRENT Compiler_gcc
#elif COMPILER_CLANG
# define Compiler_CURRENT Compiler_clang
#else
# define Compiler_CURRENT Compiler_Null
#endif 
*/

// - Compiler 
#ifdef _MSC_VER
	#define COMPILER_MSVC 1
#else 
	#error "This codebase does not support compiling on any compiler other than MSVC."
#endif

// - Compilation modes
#if defined(DEBUG_MODE)
	#undef DEBUG_MODE
	#define DEBUG_MODE 1
#else 
	#define DEBUG_MODE 0
#endif

#if defined(RELEASE_MODE)
	#undef RELEASE_MODE
	#define RELEASE_MODE 1
#else 
	#define RELEASE_MODE 0
#endif

// todo: test cpp11 verson +, there it is a part of the standard
#define StaticAssert(expr, ...) static_assert(expr, __VA_ARGS__)

#if (!DEBUG_MODE && !RELEASE_MODE)
	StaticAssert(false, "None of the possible build modes are set. Possible build modes are: Debug, Release.")
#endif
#if (DEBUG_MODE && RELEASE_MODE)
	StaticAssert(false, "More than a single build mode specified.")
#endif
// This is just in case
#if DEBUG_MODE
	#undef RELEASE_MODE
	#define RELEASE_MODE 0
#endif

// - Compiler specific intrinsics
#define BreakPoint(...)
#define BreakPointCond(cond, ...) 
#if COMPILER_MSVC
	#include "intrin.h"
	
	#undef BreakPoint // only works for x86, x64, ARM, ARM64
	#define BreakPoint(...) do { printf("\n=== BreakPoint() macro fired || FILE: %s, LINE: %d ===\n\n", __FILE__, __LINE__); __debugbreak(); } while (0) 

	#if RELEASE_MODE
		#undef BreakPoint
		#define BreakPoint(...) do {} while (0)
	#endif

#endif

// Asssert:
// 		Used to merely check and break of false condition in the dev time. Not made for the app to work based on. 
//	  Invalid conditions that might break or should still be tested via ifs should not rely on this macro. 
// 
// HandleLater: 
//	  This is for lazy developers. Handle later just breaks in the debugger, but unlike regular assert doesnt allow 
//    compilation in Release builds. This is used for legic cases that should be handled but dont get handled right now
//    and are left for later for whatever reason. In normal code if those break we just assert, this is that, but
//    with static_assert in the release build to make sure that all these are gone by the time we do a release build.
//    There is a special macro that might be defined called DONT_ASSERT_HANDLE_LATER_MACROS. This makes HandleLater 
//    macros compile in Release builds. Its just there to be able to compile to maybe test compilation or something along those lines,
//    but should not be used for legit release builds. 

#if defined(DONT_ASSERT_HANDLE_LATER_MACROS)
	#undef DONT_ASSERT_HANDLE_LATER_MACROS
	#define DONT_ASSERT_HANDLE_LATER_MACROS 1
#else
	#define DONT_ASSERT_HANDLE_LATER_MACROS 0
#endif

// TODO: Check if warning is available
#if DONT_ASSERT_HANDLE_LATER_MACROS && RELESE_BUILD
	#warning "Release build with DONT_ASSERT_HANDLE_LATER_MACROS"
#endif

#define Assert(expr, ...)      do { if (!(expr)) { BreakPoint(); } } while (0) 
#define HandleLater(expr, ...) Assert(expr) 
#if RELEASE_MODE
	#define Assert(expr, ...)             

	#define HandleLater(expr, ...)      do { StaticAssert(0, __VA_ARGS__); } while (0) 
	#if DONT_ASSERT_HANDLE_LATER_MACROS
		#undef HandleLater
		#define HandleLater(expr, ...)         										
	#endif
#endif

#define BP                   BreakPoint();     // Quick alias
#define Handle(expr, ...)    HandleLater(expr) // Quick alias
#define InvalidCodePath(...) Assert(false)     // Quick alias
#define NotImplemented()     Assert(false)     // Quick alias

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef float  F32;
typedef double F64;

#define global                    // Allowes to search for global statics faster
#define Null NULL                 // I like this more
#define tu_specific static inline // Translation unit specific

#define per_thread 
#if COMPILER_MSVC
	#undef per_thread
	#define per_thread __declspec(thread) // TODO: This is not like really needed, but would be nice to check if this is supported based on the version of the compiler or the language version
#else
	StaticAssert(0, "The current compiler doesnt support a per_thread tls allocation at compile time, therefore a \"per_thread\" macro failed to be define.");
#endif

#define __Stringify(x) #x 
#define Stringify(x)   __Stringify(x)

#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))
#define TypeFieldOffset(Type, field) ((U64)&(((Type*)(0))->field))

#define DeferLoop(start_expr, end_expr) for (int __df__ = (start_expr, 0); __df__ != 1; __df__ = (end_expr, 1))
#define DeferInitReleaseLoop(init_expr, release_expr) \
	for (U32 _DeferInitReleaseLoop_It = 0; _DeferInitReleaseLoop_It == 0;) \
	for (init_expr; _DeferInitReleaseLoop_It == 0; _DeferInitReleaseLoop_It = 1, release_expr)

#define Bytes(n)     (U64)n
#define Kilobytes(n) 1024 * Bytes(n) 
#define Megabytes(n) 1024 * Kilobytes(n)
#define Gigabytes(n) 1024 * Megabytes(n)
#define Terabytes(n) 1024 * Gigabytes(n)

#define Thousands(n) 1000ULL * (U64)(n)
#define Millions(n)  1000ULL * Thousands(n)
#define Billions(n)  1000ULL * Millions(n)

#define ToggleBool(b) ( !(b) )
#define XOR(a, b)     ( ((a) && !(b)) || (!(a) && (b)) )
#define NAND(a, b)    ( (!!(a) != 1) || (!!(b) != 1)   ) // Nand == Not AND

#define EachIndex(it, count)                          (U64 it = 0; it < count; it += 1)
#define EachEnumRange(it, Type, min_value, max_value) (Type it = min_value; it < max_value; it = (Type)((U64)it + 1))	

// TODO: I dont like that
#define EachEnum1ToCount(Type, it) EachEnumRange(it, Type, (Type)1, Type##__COUNT)

// Stack is a list that only has the "first" node pointer. Nodes only have the "next" pointer.
// When pushed onto the stack list, the first element is the new node, and the old first is now next for the new node.
//
// Queue is a list that has the "first" and "last" node pointers.
// Nodes only store the "next" node pointer.
//
// Dll is a list with "first" and "last" node pointers.
// Nodes store the "next" and "prev" node pointers.
#define StackPush_Name(list, new_node, name_for_first_in_list, name_for_next_in_node) \
            if ((list)->name_for_first_in_list == 0) {     \
                (list)->name_for_first_in_list = new_node; \
            } else {                                     \
                (new_node)->name_for_next_in_node = (list)->name_for_first_in_list; \
                (list)->name_for_first_in_list = new_node;                        \
            }
#define StackPop_Name(list, name_for_the_first_in_list, name_for_next_in_node) \
						if ((list)->name_for_the_first_in_list) { \
							(list)->name_for_the_first_in_list = (list)->name_for_the_first_in_list->name_for_next_in_node; \
						}

#define QueuePushFront_Name(queue, new_node, name_for_first_in_queue, name_for_last_in_queue, name_for_next_in_node) \
            if (queue->name_for_first_in_queue == 0) {     \
                queue->name_for_first_in_queue = new_node; \
                queue->name_for_last_in_queue = new_node;  \
            } else {                                       \
                new_node->name_for_next_in_node = queue->name_for_first_in_queue; \
                queue->name_for_first_in_queue = new_node;                       \
            }
#define QueuePushBack_Name(queue, new_node, name_for_first_in_queue, name_for_last_in_queue, name_for_next_in_node) \
            if ((queue)->name_for_first_in_queue == 0) {     \
                (queue)->name_for_first_in_queue = new_node; \
                (queue)->name_for_last_in_queue = new_node;  \
            } else {                                       \
               (queue)->name_for_last_in_queue->name_for_next_in_node = new_node; \
               (queue)->name_for_last_in_queue = new_node;                        \
            }

#define QueuePopFront_Name(queue, name_for_the_first_in_queue, name_for_the_last_in_queue, name_for_the_next_in_node) \
						if (queue->name_for_the_first_in_queue == queue->name_for_the_last_in_queue) { \
							queue->name_for_the_first_in_queue = 0; \
							queue->name_for_the_last_in_queue = 0; \
						} else if (queue->name_for_the_first_in_queue != 0) { \
								queue->name_for_the_first_in_queue = queue->name_for_the_first_in_queue->name_for_the_next_in_node; \
						}

#define DllPushBack_Name(dll_p, new_node_p, name_for_first_in_dll, name_for_last_in_dll, name_for_next_in_node, name_for_prev_in_node) \
						if (   (dll_p)->name_for_first_in_dll == Null                             \
								&& (dll_p)->name_for_last_in_dll == Null                              \
						) { 									 												                          \
							(dll_p)->name_for_first_in_dll = new_node_p;                            \
							(dll_p)->name_for_last_in_dll = new_node_p; 													  \
						} 																							 											  \
						else if ((dll_p)->name_for_first_in_dll == (dll_p)->name_for_last_in_dll) { \
							(dll_p)->name_for_first_in_dll->name_for_next_in_node = new_node_p;     \
							(new_node_p)->name_for_prev_in_node = (dll_p)->name_for_first_in_dll;     \
							(dll_p)->name_for_last_in_dll = new_node_p; 												 		\
						} 											 												 												\
						else { 									 												 												\
							(dll_p)->name_for_last_in_dll->name_for_next_in_node = new_node_p; 			\
							(new_node_p)->name_for_prev_in_node = (dll_p)->name_for_last_in_dll; 		  \
 							(dll_p)->name_for_last_in_dll = new_node_p;  											 			\
						} 
#define DllPushFront_Name(dll_p, new_node_p, name_for_first_in_dll, name_for_last_in_dll, name_for_next_in_node, name_for_prev_in_node) \
						if (   dll_p->name_for_first_in_dll == Null                             \
								&& dll_p->name_for_last_in_dll  == Null                             \
						) {                                                                     \
								dll_p->name_for_first_in_dll = new_node_p;                          \
								dll_p->name_for_last_in_dll  = new_node_p;                          \
						}                                                                       \
						else if (dll_p->name_for_first_in_dll == dll_p->name_for_last_in_dll) { \
								new_node_p->name_for_next_in_node = dll_p->name_for_last_in_dll;    \
								dll_p->name_for_last_in_dll->name_for_prev_in_node = new_node_p;    \
								dll_p->name_for_first_in_dll = new_node_p;                          \
						}                                                                       \
						else {                                                                  \
								new_node_p->name_for_next_in_node = dll_p->name_for_first_in_dll;   \
								dll_p->name_for_first_in_dll->name_for_prev_in_node = new_node_p;   \
								dll_p->name_for_first_in_dll = new_node_p;                          \
						}
#define DllPopFront_Name(list, name_for_first_in_list, name_for_last_in_list, name_for_next_in_node, name_for_prev_in_node) \
						if (list->name_for_first_in_list == list->name_for_last_in_list) { \
							list->name_for_first_in_list = 0; \
							list->name_for_last_in_list = 0; \
						} else { \
							list->name_for_first_in_list = list->name_for_first_in_list->name_for_next_in_node; \
							list->name_for_first_in_list->name_for_prev_in_node = 0; \
						}
#define DllPopBack_Name(list, name_for_first_in_list, name_for_last_in_list, name_for_next_in_node, name_for_prev_in_node) \
						if (list->name_for_first_in_list == list->name_for_last_in_list) { \
							list->name_for_first_in_list = 0; \
							list->name_for_last_in_list = 0; \
						} else { \
							list->name_for_last_in_list = list->name_for_last_in_list->name_for_prev_in_node; \
							list->name_for_last_in_list->name_for_next_in_node = 0; \
						}						
#define DllPop_Name(list, node_to_pop, name_for_first_in_list, name_for_last_in_list, name_for_next_in_node, name_for_prev_in_node) \
						if (node_to_pop) { \
							if (list->name_for_first_in_list == 0 && list->name_for_last_in_list == 0) { \
								InvalidCodePath("This should not happened. Node is given but list is not valid for it."); \
							} else if (list->name_for_first_in_list == node_to_pop && list->name_for_last_in_list == node_to_pop) { \
								list->name_for_first_in_list = list->name_for_last_in_list = 0; \
							} else if (list->name_for_first_in_list == node_to_pop) { \
								list->name_for_first_in_list = list->name_for_first_in_list->name_for_next_in_node; \
								list->name_for_first_in_list->name_for_prev_in_node = 0; \
							} else if (list->name_for_last_in_list == node_to_pop) { \
								list->name_for_last_in_list = list->name_for_last_in_list->name_for_prev_in_node; \
								list->name_for_last_in_list->name_for_next_in_node = 0; \
							} \
							else { \
								node_to_pop->name_for_prev_in_node->name_for_next_in_node = node_to_pop->name_for_next_in_node; \
								node_to_pop->name_for_next_in_node->name_for_prev_in_node = node_to_pop->name_for_prev_in_node; \
							} \
						}

// TODO: This is new code, not yet part of the official code kind of thing
#define DllPushBack_Name_NullFunc(dll_p, new_node_p, name_for_first_in_dll, name_for_last_in_dll, name_for_next_in_node, name_for_prev_in_node, is_node_null_func) \
						if (   is_node_null_func((dll_p)->name_for_first_in_dll)                             \
								&& is_node_null_func((dll_p)->name_for_last_in_dll)                              \
						) { 									 												                          \
							(dll_p)->name_for_first_in_dll = new_node_p;                            \
							(dll_p)->name_for_last_in_dll = new_node_p; 													  \
						} 																							 											  \
						else if ((dll_p)->name_for_first_in_dll == (dll_p)->name_for_last_in_dll) { \
							(dll_p)->name_for_first_in_dll->name_for_next_in_node = new_node_p;     \
							(new_node_p)->name_for_prev_in_node = (dll_p)->name_for_first_in_dll;     \
							(dll_p)->name_for_last_in_dll = new_node_p; 												 		\
						} 											 												 												\
						else { 									 												 												\
							(dll_p)->name_for_last_in_dll->name_for_next_in_node = new_node_p; 			\
							(new_node_p)->name_for_prev_in_node = (dll_p)->name_for_last_in_dll; 		  \
 							(dll_p)->name_for_last_in_dll = new_node_p;  											 			\
						} 

#define StackPush(list, new_node) StackPush_Name((list), (new_node), first, next)
#define StackPop(list)            StackPop_Name((list), first, next)

#define QueuePushFront(list, new_node) QueuePushFront_Name((list), (new_node), first, last, next)
#define QueuePushBack(list, new_node)  QueuePushBack_Name((list), (new_node), first, last, next)
#define QueuePopFront(list) 				   QueuePopFront_Name((list), first, last, next)
// QueuePopBack: Queue cant be popped from the back, since it would require a retraversal from first_node to set the new last as last

#define DllPushFront(list_p, new_node_p) DllPushFront_Name((list_p), (new_node_p), first, last, next, prev)
#define DllPushBack(list_p, new_node_p)  DllPushBack_Name((list_p), (new_node_p), first, last, next, prev)
#define DllPopFront(list_p)              DllPopFront_Name((list_p), first, last, next, prev)
#define DllPopBack(list_p)               DllPopBack_Name((list_p), first, last, next, prev)
#define DllPop(list_p, node_to_pop_p)    DllPop_Name((list_p), node_to_pop_p, first, last, next, prev)

#define SwapValues(Type, x, y) { Type temp = x; x = y; y = temp; }

#define Min(x, y) (x < y ? x : y)
#define Max(x, y) (x > y ? x : y)

enum Comparison : U32 {
  Comparison__NOT_EQUAL,
  Comparison__equal,
  Comparison__smaller,
  Comparison__greater,
};

// todo: Change this to be 
// UV__x0y0
// UV__x1y0
// UV__x0y1
// UV__x1y1
// TODO: The indexing here is off, use different names here
enum UV : U32 {
	UV__00,    // Top left
	UV__10,    // Top right
	UV__01,    // Bottom left
	UV__11,    // Bottom right
	UV__COUNT,
};

enum Axis2 : U32 {
	Axis2__x,
	Axis2__y,
	Axis2__COUNT,
};
tu_specific Axis2 axis2_other(Axis2 axis);

// TODO: Look into this (Claude told you this when you asked if this struct union thing is standardised)
// 
// This pattern uses anonymous structs inside a union, and the standardization status differs between C and C++:
// C
// Anonymous structs were officially standardized in C11 (§6.7.2.1). So in C11 and later, this is fully conformant.
// C++
// Anonymous structs are not part of any C++ standard (C++11/14/17/20/23). The standard only permits anonymous unions, not anonymous structs.
// However, every major compiler supports it as an extension:
//
// MSVC — supported by default
// GCC — supported with -std=gnu++XX or via -fms-extensions
// Clang — supported silently as an extension (may warn with -Wpedantic)
//
// So in practice this compiles everywhere, but it's technically non-conformant C++.
//
// The other issue: type-punning via union
// Even setting aside anonymous structs, accessing v[0] after writing to x (or vice versa) is type-punning through a union. The rules differ again:
// StatusCExplicitly legal (C99 TC3 / C11 §6.5.2.3)C++Undefined behavior per the standard
// In C++, the only standard-compliant way to type-pun is memcpy or std::bit_cast (C++20). In practice, GCC and Clang support union type-punning as an extension even in C++ mode, but it's not guaranteed.

union V2F32 {
	struct { F32 x; F32 y; };
	F32 v[2];
};
tu_specific V2F32 v2f32       (F32 x, F32 y);             
tu_specific B32   v2f32_match (V2F32 v1, V2F32 v2); 
tu_specific F32   v2f32_len_sq(V2F32 v);           
tu_specific F32   v2f32_len   (V2F32 v);              
tu_specific V2F32 v2f32_sub   (V2F32 v1, V2F32 v2);   

union V2U16 {
	struct { U16 x; U16 y; };
	U16 v[2];
};
tu_specific V2U16 v2u16      (U16 x, U16 y);
tu_specific B32   v2u16_match(V2U16 v1, V2U16 v2);

union V2U32 {
	struct { U32 x; U32 y; };
	U32 v[2];
};
tu_specific V2U32 v2u32      (U32 x, U32 y);
tu_specific B32   v2u32_match(V2U32 v1, V2U32 v2);

union V2U64 {
	struct { U64 x; U64 y; };
	U64 v[2];
};
tu_specific V2U64 v2u64      (U64 x, U64 y);
tu_specific B32   v2u64_match(V2U64 v1, V2U64 v2);

union V2S8 {
	struct { S8 x; S8 y; };
	S8 v[2];
};
tu_specific V2S8 v2s8      (S8 x, S8 y);
tu_specific B32  v2s8_match(V2S8 v1, V2S8 v2);

union V2S16 {
	struct { S16 x; S16 y; };
	S16 v[2];
};
tu_specific V2S16 v2s16      (S16 x, S16 y);
tu_specific B32   v2s16_match(V2S16 v1, V2S16 v2);

union V2S32 {
	struct { S32 x; S32 y; };
	S32 v[2];
};
tu_specific V2S32 v2s32      (S32 x, S32 y);
tu_specific B32   v2s32_match(V2S32 v1, V2S32 v2);

union V2S64 {
	struct { S64 x; S64 y; };
	S64 v[2];
};
tu_specific V2S64 v2s64      (S64 x, S64 y);
tu_specific B32   v2s64_match(V2S64 v1, V2S64 v2);

union V3F32 {
	struct { F32 x; F32 y; F32 z; };
	struct { F32 r; F32 g; F32 b; };
	struct { F32 hue; F32 saturation; F32 value; };
	F32 v[3];
};
tu_specific V3F32 v3f32(F32 x, F32 y, F32 z);

union V4F32 {
	struct { F32 x; F32 y; F32 z; F32 w; };
	struct { F32 r; F32 g; F32 b; F32 a; };
	F32 v[4];
	
	struct { V3F32 rgb; F32 a; };
	struct { V3F32 xyz; F32 w; };
	struct { V2F32 xy; V2F32 zw; };
	struct { F32 x; V3F32 yzw; };
	struct { F32 hue; F32 saturation; F32 value; F32 _; };
	struct { V3F32 hsv; F32 __; };
};
tu_specific V4F32 v4f32      (F32 x, F32 y, F32 z, F32 w);
tu_specific B32   v4f32_match(V4F32 v1, V4F32 v2);
tu_specific V4F32 v4f32_all  (F32 x);

union V4U8 {
	struct { U8 x; U8 y; U8 z; U8 w; };
	U8 v[4];
};
tu_specific V4U8 v4u8(U8 x, U8 y, U8 z, U8 w);

// TODO: Need better names here like Range1F32 and range2f32 for v2f32 to have the func names look great with no _ or case change
struct RangeF32 {
	F32 min; 
	F32 max;
};
tu_specific RangeF32 rangef32               (F32 min, F32 max);
tu_specific F32      rangef32_length        (RangeF32 range);
tu_specific B32      rangef32_within        (RangeF32 range, F32 v);
tu_specific B32      rangef32_is_valid      (RangeF32 range);
tu_specific B32      rangef32_contains_range(RangeF32 range, RangeF32 other);

struct RangeS64 {
	S64 min;
	S64 max;
};
tu_specific RangeS64 ranges64       (S64 start, S64 end);
tu_specific U64      range_s64_count(RangeS64 range);

struct RangeU64 {
	U64 min;
	U64 max;	
};
tu_specific RangeU64 rangeu64          (U64 min, U64 max);
tu_specific U64      range_u64_count   (RangeU64 range);
tu_specific B32      range_u64_within  (RangeU64 range, U64 v);
tu_specific B32      range_u64_is_valid(RangeU64 range);

struct RangeV2U64 {
	V2U64 min;
	V2U64 max;
};

struct RangeV2F32;
union Rect {
	struct { F32 x; F32 y; F32 width; F32 height; };
	struct { V2F32 origin; V2F32 dims; };
};
tu_specific Rect  rect_make            (F32 x, F32 y, F32 width, F32 height);
tu_specific Rect  rect_make_v          (V2F32 pos, V2F32 dims);
tu_specific Rect  rect_from_center     (V2F32 center, V2F32 dims);
tu_specific Rect  rect_from_range_v2f32(RangeV2F32 range);
tu_specific V2F32 rect_get_center      (Rect rect);
tu_specific B32   rect_match           (Rect r1, Rect r2);
tu_specific Rect  rect_padded          (Rect rect, F32 padd);
tu_specific B32   is_point_inside_rect (F32 x, F32 y, Rect r);
tu_specific B32   is_point_inside_rectV(V2F32 v, Rect r);

struct RangeV2F32 {
	V2F32 min;
	V2F32 max;
};
tu_specific RangeV2F32 range_v2f32                   (V2F32 min, V2F32 max);               
tu_specific RangeV2F32 range_v2f32_from_rect         (Rect rect);                          
tu_specific V2F32      range_v2f32_x0y0              (RangeV2F32 r);                       
tu_specific V2F32      range_v2f32_x1y0              (RangeV2F32 r);                       
tu_specific V2F32      range_v2f32_x0y1              (RangeV2F32 r);                       
tu_specific V2F32      range_v2f32_x1y1              (RangeV2F32 r);                       
tu_specific RangeF32   range_v2f32_x0x1              (RangeV2F32 r);                       
tu_specific RangeF32   range_v2f32_y0y1              (RangeV2F32 r);                       
tu_specific RangeV2F32 range_v2f32_as_bb             (V2F32 p1, V2F32 p2);                 
tu_specific B32        range_v2f32_match             (RangeV2F32 range, RangeV2F32 other); 
tu_specific B32        range_v2f32_within            (RangeV2F32 range, V2F32 vec);        
tu_specific V2F32      range_v2f32_dims              (RangeV2F32 range);
tu_specific RangeV2F32 intersect_range_v2f32_on_axis (RangeV2F32 range, RangeV2F32 other, Axis2 axis);
tu_specific RangeV2F32 intersect_range_v2f32         (RangeV2F32 range, RangeV2F32 other);

tu_specific F32 abs_f32(F32 x);
tu_specific F64 abs_f64(F64 x);
tu_specific S8  abs_s8 (S8  x);
tu_specific S16 abs_s16(S16 x);
tu_specific S32 abs_s32(S32 x);
tu_specific S64 abs_s64(S64 x);

tu_specific F32 clamp_f32(F32 value, F32 min, F32 max);
tu_specific F64 clamp_f64(F64 value, F64 min, F64 max);
tu_specific S8  clamp_s8 (S8  value, S8  min, S8  max);
tu_specific S16 clamp_s16(S16 value, S16 min, S16 max);
tu_specific S32 clamp_s32(S32 value, S32 min, S32 max);
tu_specific S64 clamp_s64(S64 value, S64 min, S64 max);
tu_specific U8  clamp_u8 (U8  value, U8  min, U8  max);
tu_specific U16 clamp_u16(U16 value, U16 min, U16 max);
tu_specific U32 clamp_u32(U32 value, U32 min, U32 max);
tu_specific U64 clamp_u64(U64 value, U64 min, U64 max);

tu_specific void clamp_f32_inplace(F32* value, F32 min, F32 max);
tu_specific void clamp_f64_inplace(F64* value, F64 min, F64 max);
tu_specific void clamp_s8_inplace (S8*  value, S8  min, S8  max);
tu_specific void clamp_s16_inplace(S16* value, S16 min, S16 max);
tu_specific void clamp_s32_inplace(S32* value, S32 min, S32 max);
tu_specific void clamp_s64_inplace(S64* value, S64 min, S64 max);
tu_specific void clamp_u8_inplace (U8*  value, U8  min, U8  max);
tu_specific void clamp_u16_inplace(U16* value, U16 min, U16 max);
tu_specific void clamp_u32_inplace(U32* value, U32 min, U32 max);
tu_specific void clamp_u64_inplace(U64* value, U64 min, U64 max);

tu_specific F32   lerp_f32  (F32 v0, F32 v1, F32 t);
tu_specific F64   lerp_f64  (F64 v0, F64 v1, F64 t);
tu_specific V2F32 lerp_v2f32(V2F32 v0, V2F32 v1, F32 t);
tu_specific V3F32 lerp_v3f32(V3F32 v0, V3F32 v1, F32 t);
tu_specific V4F32 lerp_v4f32(V4F32 v0, V4F32 v1, F32 t);

tu_specific F32 reverse_lerp_f32(F32 min, F32 max, F32 value);

#pragma warning(push)
#pragma warning(disable: 4309)

// - F32 constatns
global const U32 f32_sign        = 0x80000000;
global const U32 f32_exponent    = 0x7f800000;
global const U32 f32_mantisa     = 0x007fffff;
global const F32 f32_max_decimal = FLT_MAX;
tu_specific F32 f32_inf();
tu_specific F32 f32_neg_inf();
tu_specific F32 f32_nan();
tu_specific B32 f32_is_nan(F32 f); // Comparisons with nan result in false, even if its nan with nan. So we cant just do the ==, we have to do the byte comparison, for that we need a func.

// TODO: Fuck that, use std lib values here
global const S64 s64_min = 0x8000000000000000;
global const S64 s64_max = 0x7fffffffffffffff;
StaticAssert((S64)~s64_min == s64_max, "s64_max and s64_min are not right.");

global const U64 u64_min = 0x0000000000000000;
global const U64 u64_max = 0xffffffffffffffff;
StaticAssert((U64)~u64_min == u64_max, "u64_max and u64_min are not right.");

global const S32 s32_min = 0x80000000;
global const S32 s32_max = 0x7fffffff;
StaticAssert((S32)~s32_min == s32_max, "s32_max and s32_min are not right.");

global const U32 u32_min = 0x00000000;
global const U32 u32_max = 0xffffffff;
StaticAssert((U32)~u32_min == u32_max, "u32_max and u32_min are not right.");

global const S16 s16_min = 0x8000;
global const S16 s16_max = 0x7fff;
StaticAssert((S16)~s16_min == s16_max, "s16_max and s16_min are not right.");

global const U16 u16_min = 0x0000;
global const U16 u16_max = 0xffff;
StaticAssert((U16)~u16_min == u16_max, "u16_max and u16_min are not right.");

global const S8 s8_min = 0x80;
global const S8 s8_max = 0x7f;
StaticAssert((S8)~s8_min == s8_max, "s8_max and s8_min are not right.");

global const U8 u8_min = 0x00;
global const U8 u8_max = 0xff;
StaticAssert((U8)~u8_min == u8_max, "u8_max and u8_min are not right.");

#pragma warning(pop)

// - Colors 
// TODO: These shoud be macros since then you wont have to step into them in the debugger
tu_specific V4U8 transparent_u(); 
tu_specific V4U8 black_u();       
tu_specific V4U8 white_u();       
tu_specific V4U8 red_u();         
tu_specific V4U8 green_u();       
tu_specific V4U8 blue_u();        
tu_specific V4U8 yellow_u();      
tu_specific V4U8 pink_u();        
tu_specific V4U8 teal_u();        
tu_specific V4U8 orange_u();      
tu_specific V4U8 taupe_u();       
tu_specific V4U8 magenta_u();     
tu_specific V4U8 nice_green_u();  
tu_specific V4U8 nice_blue_u();   
  
tu_specific V4U8 change_alpha_u(V4U8 color, U8 new_a);

#define _U_COLOR_TO_F_COLOR(uc) v4f32((F32)uc.x/255.0f, (F32)uc.y/255.0f, (F32)uc.z/255.0f, (F32)uc.w/255.0f)

// TODO: These shoud be macros since then you wont have to step into them in the debugger
tu_specific V4F32 transparent(); 
tu_specific V4F32 black();       
tu_specific V4F32 white();       
tu_specific V4F32 red();         
tu_specific V4F32 green();       
tu_specific V4F32 blue();        
tu_specific V4F32 yellow();      
tu_specific V4F32 pink();        
tu_specific V4F32 teal();        
tu_specific V4F32 orange();      
tu_specific V4F32 taupe();       
tu_specific V4F32 magenta();     
tu_specific V4F32 nice_green();  
tu_specific V4F32 nice_blue();   

tu_specific V4F32 color_change_alpha(V4F32 color, F32 new_a);
tu_specific V4F32 color_light_up(V4F32 color, F32 how_much_lighter);

tu_specific V4F32 rgba_from_rgb(V3F32 rgb, F32 a);
tu_specific V3F32 rgb_from_rgba(V4F32 rgba);

tu_specific V3F32 hsv_from_rgb(V3F32 rgb);
tu_specific V3F32 rgb_from_hsv(V3F32 hsv);

tu_specific V4F32 hsva_from_rgba(V4F32 rgba);
tu_specific V4F32 rgba_from_hsva(V4F32 hsva);

tu_specific V4F32 rgba_from_hex(U32 hex);

tu_specific V4F32 purify_rgb(V4F32 rgb);

// - Memory 
#define U16From2U8(top, bottom)  (((U16)(top) << 8)  | (U16)(bottom)) 
#define U32From2U16(top, bottom) (((U32)(top) << 16) | (U32)(bottom)) 
#define U64From2U32(top, bottom) (((U64)(top) << 32) | (U64)(bottom)) 

tu_specific B32 __is_memory_zero(U8* p, U64 size);
#define IsMemZero(var)    __is_memory_zero((U8*)&(var), sizeof((var))) 
#define IsZeroStruct(var) IsMemZero(var)

#define MemCopySafe(dest, src) \
	do { \
		StaticAssert(sizeof(dest) == sizeof(src), "Cant copy memory safely, the sizes of dest and src variables are not the equal."); \
		memcpy(&dest, &src, sizeof(dest)); \
	} while(0)

// Thanks to AIG for this awesome text generation ))
global const U64 bit_0  = (1ULL << 0); // TODO: Should you start with 0 or 1
global const U64 bit_1  = (1ULL << 1);
global const U64 bit_2  = (1ULL << 2);
global const U64 bit_3  = (1ULL << 3);
global const U64 bit_4  = (1ULL << 4);
global const U64 bit_5  = (1ULL << 5);
global const U64 bit_6  = (1ULL << 6);
global const U64 bit_7  = (1ULL << 7);
global const U64 bit_8  = (1ULL << 8);
global const U64 bit_9  = (1ULL << 9);
global const U64 bit_10 = (1ULL << 10);
global const U64 bit_11 = (1ULL << 11);
global const U64 bit_12 = (1ULL << 12);
global const U64 bit_13 = (1ULL << 13);
global const U64 bit_14 = (1ULL << 14);
global const U64 bit_15 = (1ULL << 15);
global const U64 bit_16 = (1ULL << 16);
global const U64 bit_17 = (1ULL << 17);
global const U64 bit_18 = (1ULL << 18);
global const U64 bit_19 = (1ULL << 19);
global const U64 bit_20 = (1ULL << 20);
global const U64 bit_21 = (1ULL << 21);
global const U64 bit_22 = (1ULL << 22);
global const U64 bit_23 = (1ULL << 23);
global const U64 bit_24 = (1ULL << 24);
global const U64 bit_25 = (1ULL << 25);
global const U64 bit_26 = (1ULL << 26);
global const U64 bit_27 = (1ULL << 27);
global const U64 bit_28 = (1ULL << 28);
global const U64 bit_29 = (1ULL << 29);
global const U64 bit_30 = (1ULL << 30);
global const U64 bit_31 = (1ULL << 31);
global const U64 bit_32 = (1ULL << 32);
global const U64 bit_33 = (1ULL << 33);
global const U64 bit_34 = (1ULL << 34);
global const U64 bit_35 = (1ULL << 35);
global const U64 bit_36 = (1ULL << 36);
global const U64 bit_37 = (1ULL << 37);
global const U64 bit_38 = (1ULL << 38);
global const U64 bit_39 = (1ULL << 39);
global const U64 bit_40 = (1ULL << 40);
global const U64 bit_41 = (1ULL << 41);
global const U64 bit_42 = (1ULL << 42);
global const U64 bit_43 = (1ULL << 43);
global const U64 bit_44 = (1ULL << 44);
global const U64 bit_45 = (1ULL << 45);
global const U64 bit_46 = (1ULL << 46);
global const U64 bit_47 = (1ULL << 47);
global const U64 bit_48 = (1ULL << 48);
global const U64 bit_49 = (1ULL << 49);
global const U64 bit_50 = (1ULL << 50);
global const U64 bit_51 = (1ULL << 51);
global const U64 bit_52 = (1ULL << 52);
global const U64 bit_53 = (1ULL << 53);
global const U64 bit_54 = (1ULL << 54);
global const U64 bit_55 = (1ULL << 55);
global const U64 bit_56 = (1ULL << 56);
global const U64 bit_57 = (1ULL << 57);
global const U64 bit_58 = (1ULL << 58);
global const U64 bit_59 = (1ULL << 59);
global const U64 bit_60 = (1ULL << 60);
global const U64 bit_61 = (1ULL << 61);
global const U64 bit_62 = (1ULL << 62);
global const U64 bit_63 = (1ULL << 63);

// - Misc
// TODO: Structure these in the file better
enum Day : U8 { 
	Day__monday,
	Day__tuesday,
	Day__wednesday,
	Day__thursday,
	Day__friday, 
	Day__saturday, 
	Day__sunday,
	Day__COUNT,
};	

enum Month : U8 {
	Month__january,
	Month__february, 
	Month__march,
	Month__april,
	Month__may,
	Month__june,
	Month__july,
	Month__august,
	Month__september,
	Month__october,
	Month__november,
	Month__december,
	Month__COUNT,
};

struct Readable_time {
  U64 year;        // [0, 18,446,744,073,709,551,615]
  Month month;     // [0, 11]
  U8 day;          // [1, 31]
  U8 hour;         // [0, 24)
  U8 minute;       // [0, 60)
  U8 second;       // [0, 60)
  U16 millisecond; // [0, 1000)
};
typedef U64 Time; // This is used as just a value from Readable_time, it has to connections to any relative point. 

// todo: Not sure yet which ones of these i would like to keep
// note: i definatelly like the function like approach here --> MsFromSec(sec)
#define MsInSec 1000
#define SecInMin 60
#define MinInH 60
#define HoursInDay 24
#define MsFromSec(sec) (sec * MsInSec)

tu_specific Time time_from_readable_time(Readable_time* r_time);
tu_specific Readable_time readable_time_from_time(Time time);


#endif

