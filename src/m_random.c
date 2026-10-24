// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2022-2023 by tertu marybig.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h"  

#include "m_random.h"
#include "m_fixed.h"



typedef struct rnstate_s {
	UINT32 data[3];
	UINT32 counter;
} rnstate_t;

                                                                  
   
                                      
                            
    
static inline UINT32 RandomState_Get32(rnstate_t *state) {
	UINT32 result, b, c;

	b = state->data[1];
	c = state->data[2];
	result = state->data[0] + b + state->counter++;

	state->data[0] = b ^ (b >> 9);
	state->data[1] = c * 9;
	state->data[2] = ((c << 21) | (c >> 11)) + result;

	return result;
}

                                                            
   
                                       
                                                                  
                                               
    
static inline void RandomState_Seed(rnstate_t *state, UINT32 *seeds, size_t seed_count)
{
	size_t i;

	state->counter = 1;

	for(i = 0; i < 3; i++)
	{
		UINT32 seed_word;

		if(i < seed_count)
			seed_word = seeds[i];
		else
			seed_word = 0;


		state->data[2-i] = seed_word;
	}

	for(i = 0; i < 16; i++)
		RandomState_Get32(state);
}

                                                  
                                                                         
                                      
   
                                                   
   
                                      
                                              
                                             
    
static inline UINT32 RandomState_GetKey32(rnstate_t *state, const UINT32 limit)
{
	UINT32 raw_random, scaled_lower_word;
	UINT64 scaled_random;


	if (limit == 0) return 0;

	raw_random = RandomState_Get32(state);
	scaled_random = (UINT64)raw_random * (UINT64)limit;

	                                                                          
                                                                       
                                                                        
                      
   
	scaled_lower_word = (UINT32)scaled_random;


	if (scaled_lower_word < limit)
	{


		UINT32 scaled_limit;




		scaled_limit = (-limit) % limit;

		while (scaled_lower_word < scaled_limit)
		{
			raw_random = RandomState_Get32(state);
			scaled_random = (UINT64)raw_random * (UINT64)limit;
			scaled_lower_word = (UINT32)scaled_random;
		}
	}

	return scaled_random >> 32;
}


static rnstate_t m_randomstate = {
	.data = {0x4A3B6035U, 0x99555606U, 0x6F603421U},
	.counter = 16
};





                                                                  
                                                            
   
                                                   
    
fixed_t M_RandomFixed(void)
{
	return RandomState_Get32(&m_randomstate) >> (32-FRACBITS);
}

                                                    
                                                            
   
                                           
    
UINT8 M_RandomByte(void)
{
	return RandomState_Get32(&m_randomstate) >> 24;
}

                                                                        
                            
                                                            
   
                                      
                                        
    
INT32 M_RandomKey(INT32 a)
{
	boolean range_is_negative;
	INT64 range;
	INT32 random_result;

	range = a;
	range_is_negative = range < 0;

	if(range_is_negative)
		range = -range;

	random_result = RandomState_GetKey32(&m_randomstate, (UINT32)range);

	if(range_is_negative)
		random_result = -random_result;

	return random_result;
}

                                               
                            
                                                            
   
                         
                         
                                        
    
INT32 M_RandomRange(INT32 a, INT32 b)
{
	if (b < a)
	{
		INT32 temp;

		temp = a;
		a = b;
		b = temp;
	}

	const UINT32 spread = b-a+1;
	return (INT32)((INT64)RandomState_GetKey32(&m_randomstate, spread) + a);
}

                                                                       
                                     
                                              
    
boolean M_RandomSeedFromOS(void)
{
	UINT32 complete_word_count;

	union {
		UINT32 words[3];
		char bytes[sizeof(UINT32[3])];
	} seed_data;

	complete_word_count = I_GetRandomBytes((char *)&seed_data.bytes, sizeof(seed_data)) / sizeof(UINT32);


	if (complete_word_count == 0)
		return false;

	RandomState_Seed(&m_randomstate, (UINT32 *)&seed_data.words, complete_word_count);

	return true;
}

void M_RandomSeed(UINT32 seed)
{
	RandomState_Seed(&m_randomstate, &seed, 1);
}








static UINT32 randomseed = 0xBADE4404;


static UINT32 initialseed = 0xBADE4404;

                                         
                                                                                    
   
                                                   
    
ATTRINLINE static fixed_t FUNCINLINE __internal_prng__(void)
{
	randomseed ^= randomseed >> 13;
	randomseed ^= randomseed >> 11;
	randomseed ^= randomseed << 21;
	return ( (randomseed*36548569) >> 4) & (FRACUNIT-1);
}

                                                                  
                                                       
   
                                                   
    
#ifndef DEBUGRANDOM
fixed_t P_RandomFixed(void)
{
#else
fixed_t P_RandomFixedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_RandomFixed() at: %sp %d\n", rfile, rline);
#endif
	return __internal_prng__();
}

                                                    
                                                           
                                           
   
                                         
                         
    
#ifndef DEBUGRANDOM
UINT8 P_RandomByte(void)
{
#else
UINT8 P_RandomByteD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_RandomByte() at: %sp %d\n", rfile, rline);
#endif
	return (UINT8)((__internal_prng__()&0xFF00)>>8);
}

                                                                        
                            
                                 
   
                                      
                                        
                         
    
#ifndef DEBUGRANDOM
INT32 P_RandomKey(INT32 a)
{
#else
INT32 P_RandomKeyD(const char *rfile, INT32 rline, INT32 a)
{
	CONS_Printf("P_RandomKey() at: %sp %d\n", rfile, rline);
#endif
	return (INT32)(((INT64)__internal_prng__() * a) >> FRACBITS);
}

                                               
                            
                                 
   
                         
                         
                                        
                         
    
#ifndef DEBUGRANDOM
INT32 P_RandomRange(INT32 a, INT32 b)
{
#else
INT32 P_RandomRangeD(const char *rfile, INT32 rline, INT32 a, INT32 b)
{
	CONS_Printf("P_RandomRange() at: %sp %d\n", rfile, rline);
#endif
	return (INT32)(((INT64)__internal_prng__() * (b-a+1)) >> FRACBITS) + a;
}







                                                            
                       
   
                                                     
                         
    
fixed_t P_RandomPeek(void)
{
	UINT32 r = randomseed;
	fixed_t ret = __internal_prng__();
	randomseed = r;
	return ret;
}

                                                             
   
                                
                     
    
#ifndef DEBUGRANDOM
UINT32 P_GetRandSeed(void)
{
#else
UINT32 P_GetRandSeedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_GetRandSeed() at: %sp %d\n", rfile, rline);
#endif
	return randomseed;
}

                                                 
   
                                
                     
    
#ifndef DEBUGRANDOM
UINT32 P_GetInitSeed(void)
{
#else
UINT32 P_GetInitSeedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_GetInitSeed() at: %sp %d\n", rfile, rline);
#endif
	return initialseed;
}

                         
                                                             
   
                                   
                     
    
#ifndef DEBUGRANDOM
void P_SetRandSeed(UINT32 seed)
{
#else
void P_SetRandSeedD(const char *rfile, INT32 rline, UINT32 seed)
{
	CONS_Printf("P_SetRandSeed() at: %sp %d\n", rfile, rline);
#endif


	if (!seed) seed = 0xBADE4404;
	randomseed = initialseed = seed;
}

                                                       
                                                                             
                                                                
   
                     
    
UINT32 M_RandomizedSeed(void)
{
	UINT32 seed;

	do {
		seed = RandomState_Get32(&m_randomstate);
	} while(seed == 0);

	return seed;
}
