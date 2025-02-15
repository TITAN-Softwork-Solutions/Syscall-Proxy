﻿#ifndef ACTIVEBREACH_H
#define ACTIVEBREACH_H

#ifdef _MSC_VER
#define strdup _strdup
#endif

#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
    extern HANDLE g_abInitializedEvent;
#endif

#ifndef NTSTATUS
    typedef long NTSTATUS;
#endif

#ifndef NTAPI
#define NTAPI __stdcall
#endif

#define STATUS_SUCCESS              ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

    typedef struct SyscallEntry {
        char* name;
        uint32_t ssn;
    } SyscallEntry;

    typedef struct SyscallTable {
        SyscallEntry* entries;
        size_t count;
    } SyscallTable;

    void* _Buffer(size_t* out_size);
    SyscallTable _GetSyscallTable(void* mapped_base);
    void _Cleanup(void* mapped_base);

    typedef struct {
        char* name;
        void* stub;
    } StubEntry;

    typedef struct ActiveBreach {
        uint8_t* stub_mem;
        size_t stub_mem_size;
        StubEntry* stubs;
        size_t stub_count;
    } ActiveBreach;

    void _ActiveBreach_Init(ActiveBreach* ab);
    int _ActiveBreach_AllocStubs(ActiveBreach* ab, const SyscallTable* table);
    void* _ActiveBreach_GetStub(ActiveBreach* ab, const char* name);
    void _ActiveBreach_Free(ActiveBreach* ab);

    /* Cleanup function; registers with atexit */
    void _ActiveBreach_Cleanup(void);

    /* Launch function spawns ab thread that performs initialization and handles ab_calls */
    void ActiveBreach_launch(void);

    /* --- Global ActiveBreach instance --- */
    extern ActiveBreach g_ab;

    /* --- Call Dispatcher --- */
    /*
       _ActiveBreach_Call dispatches a call to the worker thread
       Supports 0..8 ULONG_PTR args
    */
    ULONG_PTR _ActiveBreach_Call(void* stub, size_t arg_count, ...);

#ifdef __cplusplus
}
#endif

/* --- Macro Helpers to count args --- */
#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8, N, ...) N
#define PP_RSEQ_N() 8,7,6,5,4,3,2,1,0

#ifdef __cplusplus
#include <type_traits>
template <typename Fn, typename... Args>
inline auto ab_call_cpp(const char* name, Args... args)
-> decltype(((Fn)nullptr)(args...))
{
    void* stub = _ActiveBreach_GetStub(&g_ab, name);
    // Call the dispatcher and cast the ULONG_PTR result into the proper return type
    return (decltype(((Fn)nullptr)(args...)))_ActiveBreach_Call(stub, sizeof...(args), (ULONG_PTR)args...);
}
#define ab_call(nt_type, name, ...) ab_call_cpp<nt_type>(name, __VA_ARGS__)
#else
#define ab_call(nt_type, name, result, ...) do {                        \
      void* _stub = _ActiveBreach_GetStub(&g_ab, (name));                 \
      result = ((nt_type)_ActiveBreach_Call(_stub, PP_NARG(__VA_ARGS__),    \
                  (ULONG_PTR)__VA_ARGS__));                              \
  } while(0)
#endif

#endif // ACTIVEBREACH_H