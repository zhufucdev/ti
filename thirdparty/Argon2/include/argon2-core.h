/*
 * Argon2 source code package
 *
 *  * 
 * Written by Daniel Dinu and Dmitry Khovratovich, 2015
 *  
 * This work is licensed under a Creative Commons CC0 1.0 License/Waiver.
 * 
 * You should have received a copy of the CC0 Public Domain Dedication along with
 * this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */


#pragma once

#ifndef __ARGON2_CORE_H__
#define __ARGON2_CORE_H__

#include <cstring>
#include "argon2.h"

/*************************Argon2 internal constants**************************************************/

/* Version of the algorithm */
const uint32_t ARGON2_VERSION_NUMBER = 0x10;

/* Memory block size in bytes */
const uint32_t ARGON2_BLOCK_SIZE = 1024;
const uint32_t ARGON2_WORDS_IN_BLOCK = ARGON2_BLOCK_SIZE / sizeof (uint64_t);
const uint32_t ARGON2_QWORDS_IN_BLOCK = ARGON2_WORDS_IN_BLOCK / 2;

/* Number of pseudo-random values generated by one call to Blake in Argon2i  to generate reference block positions*/
const uint32_t ARGON2_ADDRESSES_IN_BLOCK = (ARGON2_BLOCK_SIZE * sizeof (uint8_t) / sizeof (uint64_t));

/* Pre-hashing digest length and its extension*/
const uint32_t ARGON2_PREHASH_DIGEST_LENGTH = 64;
const uint32_t ARGON2_PREHASH_SEED_LENGTH = ARGON2_PREHASH_DIGEST_LENGTH + 8;

/* Argon2 primitive type */
enum Argon2_type {
    Argon2_d=0,
    Argon2_i=1,
    Argon2_id=2,
    Argon2_ds=4
};


/*****SM-related constants******/
const uint32_t ARGON2_SBOX_SIZE = 1 << 10;
const uint32_t ARGON2_SBOX_MASK = ARGON2_SBOX_SIZE / 2 - 1;


/*************************Argon2 internal data types**************************************************/

/*
 * Structure for the (1KB) memory block implemented as 128 64-bit words.
 * Memory blocks can be copied, XORed. Internal words can be accessed by [] (no bounds checking).
 */
struct block {
    uint64_t v[ARGON2_WORDS_IN_BLOCK];

    block() { //default ctor
        
    }

    explicit block(uint8_t in) { 
        memset(v, in, ARGON2_BLOCK_SIZE);
    }

    uint64_t& operator[](const uint32_t i) { //Subscript operator
        return v[i];
    }

    block& operator=(const block& r) { //Assignment operator
        memcpy(v, r.v, ARGON2_BLOCK_SIZE);
        return *this;
    }

    block& operator^=(const block& r) { //Xor-assignment
        for (uint32_t j = 0; j < ARGON2_WORDS_IN_BLOCK; ++j) {
            v[j] ^= r.v[j];
        }
        return *this;
    }

    block(const block& r) {
        memcpy(v, r.v, ARGON2_BLOCK_SIZE);
    }
};

/*
 *  XORs two blocks
 * @param  l  Left operand
 * @param  r  Right operand
 * @return Xors of the blocks
 */
block operator^(const block& l, const block& r);

/*
 * Argon2 instance: memory pointer, number of passes, amount of memory, type, and derived values. 
 * Used to evaluate the number and location of blocks to construct in each thread
 */
struct Argon2_instance_t {
    block*  memory; //Memory pointer
    const uint32_t passes; //Number of passes
    const uint32_t memory_blocks; //Number of blocks in memory
    const uint32_t lanes; //Number of lanes
    const uint32_t threads; //Actual parallelism. If @threads > @lanes, no error is reported, just unnecessary threads are not created
    const Argon2_type type; //Argon2d, 2id, 2i, or 2ds
    const uint32_t lane_length; //Value derived from @memory_blocks and @lanes  --- just for cache and readability
    const uint32_t segment_length;  //Value derived from @lane_length and SYNC_POINTS --- just for cache and readability
    uint64_t *Sbox; //S-boxes for Argon2_ds
    const bool internal_print; //whether to print the memory blocks to the file - for test vectors only!

    Argon2_instance_t(block* ptr, Argon2_type t, uint32_t p, uint32_t m, uint32_t l, uint32_t thr, bool pr) :
    memory(ptr),  passes(p), memory_blocks(m), lanes(l),threads(thr), type(t),   lane_length(m / l),
    segment_length(m / (l*ARGON2_SYNC_POINTS)),
     Sbox(NULL), internal_print(pr) {
    };
};

/*
 * Argon2 position: where we construct the block right now. Used to distribute work between threads.
 */
struct Argon2_position_t {
    const uint32_t pass;
    const uint32_t lane;
    const uint8_t slice;
    uint32_t index;

    Argon2_position_t(uint32_t p, uint32_t l, uint8_t s, uint32_t i) : pass(p),  lane(l), slice(s), index(i) {
    };
};

/*Macro for endianness conversion*/

#if defined(_MSC_VER) 
#define BSWAP32(x) _byteswap_ulong(x)
#else
#define BSWAP32(x) __builtin_bswap32(x)
#endif

/*************************Argon2 core functions**************************************************/

/* Allocates memory to the given pointer
 * @param memory pointer to the pointer to the memory
 * @param m_cost number of blocks to allocate in the memory
 * @return ARGON2_OK if @memory is a valid pointer and memory is allocated
 */
int AllocateMemory(block **memory, uint32_t m_cost);

/* Deallocates memory
 * @param instance pointer to the current instance
 * @param clear_memory indicates if we clear the memory with zeros.
 */
void FreeMemory(Argon2_instance_t* instance, bool clear_memory);

/*
 * Generate pseudo-random values to reference blocks in the segment and puts them into the array
 * @param instance Pointer to the current instance
 * @param position Pointer to the current position
 * @param pseudo_rands Pointer to the array of 64-bit values
 * @pre pseudo_rands must point to @a instance->segment_length allocated values
 */
void GenerateAddresses(const Argon2_instance_t* instance, const Argon2_position_t* position, uint64_t* pseudo_rands);

/*
 * Computes absolute position of reference block in the lane following a skewed distribution and using a pseudo-random value as input
 * @param instance Pointer to the current instance
 * @param position Pointer to the current position
 * @param pseudo_rand 32-bit pseudo-random value used to determine the position
 * @param same_lane Indicates if the block will be taken from the current lane. If so we can reference the current segment
 * @pre All pointers must be valid
 */
uint32_t IndexAlpha(const Argon2_instance_t* instance, const Argon2_position_t* position, uint32_t pseudo_rand, bool same_lane);

/*
 * Function that validates all inputs against predefined restrictions and return an error code
 * @param context Pointer to current Argon2 context
 * @return ARGON2_OK if everything is all right, otherwise one of error codes (all defined in <argon2.h>
 */
int ValidateInputs(const Argon2_Context* context);

/*
 * Hashes all the inputs into @a blockhash[PREHASH_DIGEST_LENGTH], clears password and secret if needed
 * @param  context  Pointer to the Argon2 internal structure containing memory pointer, and parameters for time and space requirements.
 * @param  blockhash Buffer for pre-hashing digest
 * @param  type Argon2 type
 * @pre    @a blockhash must have at least @a PREHASH_DIGEST_LENGTH bytes allocated
 */
void InitialHash(uint8_t* blockhash, const Argon2_Context* context, Argon2_type type);

/*
 * Function creates first 2 blocks per lane
 * @param instance Pointer to the current instance
 * @param blockhash Pointer to the pre-hashing digest
 * @pre blockhash must point to @a PREHASH_SEED_LENGTH allocated values
 */
void FillFirstBlocks(uint8_t* blockhash, const Argon2_instance_t* instance);


/*
 * Function allocates memory, hashes the inputs with Blake,  and creates first two blocks. Returns the pointer to the main memory with 2 blocks per lane
 * initialized
 * @param  context  Pointer to the Argon2 internal structure containing memory pointer, and parameters for time and space requirements.
 * @param  instance Current Argon2 instance
 * @return Zero if successful, -1 if memory failed to allocate. @context->memory will be modified if successful.
 */
int Initialize(Argon2_instance_t* instance, Argon2_Context* context);

/*
 * XORing the last block of each lane, hashing it, making the tag. Deallocates the memory.
 * @param context Pointer to current Argon2 context (use only the out parameters from it)
 * @param instance Pointer to current instance of Argon2
 * @pre instance->memory must point to necessary amount of memory
 * @pre context->out must point to outlen bytes of memory
 * @pre if context->free_cbk is not NULL, it should point to a function that deallocates memory
 */
void Finalize(const Argon2_Context *context, Argon2_instance_t* instance);


/*
 * Function fills a new memory block
 * @param prev_block Pointer to the previous block
 * @param ref_block Pointer to the reference block
 * @param next_block Pointer to the block to be constructed
 * @param Sbox Pointer to the Sbox (used in Argon2_ds only)
 * @pre all block pointers must be valid
 */
void FillBlock(const block* prev_block, const block* ref_block, block* next_block, const uint64_t* Sbox);

/*
 * Function that fills the segment using previous segments also from other threads
 * @param instance Pointer to the current instance
 * @param position Current position
 * @pre all block pointers must be valid
 */
void FillSegment(const Argon2_instance_t* instance, Argon2_position_t position);

/*
 * Function that fills the entire memory t_cost times based on the first two blocks in each lane
 * @param instance Pointer to the current instance
 */
void FillMemoryBlocks(const Argon2_instance_t* instance);


/*
 * Function that performs memory-hard hashing with certain degree of parallelism
 * @param  context  Pointer to the Argon2 internal structure
 * @return Error code if smth is wrong, ARGON2_OK otherwise
 */
int Argon2Core(Argon2_Context* context, Argon2_type type);

/*
 * Generates the Sbox from the first memory block (must be ready at that time)
 * @param instance Pointer to the current instance 
 */
void GenerateSbox(Argon2_instance_t* instance);

#endif
