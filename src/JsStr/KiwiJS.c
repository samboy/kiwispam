/* Relased to the public domain by Sam Trenholme 2000 */

/* Routines that perform Kiwi's encryption (a 32-bit Blowfish variant)
   on js_string objects, and converts the data to/from ASCII */

#include <sys/types.h>
#include "JsStr.h"
#include "Kiwi.h"

/* Mapping for binary->JS_US_ASCII conversions */
char asciimap[] = "abcdefghijklmnopqrstuvwxyz234567";
/* Mapping for encoding and decoding five-letter codes */
unsigned char fivemap[]  = "abcdefghijklmnopqrstuvwxyzáéíóúñ";

u_int16_t default_sbox[4][16] = { { 0xd131, 0x98df, 0x2ffd, 0xd01a,
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

u_int16_t default_pbox[18] = { 0x243f, 0x85a3, 0x1319, 0x0370,
                               0xa409, 0x299f, 0x082e, 0xec4e,
                               0x4528, 0x38d0, 0xbe54, 0x34e9,
                               0xc0ac, 0xc97c, 0x3f84, 0xb547,
                               0x9216, 0x8979 };

static int kjs_is_setup = 0;

u_int16_t sbox[4][16];

u_int16_t pbox[18];

#define f(x) ( ( ( sbox[0][(x & 0xF000) >> 12] + sbox[1][(x & 0x0F00) >> 8] ) \
             ^ sbox[2][(x & 0x00F0) >> 4] ) + sbox[3][x & 0x000F] )

u_int16_t l,r;

/* For a given Blowfish-32 key, spit out the sboxes and pboxes */
/* key_sched: Perform a key schedule round for the inner core of the
   Kiwi encryption.
   Input:  Pointers to the left half and the right halves of a Kiwi
           32-bit block.
   Output: Void
   Global Variables used: The Pbox[] array
*/

void kjs_key_sched( u_int16_t *pl, u_int16_t *pr ) 
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

/* kjs_key_setup: Set up the P and S boxes for a Kiwi encrpytion
                  function.
   input: A pointer to a js_string object containing the key.
   output: Void.
   global vars used: pbox, sbox, default_pbox, default_sbox, kjs_is_setup
*/
 
void kjs_key_setup(js_string *key)
{

int counter, index;
char *point;

/* Do nothing if key is pointing to NULL (sanity check) */
if(js_has_sanity(key) == -1)
	return;

/* Initialize the pboxes and sboxes to their default values */

for(counter = 0 ; counter < 4 ; counter++)	
	for(index = 0 ; index < 16 ; index++)
		sbox[counter][index] = default_sbox[counter][index];

for(counter = 0; counter < 18 ; counter++)
	pbox[counter] = default_pbox[counter];

l = r = 0x0000;

/* Now, initialize the Pboxes and Sboxes based on the key */

point = key->string;

index = 0;

/* Initialize the Pboxes with the key */

for(counter=0;counter<18;counter++)
	{
	u_int16_t temp;
	temp = (*point & 0xff) << 8;
	point++;
	index++;
	if(!*point || index >= 28 || index >= 
                                           (key->unit_size * key->unit_count))
		{
		point = key->string;
		index = 0;
		}
	temp |= *point & 0xff;
	pbox[counter] ^= temp;
	point++;
	index++;
	if(!*point || index >= 28 || index >= 
             (key->unit_size * key->unit_count))
		{
		point = key->string;
		index = 0;
		}
	}

/* And run the encryption so as to garble up the pbox values */
for(counter=0;counter<18;counter+=2)
	kjs_key_sched(&pbox[counter],&pbox[(counter + 1)]);

/* Do the same to the sboxes */
for(counter=0;counter<4;counter++)
	for(index=0;index<16;index+=2)
		kjs_key_sched(&sbox[counter][index],
                              &sbox[counter][(index + 1)]);

kjs_is_setup = 1;

}

/* kjs_scramble: Convert a single 32-bit block of plaintext into ciphertext
                 input: The plaintext
                 output: The ciphertext
                 global variables used: pbox, sbox */
Kiwi_cdata kjs_scramble( Kiwi_cdata plaintext ) 
{
u_int16_t l,r,counter;
   
r = plaintext & 0xFFFF;
l = (plaintext >> 16) & 0xFFFF;

l ^= pbox[0];

for(counter=1;counter<17;counter++)
	{
	if(counter % 2)	
		r ^= f(l) ^ pbox[counter];
	else	
		l ^= f(r) ^ pbox[counter];
	}

r ^= pbox[17];

return ((r << 16) | (l & 0xFFFF));

}

/* kjs_unscramble: Convert a single 32-bit block of ciphertext into plaintext
                 input: The ciphertext
                 output: The plaintext
                 global variables used: pbox, sbox */
Kiwi_cdata kjs_unscramble( Kiwi_cdata plaintext ) 
{
u_int16_t l,r,counter;
   
l = plaintext & 0xFFFF;
r = (plaintext >> 16) & 0xFFFF;

r ^= pbox[17];

for(counter=16;counter>0;counter--)
        {
	if(counter % 2)
		r ^= f(l) ^ pbox[counter];
	else
		l ^= f(r) ^ pbox[counter];
	}

l ^= pbox[0];

return ((l << 16) | (r & 0xFFFF));

}

/* us_ascii_map: Convert a 5-bit number in to an 8-bit ASCII char.
   input: The 5-bit number
   output: An 8-bit JS_US_ASCII char 
   global vars: asciimap */
char us_ascii_map(int number)
        {
        return asciimap[number & 31];
        }

/* five_letter_map: Convert a 5-bit number that has both English and Spanish
   letters in to an 8-bit ascii char.
   input: The 5-bit number
   output: An 9-bit ISO_8859_1 char
   glocal vars: fivemap */
unsigned char five_letter_map(int number)
        {
        return fivemap[number & 31];
	}

/* us_ascii_unmap: Convert an 8-bit ASCII char generated by us_ascii_map
                   in to a 5-bit number
   input: An 8-bit JS_US_ASCII char;
   output: 5-bit number on success, JS_ERROR on invalid value */
int us_ascii_unmap(char data) {
    int index;
    /* Convert to lower-case if needed */
    if(data >= 'A' && data <= 'Z')
        data += 32;
    for(index=0;index<32;index++) 
        if(asciimap[index] == data) 
	    return index;
    return JS_ERROR;
    }

/* five_letter_unmap: Convert an 8-bit ISO 8859-1 char generated by 
                      five_letter_map in to a 5-bit number
   input: An 8-bit ISO 8859-1 char, or part of a utf-8 sequence
   output: 5-bit number on success, 64 to indicate that we need to ignore this
           byte, JS_ERROR on invalid value 
 */
int five_letter_unmap(unsigned char data) {
    int index;

    /* Convert to lower-case if needed */
    if(data >= 'A' && data <= 'Z')
        data += 32;

    /* Hackish code to handle the case of this routine being fed utf-8 */
    if(data == 0xc2 || data == 0xc3)
        return 64; /* Ignore this byte */
    if(data >= 0x81 && data <= 0x9a)
        data += 0x40;
    if(data >= 0xa1 && data <= 0xba)
        data += 0x40;

    /* See if the character is there */
    for(index=0;index<32;index++)
        if(fivemap[index] == data)
	    return index;

    /* If not, return error */
    return JS_ERROR;

    }

/* kjs_make_cookie: convert a 32-bit dinary data in to a string with readable 
                chars for the codepage we are in. 
   input: The data to encrypt, the string to put the encrypted cookie in
   output: JS_ERROR on error, JS_SUCCESS on success 
   global vars: kjs_is_setup */

int kjs_make_cookie(Kiwi_cdata data, js_string *strn)
{

        Kiwi_cdata shift;
	u_int8_t parity;

	int counter;

        /* Sanity checks */  
        if(js_has_sanity(strn) == -1)
                return JS_ERROR;
        if(!kjs_is_setup)
                return JS_ERROR;
        /* Currently, we only work with ASCII strings */
        if(strn->encoding != JS_US_ASCII)
                return JS_ERROR;
        /* No overflows */
        if(strn->unit_size * strn->max_count < 7)
                return JS_ERROR;


	/* Calculate the 3-bit parity of the plaintext */
	parity = 0;
	shift = data;
	for(counter = 0; counter < 11; counter++)
		{
		parity ^= shift & 0x07;
		shift >>= 3;
		}

	/* Do the actual encryption */

	shift = kjs_scramble(data);
	
	/* prepare the parity to add to the encrypted text */

	parity <<= 2;

        for(counter=0;counter<7;counter++)
                {
		if(counter == 6)
			shift |= parity;
		*(strn->string + counter) = us_ascii_map(shift & 31);
                shift >>= 5;
                }

	strn->unit_count = 7;
	strn->unit_size = 1;

	return JS_SUCCESS;

}

/* kjs_parse_cookie: convert an encrypted cookie like jlsz63a into 
                     the data it contains 
   input: A js_string object containing the encrypted data
   output: The plaintext as a u_int_32t*/

Kiwi_cdata kjs_parse_cookie(js_string *cookie)
	{

        int counter, index;

        Kiwi_cdata shift, temp;
	u_int16_t data[2];
	u_int8_t parity, paritycheck;

        /* Sanity checks */
	if(js_has_sanity(cookie) == -1)
                return 0;
        if(cookie->unit_size * cookie->unit_count < 7)
                return 0;
        /* Currently, we only work with ASCII strings */
        if(cookie->encoding != JS_US_ASCII)
                return JS_ERROR;

	/* Go through the 7-character cookie, and convert it to binary
	   data */
        for(counter=6;counter>=0;counter--) 
     		{
		/* We shift left if we are not on the right most end */
                if(counter<6)
                	shift <<= 5;

		/* What we do is go through the entire 32-character possible
		   mappings (where 0 is 'a', 1 is 'b', and so on), and
		   see if the character we are looking at has a mapping */

		/* For all 32 possible mappings... */
                for(index=0;index<32;index++) 
                	{
			/* Make the number 1 a 'l' to minimize errors
                           in transcribing cookies */
                        if(*(cookie->string + counter) == '1')
			    *(cookie->string + counter) = 'l';
                        /* Make the number 0 a 'o' */
                        if(*(cookie->string + counter) == '0')
			    *(cookie->string + counter) = 'o';
			/* If the character we are looking at matches
			   the mapping we are looking at... */
			if(*(cookie->string + counter) == asciimap[index])
				/* Then add the information to the binary
				   data */
				if(counter<6)
					shift |= index;
				else	
					/* If we are looking at the right
					   most character, we have two
					   pieces of binary data--the two
					   bits of data, and three bits
					   of parity */
					{
					shift = index & 0x03;
					parity = index & 0x1C;
					parity >>= 2;
					}
			}
                }

	/* Since the data (in shift) is cipher text, we use the unscramble
	   routine to make it plaintext */
	shift = kjs_unscramble(shift);

	/* We copy the plaintext over to a temporary variable, in preparation
	   for making sure the parity is correct */
	temp = shift;

	/* We look at the plaintext and determine what the correct parity
	   for it should be */	
	paritycheck = 0;
	for(counter = 0; counter < 11; counter++)
                {
                paritycheck ^= temp & 0x07;
                temp >>= 3;
                }

	/* If the parity is incorrect, we return it as 1 (bad parity) */
	if(paritycheck != parity)
		return 1;

        return shift;
        }

/* kjs_grokdata: Decode a 32-bit decrypted cookie in to the type of data
                 it is and the actual data it carries 
   input: shift: The data to decode
          type: A string which will describe the data
   output: A u_int_32t with the actual data in the decoded cookie (if
           applicable) */

Kiwi_cdata kjs_grokdata(Kiwi_cdata shift,js_string *type)
        {

        /* sanity checks */
        if(js_has_sanity(type) == -1)
            return 0;
        /* Only ASCII support for Kiwi js-type functions */
        if(type->unit_size != 1)
            return 0;
        if(type->encoding != JS_US_ASCII)
            return 0;

	/* If the high four bits are "1010" (A), we know it is a time 
	   stamped message with a long timeout */
	if((shift & 0xF0000000) == 0xA0000000)
		{
		shift <<= 4;
		js_qstr2js(type,"LONG TIMEOUT");
		}

	/* If the high four bits are "1000" (8), we know it is a time
	   stamped message with a short timeout */
	else if((shift & 0xF0000000) == 0x80000000)
		{
		shift <<= 4;
		js_qstr2js(type,"SHORT TIMEOUT");
		}
 
	/* If the high four bits are "1001" (9), we know it is a time
	   stamped message with a medium timeout */
	else if((shift & 0xF0000000) == 0x90000000)
		{
		shift <<= 4;
		js_qstr2js(type,"MID TIMEOUT");
		}

	/* If the high four bits are "1100" (C), we know the message is
	   an IP, within a 16-ip range */	
	else if((shift & 0xF0000000) == 0xC0000000)
		{
		shift <<= 4;
		js_qstr2js(type,"28BIT IP BLOCK");
		}

	/* If the high 12 bits are 0x7FF, we know the message is a
	   timestamp that changes value every hour or so (used for moving
	   a directory or some other information once an hour, mail
	   sent to a cookie in this form is marked as "bad" and discarded) */
	else if((shift & 0xFFF00000) == 0x7FF00000)
		{
		shift <<= 12;
		js_qstr2js(type,"HIDDEN DIR");
		}

	/* If the high 7 bits are "0111110" (7C), the remaining 25 bits are
           a five-letter message (where each letter uses five bits) */
	else if((shift & 0xFE000000) == 0x7C000000)
		{
		js_qstr2js(type,"FIVE LETTERS");
		}

	/* We can't grok the message, so we consider it an invalid message, 
   	   discard it if it is a mail message */
	else 
		{
		js_qstr2js(type,"INVALID");
		shift = 0;
		}

	return shift;

	}

