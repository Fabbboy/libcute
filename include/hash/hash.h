#pragma once

#include <stddef.h>
#include <stdint.h>

/** \brief 32-bit FNV-1a offset basis. */
#define CU_FNV1A32_OFFSET_BASIS 2166136261u
/** \brief 32-bit FNV-1a prime. */
#define CU_FNV1A32_PRIME 16777619u

/** \brief 64-bit FNV-1a offset basis. */
#define CU_FNV1A64_OFFSET_BASIS 14695981039346656037ull
/** \brief 64-bit FNV-1a prime. */
#define CU_FNV1A64_PRIME 1099511628211ull

/** \brief Murmur3 constant c1. */
#define CU_MURMUR3_C1 0xcc9e2d51u
/** \brief Murmur3 constant c2. */
#define CU_MURMUR3_C2 0x1b873593u
/** \brief Murmur3 constant used during mixing. */
#define CU_MURMUR3_N 0xe6546b64u
/** \brief Murmur3 finalization constant 1. */
#define CU_MURMUR3_F1 0x85ebca6bu
/** \brief Murmur3 finalization constant 2. */
#define CU_MURMUR3_F2 0xc2b2ae35u

/** \brief SipHash initial v0 constant. */
#define CU_SIPHASH_V0_INIT 0x736f6d6570736575ull
/** \brief SipHash initial v1 constant. */
#define CU_SIPHASH_V1_INIT 0x646f72616e646f6dull
/** \brief SipHash initial v2 constant. */
#define CU_SIPHASH_V2_INIT 0x6c7967656e657261ull
/** \brief SipHash initial v3 constant. */
#define CU_SIPHASH_V3_INIT 0x7465646279746573ull

/**
 * \brief Compute a 32-bit FNV-1a hash.
 *
 * \param data input bytes
 * \param len number of bytes
 */
uint32_t cu_Hash_FNV1a32(const void *data, size_t len);
/** \brief Compute a 64-bit FNV-1a hash. */
uint64_t cu_Hash_FNV1a64(const void *data, size_t len);

/**\brief Compute a 32-bit Murmur3 hash. */
uint32_t cu_Hash_Murmur3_32(const void *data, size_t len, uint32_t seed);

/**\brief Compute SipHash-2-4. */
uint64_t cu_Hash_SipHash24(const uint8_t key[16], const void *data, size_t len);
