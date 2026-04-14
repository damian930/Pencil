#ifndef CORE_BASE_H
#define CORE_BASE_H

#include "stdlib.h" // For memset, memcopy, ...
#include "stdint.h" // For int types
#include "stdio.h"  // For printf
#include "stdarg.h" // For va_args

/* todo:
	[ ] - Remove todos
*/

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

// - Compilation sub modes
#if defined(DONT_ASSERT_HANDLE_LATER_MACROS)
	#undef DONT_ASSERT_HANDLE_LATER_MACROS
	#define DONT_ASSERT_HANDLE_LATER_MACROS 1
#else
	#define DONT_ASSERT_HANDLE_LATER_MACROS 0
#endif

// - Compiler specific intrinsics
#define BreakPoint(...)
#define BreakPointCond(cond, ...) 
#if COMPILER_MSVC
	#include <intrin.h>
	
	#undef BreakPoint
	#define BreakPoint(...) __debugbreak()

	#undef BreakPointCond
	#define BreakPointCond(cond) do { if (!cond) { __debugbreak(); } } while(0)  // only works for x86, x64, ARM, ARM64
#endif

// todo: test cpp11 verson +, there it is a part of the standard
#define StaticAssert(expr, ...) static_assert(expr, __VA_ARGS__)


#if (!DEBUG_MODE && !RELEASE_MODE)
	StaticAssert(false, "Both Debug and Release modes are not set. Set one of them before building.")
#endif
#if (DEBUG_MODE && RELEASE_MODE)
	StaticAssert(false, "Both Debug and Release modes are set. Only of them at a time is supported.")
#endif

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int Int;
typedef char Char; 

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef float  F32;
typedef double F64;

// #define static (reatined, persitent)
#define global static 
#define Null NULL 
#define tu_specific static inline // tu -> translation unit  

#define per_thread 
#if COMPILER_MSVC
	#undef per_thread
	#define per_thread __declspec(thread)
#else
	StaticAssert(0, "The current compiler doesnt support a per_thread tls allocation at compile time, therefore a \"per_thread\" macro failed to be define.");
#endif

// #define AtomicCompareExchangeU64(volatile_u64_p__dest, u64_new_value, u64_comparand) // note: returns the initial value of the destination address 
#define AtomicIncrementS64(volatile_s64_p_value) 															 // note: returns the the new incremented value at the address
#if COMPILER_MSVC
	// #undef AtomicCompareExchangeU64
	#undef AtomicIncrementS64
	
	// #define AtomicCompareExchangeU64(volatile_u64_p__dest, u64_new_value, u64_comparand) InterlockedCompareExchange((volatile U64*)volatile_u64_p__dest, (U64)u64_new_value, (U64)u64_comparand)
	#define AtomicIncrementS64(volatile_s64_p_value)                                     _InterlockedIncrement64(volatile_s64_p_value)
#else
	StaticAssert(0, "The current compiler does not support some needed compiler compiler intrinsics for atomic operations for the cpu.");
#endif

#define __Stringify(x) #x 
#define Stringify(x)   __Stringify(x)

#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

#define DeferLoop(start_expr, end_expr) for (int __df__ = (start_expr, 0); __df__ != 1; __df__ = (end_expr, 1))
#define DeferInitReleaseLoop(init_expr, release_expr) \
	for (U32 _DeferInitReleaseLoop_It = 0; _DeferInitReleaseLoop_It == 0;) \
	for (init_expr; _DeferInitReleaseLoop_It == 0; _DeferInitReleaseLoop_It = 1, release_expr)

#define Bytes(n)     (U64)n
#define Kilobytes(n) 1024 * Bytes(n) 
#define Megabytes(n) 1024 * Kilobytes(n)
#define Gigabytes(n) 1024 * Megabytes(n)
#define Terabytes(n) 1024 * Terabytes(n)

#define ToggleBool(b) ((b) = !(b));
#define XOR(a, b) ( ((a) && !(b)) || (!(a) && (b)) )
#define NAND(a, b) ( !!(a) != 1 || !!(b) != 1)       // Nand == Not and

#if RELEASE_MODE
	#define Assert(expr, ...)                do { (void)(expr); } while (0) 				         	// note: Removing the warning where the value that might only be be used for the assert, but the assert is compiled out in release build, so the warning of an unused variable appears
	#define AssertPrint(expr, note_fmt, ...) do { (void)(expr); (void)(note_fmt); } while (0) // 
	#define HandleLater(expr, ...)           do { StaticAssert(0, __VA_ARGS__); } while (0)   // note: We want all the HandleLater calls to be gone in the release build. HandleLater is used for something like testing os, where there are bunch of thing that might take place, like file open fail. So you put HandleLater in and then use the api, but for the final version tho HandleLater calles shoud be resolved. Therefore a StaticAssert. 
	#if DONT_ASSERT_HANDLE_LATER_MACROS
		#undef HandleLater
		#define HandleLater(expr, ...)         do { (void)(expr); } while (0)										// note: In case we dont want to deal with HandleLaters now, be we want to build in release to have the optimisations working, for example for testing multi threading and stuff.
	#endif
#else  
	#define Assert(expr, ...)                do { if (!(expr)) { U32* p = 0; *p = 69; } } while (0)
	#define AssertPrint(expr, note_fmt, ...) do { if (!(expr)) { printf(note_fmt, __VA_ARGS__); U32* p = 0; *p = 69; } } while (0)
	#define HandleLater(expr, ...)           Assert(expr) 
#endif

#define InvalidCodePath(...) do { Assert(false); } while (0)
#define NotImplemented(...)  do { Assert(false); } while (0)

#define EachIndex(it, count)                  (U64 it = 0; it < count; it += 1)
#define EachArrElement(it, static_arr)        (U64 it = 0; i < ArrayCount(static_arr); it += 1)
#define EachInRange(it, range_min, range_max) (U64 it = range_min; it < range_max; it += 1)
#define EachEnum(it, max_enum_value)          (U64 it = 0; it < max_enum_value; it += 1)

/* NOTES:
  Stack here is a list that only has the "first" node pointer. Nodes only have the "next" pointer.
  When pushed onto the stack list, the first element is the new node, and the old first is now next for the new node.

  Queue is a list that has the "first" and "last" node pointers.
  Nodes only store the "next" node pointer.

  Dll is a list with "first" and "last" node pointers.
  Nodes store the "next" and "prev" node pointers.
*/
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
            if (queue->name_for_first_in_queue == 0) {     \
                queue->name_for_first_in_queue = new_node; \
                queue->name_for_last_in_queue = new_node;  \
            } else {                                       \
               queue->name_for_last_in_queue->name_for_next_in_node = new_node; \
               queue->name_for_last_in_queue = new_node;                        \
            }

#define QueuePopFront_Name(queue, name_for_the_first_in_queue, name_for_the_last_in_queue, name_for_the_next_in_node) \
						if (queue->name_for_the_first_in_queue == queue->name_for_the_last_in_queue) { \
							queue->name_for_the_first_in_queue = 0; \
							queue->name_for_the_last_in_queue = 0; \
						} else if (queue->name_for_the_first_in_queue != 0) { \
								queue->name_for_the_first_in_queue = queue->name_for_the_first_in_queue->name_for_the_next_in_node; \
						}

#define DllPushBack_Name(dll_p, new_node_p, name_for_first_in_dll, name_for_last_in_dll, name_for_next_in_node, name_for_prev_in_node) \
						if (   dll_p->name_for_first_in_dll == Null                             \
								&& dll_p->name_for_last_in_dll == Null                              \
						) { 									 												                          \
							dll_p->name_for_first_in_dll = new_node_p;                            \
							dll_p->name_for_last_in_dll = new_node_p; 													  \
						} 																							 											  \
						else if (dll_p->name_for_first_in_dll == dll_p->name_for_last_in_dll) { \
							dll_p->name_for_first_in_dll->name_for_next_in_node = new_node_p;     \
							new_node_p->name_for_prev_in_node = dll_p->name_for_first_in_dll;     \
							dll_p->name_for_last_in_dll = new_node_p; 												 		\
						} 											 												 												\
						else { 									 												 												\
							dll_p->name_for_last_in_dll->name_for_next_in_node = new_node_p; 			\
							new_node_p->name_for_prev_in_node = dll_p->name_for_last_in_dll; 		  \
 							dll_p->name_for_last_in_dll = new_node_p;  											 			\
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
								InvalidCodePath("This should not happed. Node is given but list is not valid for it."); \
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
								node_to_pop->name_for_prev_in_node->next = node_to_pop->name_for_next_in_node; \
								node_to_pop->name_for_next_in_node->prev = node_to_pop->name_for_prev_in_node; \
							} \
						}

#define StackPush(list, new_node) StackPush_Name((list), (new_node), first, next)
#define StackPop(list) StackPop_Name((list), first, next)

#define QueuePushFront(list, new_node) QueuePushFront_Name((list), (new_node), first, last, next)
#define QueuePushBack(list, new_node)  QueuePushBack_Name((list), (new_node), first, last, next)
#define QueuePopFront(list) 				   QueuePopFront_Name((list), first, last, next)
// Queue cant be popped from the back, since it would require a retraversal from first_node to set the new last as last

#define DllPushFront(list_p, new_node_p) DllPushFront_Name((list_p), (new_node_p), first, last, next, prev)
#define DllPushBack(list_p, new_node_p)  DllPushBack_Name((list_p), (new_node_p), first, last, next, prev)
#define DllPopFront(list_p) DllPopFront_Name((list_p), first, last, next, prev)
#define DllPopBack(list_p) DllPopBack_Name((list_p), first, last, next, prev)
#define DllPop(list_p, node_to_pop_p) DllPop_Name((list_p), node_to_pop_p, first, last, next, prev)

#define SwapValues(Type, x, y) { Type temp = x; x = y; y = temp; }

#define Min(x, y) (x < y ? x : y)
#define Max(x, y) (x > y ? x : y)

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

tu_specific F32 lerp_f32(F32 v0, F32 v1, F32 t);
tu_specific F64 lerp_f64(F64 v0, F64 v1, F64 t);

tu_specific U64 u64_from_2_u32(U32 high_order_word, U32 low_order_word);

enum Day : U8 { 
	Day__monday,
	Day__tuesday,
	Day__wednesday,
	Day__thursday,
	Day__frinday, 
	Day__saturdan, 
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
  U64 year;       // [0, 18,446,744,073,709,551,615]
  Month month;       // [0, 11]
  U8 day;         // [1, 31]
  U8 hour;        // [0, 24)
  U8 minute;      // [0, 60)
  U8 second;      // [0, 60)
  U16 milisecond; // [0, 1000)
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

enum Comparison : U32 {
  Comparison__NOT_EQUAL,
  Comparison__equal,
  Comparison__smaller,
  Comparison__greater,
};

enum Axis2 {
	Axis2__x,
	Axis2__y,
	Axis2__COUNT,
};

tu_specific Axis2 axis2_other(Axis2 axis) { return (axis == Axis2__x ? Axis2__y : Axis2__x); }

// todo: Have the const values just be macros, this way you dont need cosnt
// 		   But then have the functions that dont have to be macros, like MsFromSec and stuff be functions
//	     If you really want to have them be macros to not generate function jumps, then just have them be 
//	     inlined, either way, i would assume the compiler to do it eitherway or not do at all 

// - F32 constatns
global const U32 f32_sign     = 0x80000000;
global const U32 f32_exponent = 0x7f800000;
global const U32 f32_mantisa  = 0x007fffff;
tu_specific F32 f32_inf();
tu_specific F32 f32_neg_inf();
tu_specific B32 f32_is_nan(F32 f);

global const S64 s64_min = 0x8000000000000000;
global const S64 s64_max = 0x7fffffffffffffff;
StaticAssert(~s64_min == s64_max, "s64_max and s64_min are not right.");

global const U64 u64_min = 0x0000000000000000;
global const U64 u64_max = 0xffffffffffffffff;
StaticAssert(~u64_min == u64_max, "u64_max and u64_min are not right.");

global const S32 s32_min = 0x80000000;
global const S32 s32_max = 0x7fffffff;
StaticAssert(~s32_min == s32_max, "s32_max and s32_min are not right.");

global const U32 u32_min = 0x00000000;
global const U32 u32_max = 0xffffffff;
StaticAssert(~u32_min == u32_max, "u32_max and u32_min are not right.");

// todo: signed 16 int

global const U16 u16_max = 0xffff;
global const U16 u16_min = 0x0000;

// todo: signed 8 int
// todo: unsigned 8 int

struct RangeS64 {
	S64 min;
	S64 max;
};
tu_specific RangeS64 range_s64_make(S64 start, S64 end);
tu_specific U64 range_s64_count(RangeS64 range);

struct RangeU64 {
	U64 min;
	U64 max;	
};
tu_specific RangeU64 range_u64_make(U64 min, U64 max);
tu_specific U64 range_u64_count(RangeU64 range);
tu_specific B32 range_u64_within(RangeU64 range, U64 v);

#define Address(var) (U64)(&(var))

// Thanks to AIG for this awesome text generation ))
global const U64 bit_0  = (1ULL << 0);
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

union Vec2_F32 {
	struct {
		F32 x;
		F32 y;
	};
	F32 v[2];
};
typedef Vec2_F32 Vec2;
typedef Vec2_F32 V2;

tu_specific Vec2_F32 vec2_f32_make(F32 x, F32 y) { Vec2_F32 v = { x, y }; return v; }
tu_specific B32 vec2_f32_match(Vec2_F32 v1, Vec2_F32 v2) { B32 b = (v1.x == v2.x && v1.y == v2.y); return b; }

struct Rect {
	F32 x;
	F32 y;
	F32 width;
	F32 height;
};

tu_specific Vec2_F32 rect_dims(Rect rect);
tu_specific B32 is_point_inside_rect(F32 x, F32 y, Rect r);
tu_specific B32 is_point_inside_rectV(Vec2_F32 v, Rect r);
tu_specific B32 is_point_inside_line(V2 point, V2 line_start, V2 line_end);

// note: this is static inline just cause i didnt care to move it to cpp file
tu_specific B32 __is_memory_zero(U8* p, U64 size)
{
	B32 is_zero = true;
	for (U64 i = 0; i < size; i += 1)
	{
		if (p[i] != 0) {
			is_zero = false;
			break;
		}
	}
	return is_zero;
}
#define IsMemZero(var) __is_memory_zero((U8*)&var, sizeof(var)) 

#define MemCopySafe(dest, src) \
	do { \
		StaticAssert("Cant copy memory safely, the sizes of dest and src variables are not the eqaul."); \
		memcpy(&dest, &src, sizeof(dest)); \
	} while(0);

tu_specific B32 __mem_compare(U8* m1, U64 size1, U8* m2, U64 size2)
{
	if (size1 != size2) { return false; }
	B32 result = true;
	for EachIndex(i, size1)
	{
		if (m1[i] != m2[i]) { result = false; break; }
	}
	return result;
}

// #define MemComapare(var1, size1, var2, size2) __mem_compare((U8*)&var1, sizeof(var1), (U8*)&var2, sizeof(var2))

union Vec4_F32 {
	struct { F32 x; F32 y; F32 z; F32 w; };
	struct { F32 r; F32 g; F32 b; F32 a; };
};
typedef Vec4_F32 Vec4;
typedef Vec4_F32 V4;

tu_specific Vec4_F32 vec4_f32_make(F32 x, F32 y, F32 z, F32 w) { Vec4_F32 v = { x, y, z, w }; return v; }

tu_specific B32 vec4_f32_match(Vec4_F32 v1, Vec4_F32 v2)
{
	B32 result = (
		v1.r == v2.r &&
		v1.g == v2.g &&
		v1.b == v2.b &&
		v1.a == v2.a 
	);
	return result;
}

struct ArrU64 {
  U64* data;
  U64  count;
};

#define U16_FROM_BYTES(top_byte, bottom_byte) (((U16)(top_byte) << 8) | (U16)(bottom_byte)) 

// #define COLOR_MACROS_PREFIX Color
// const V4_F32 COLOR_MACROS_PREFIX##Blue v4_f32_make(0.0f, 255.0f, 0.0f, 255.0f)


#endif


