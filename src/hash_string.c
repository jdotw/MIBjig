#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

#define mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c >> 13); \
  b -= c; b -= a; b ^= (a << 8); \
  c -= a; c -= b; c ^= (b >> 13); \
  a -= b; a -= c; a ^= (c >> 12); \
  b -= c; b -= a; b ^= (a << 16); \
  c -= a; c -= b; c ^= (b >> 5); \
  a -= b; a -= c; a ^= (c >> 3); \
  b -= c; b -= a; b ^= (a << 10); \
  c -= a; c -= b; c ^= (b >> 15); \
}

unsigned long i_hash_string (unsigned char *k, unsigned long htsize)
{
  unsigned long initval = htsize;
  register unsigned long int a, b, c, len;

  initval -= 1;

  /* Check string length */
  if (strlen((const char *)k) < 1)
  { printf ("i_hash_string failed, string length is < 1\n"); return 0; }
  
  /* Set up the internal state */
  len = strlen((const char *) k) - 1;
  a = b = 0x9e3779b9;     /* the golden ratio; an arbitrary value */
  c = initval;        /* the previous hash value */

  /* handle most of the key */
  while (len >= 12) 
  {
    a += (k[0] + ((unsigned long int) k[1] << 8) + ((unsigned long int) k[2] << 16) + ((unsigned long int) k[3] << 24));
    b += (k[4] + ((unsigned long int) k[5] << 8) + ((unsigned long int) k[6] << 16) + ((unsigned long int) k[7] << 24));
    c += (k[8] + ((unsigned long int) k[9] << 8) + ((unsigned long int) k[10] << 16) + ((unsigned long int) k[11] << 24));
    mix(a, b, c);
    k += 12;
    len -= 12;
  }

  /* handle the last 11 bytes */
  c += len;
  switch (len) {      /* all the case statements fall through */
    case 11:
      c += ((unsigned long int) k[10] << 24);
    case 10:
      c += ((unsigned long int) k[9] << 16);
    case 9:
      c += ((unsigned long int) k[8] << 8);
      /* the first byte of c is reserved for the length */
    case 8:
      b += ((unsigned long int) k[7] << 24);
    case 7:
      b += ((unsigned long int) k[6] << 16);
    case 6:
      b += ((unsigned long int) k[5] << 8);
    case 5:
      b += k[4];
    case 4:
      a += ((unsigned long int) k[3] << 24);
    case 3:
      a += ((unsigned long int) k[2] << 16);
    case 2:
      a += ((unsigned long int) k[1] << 8);
    case 1:
      a += k[0];
      /* case 0: nothing left to add */
  }
  mix(a, b, c);
  c = (c % initval) + 1;
  /* report the result */

  if (c >= htsize)
  {
    printf ("HT_FAILURE: i_hash_string calculated invalid hash of %lu (htsize is %lu). Returning fail-safe hash of 0\n", c, initval);
    return 0;
  }
    
  return (c);
}

i_hashtable_key* i_hashtable_create_key_string (char *str, unsigned long htsize)
{
  i_hashtable_key *key;

  key = i_hashtable_create_key (str, (strlen(str)+1));	
  if (!key) return NULL;

  key->hash = i_hash_string ((unsigned char *)str, htsize);

  return key;	
}
