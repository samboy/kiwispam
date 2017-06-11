/* Copyright 1998,2000 Sam Trenholme
   This code is released to the public domain */

/* For a given Blowfish-32 key, spit out the sboxes and pboxes */

#include <Pilot.h>

UInt default_sbox[4][16] = { { 0xd131, 0x98df, 0x2ffd, 0xd01a,
                                    0xb8e1, 0x6a26, 0xba7c, 0xf12c,
                                    0x24a1, 0xb391, 0x0801, 0x858e,
                                    0x6369, 0x7157, 0xa458, 0xf493,
                               }, { 0x4b7a, 0xb5b3, 0xdb75, 0xc419,
                                    0xad6e, 0x49a7, 0x9cee, 0x8fed,
                                    0xecaa, 0x699a, 0x5664, 0xc2b1,
                                    0x1936, 0x7509, 0xa059, 0xe418,
                               }, { 0xe93d, 0x9481, 0xf64c, 0x9469,
                                    0x4115, 0x7602, 0xbcf4, 0xd4a2,
                                    0xd408, 0x3320, 0x43b7, 0x5000,
                                    0x1e39, 0x9724, 0x1421, 0xbf8b,
                               }, { 0x3a39, 0xd3fa, 0xabc2, 0x5ac5,
                                    0x5cb0, 0x4fa3, 0xd382, 0x99bc,
                                    0xd511, 0xbf0f, 0xd62d, 0xc700,
                                    0xb78c, 0x21a1, 0xb26e, 0x6a36 } };

UInt default_pbox[18] = { 0x243f, 0x85a3, 0x1319, 0x0370,
                               0xa409, 0x299f, 0x082e, 0xec4e,
                               0x4528, 0x38d0, 0xbe54, 0x34e9,
                               0xc0ac, 0xc97c, 0x3f84, 0xb547,
                               0x9216, 0x8979 };

UInt sbox[4][16];

UInt pbox[18];

#define f(x) ( ( ( sbox[0][(x & 0xF000) >> 12] + sbox[1][(x & 0x0F00) >> 8] ) \
             ^ sbox[2][(x & 0x00F0) >> 4] ) + sbox[3][x & 0x000F] )

UInt l,r;

void key_sched( UInt *pl, UInt *pr ) 
{
   
l ^= pbox[0];
r ^= f(l) ^ pbox[1];
l ^= f(r) ^ pbox[2];
r ^= f(l) ^ pbox[3];
l ^= f(r) ^ pbox[4];
r ^= f(l) ^ pbox[5];
l ^= f(r) ^ pbox[6];
r ^= f(l) ^ pbox[7];
l ^= f(r) ^ pbox[8];
r ^= f(l) ^ pbox[9];
l ^= f(r) ^ pbox[10];
r ^= f(l) ^ pbox[11];
l ^= f(r) ^ pbox[12];
r ^= f(l) ^ pbox[13];
l ^= f(r) ^ pbox[14];
r ^= f(l) ^ pbox[15];
l ^= f(r) ^ pbox[16];
r ^= pbox[17];

*pr = l;
*pl = r;

l = r;
r = *pr;

}

void key_setup(char *key, int length)
{

int counter, index;
char *point;

/* Do nothing if key is pointing to NULL (sanity check) */
if(!key)
	return;

/* Initialize the pboxes and sboxes to their default values */

for(counter = 0 ; counter < 4 ; counter++)	
	for(index = 0 ; index < 16 ; index++)
		sbox[counter][index] = default_sbox[counter][index];

for(counter = 0; counter < 18 ; counter++)
	pbox[counter] = default_pbox[counter];

l = r = 0x0000;

/* Now, initialize the Pboxes and Sboxes based on the key */

point = key;

index = 0;

/* Initialize the Pboxes with the key */

for(counter=0;counter<18;counter++)
	{
	UInt temp;
	temp = (*point & 0xff) << 8;
	point++;
	index++;
	if(!*point || index >= 28 || index >= length)
		{
		point = key;
		index = 0;
		}
	temp |= *point & 0xff;
	pbox[counter] ^= temp;
	point++;
	index++;
	if(!*point || index >= 28 || index >= length)
		{
		point = key;
		index = 0;
		}
	}

/* And run the encryption so as to garble up the pbox values */
for(counter=0;counter<18;counter+=2)
	key_sched(&pbox[counter],&pbox[(counter + 1)]);

/* Do the same to the sboxes */
for(counter=0;counter<4;counter++)
	for(index=0;index<16;index+=2)
		key_sched(&sbox[counter][index],&sbox[counter][(index + 1)]);

}

ULong scramble( ULong *plaintext ) 
{
UInt l,r,counter;
ULong pt;
  
pt = *plaintext;
 
r = pt & 0xFFFF;
l = (pt >> 16) & 0xFFFF;

l ^= pbox[0];

for(counter=1;counter<17;counter++)
	{
	if(counter % 2)	
		r ^= f(l) ^ pbox[counter];
	else	
		l ^= f(r) ^ pbox[counter];
	}

r ^= pbox[17];

pt = r;
pt <<= 16;
pt |= l;
*plaintext = pt;
return pt;

}

ULong unscramble( ULong *plaintext ) 
{
UInt l,r,counter;
ULong pt;

pt = *plaintext;
   
l = pt & 0xFFFF;
r = (pt >> 16) & 0xFFFF;

r ^= pbox[17];

for(counter=16;counter>0;counter--)
        {
	if(counter % 2)
		r ^= f(l) ^ pbox[counter];
	else
		l ^= f(r) ^ pbox[counter];
	}

l ^= pbox[0];

pt = r;
pt <<= 16;
pt |= l;
*plaintext = pt;
return pt;

}

