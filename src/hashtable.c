#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

void i_hashtable_free_key (i_hashtable_key *key)
{
	if (!key) return;

	if (key->key) free (key->key);
	free (key);
}

void i_hashtable_key_free (i_hashtable_key *key)
{ i_hashtable_free_key (key); }

void i_hashtable_free_cell (i_hashtable_cell *cell)
{
	if (!cell) return;

	if (cell->key) i_hashtable_free_key (cell->key);	/* Free the key data */
	free (cell);						/* Free the cell */
}

void i_hashtable_clean (i_hashtable *ht)
{
  unsigned int i;
  if (!ht) return;
	
  if (ht->table)
  {
    for (i=0; i < ht->size; i++)						/* Loop through each cell */
    {
      i_hashtable_cell *cell;
      i_hashtable_cell *next;
    
      if (!ht->table[i]) continue;					/* Empty Cell */

      for (cell=ht->table[i]; cell != NULL; cell=next)
      {
        next = cell->next;
        if (ht->destructor) ht->destructor (cell->data);       
        i_hashtable_free_cell (cell);
        ht->inuse--;
      }

      ht->table[i] = NULL;						/* Mark cell as empty */
    }	
  }
}

void i_hashtable_free (void *htptr)
{
  i_hashtable *ht = htptr;

  if (!ht) return;

  i_hashtable_clean (ht);                         /* Remove all entries */
  free (ht->table);
  free (ht);
}

i_hashtable* i_hashtable_create (unsigned int size)
{
  i_hashtable *ht;

	ht = (i_hashtable *) malloc (sizeof(i_hashtable));
	if (!ht) return NULL;
	memset (ht, 0, sizeof(i_hashtable));

    ht->size = size;

	ht->table = (i_hashtable_cell **) calloc (size, sizeof(i_hashtable_cell));
    if (!ht->table) { i_hashtable_free (ht); return NULL; }

	return ht;
}

void i_hashtable_set_destructor (i_hashtable *ht, void (*destructor) (void *data))
{
  if (!ht) return;

  ht->destructor = destructor;
}

i_hashtable_key* i_hashtable_create_key (void *key, int size)				/* Create the key struct */
{
	i_hashtable_key *ht_key;

	if (size < 1) return NULL;

	ht_key = (i_hashtable_key *) malloc (sizeof(i_hashtable_key));
	if (!ht_key) return NULL;

	ht_key->key = malloc (size);
	memcpy (ht_key->key, key, size);
	ht_key->size = size;

	return ht_key;
}

i_hashtable_key* i_hashtable_duplicate_key (i_hashtable_key *key)
{
	i_hashtable_key *return_key;

	return_key = (i_hashtable_key *) malloc (sizeof(i_hashtable_key));
	if (!return_key) return NULL;

	return_key->size = key->size;
	return_key->hash = key->hash;

	return_key->key = malloc (key->size);
	memcpy (return_key->key, key->key, key->size);

	return return_key;
}


int i_hashtable_put (i_hashtable *table, i_hashtable_key *key, void *data)		/* Put the data into the table, 0 on success, -1 on fail */
{
	i_hashtable_cell *existing;
	i_hashtable_cell *cell;

	if (!key || !table || !data) return -1;

	cell = (i_hashtable_cell *) malloc (sizeof(i_hashtable_cell));
	if (!cell) return -1;
	memset (cell, 0, sizeof(i_hashtable_cell));

	cell->key = i_hashtable_duplicate_key (key);					/* Create a dup of the key so the passed key can be freed */
	cell->data = data;

	existing = table->table[key->hash];						/* Get the existing cell */

	if (existing)									/* Cell isnt empty, tag on the end */
	{
		i_hashtable_cell *last;

		for (existing=table->table[key->hash]; existing != NULL; existing = existing->next)
		{
			last = existing;
		}

		last->next = cell;							/* Tagged on the end */
	}
	else										/* Cell is empty, simple store */
	{
		table->table[key->hash] = cell;
	}

    table->inuse++;
	return 0;
}

void* i_hashtable_get (i_hashtable *table, i_hashtable_key *key)			/* Get the data */
{
	i_hashtable_cell *cell;

	if (!table || !key) return NULL;	

	cell=table->table[key->hash];

	while (cell != NULL)
	{
		if (cell->key->size == key->size)					/* Do the sizes look right? */
		{ 
			if ((memcmp(cell->key->key, key->key, key->size)) == 0) return cell->data; /* Data verified! */
		}

		cell = cell->next;
	}

	return NULL;
}

int i_hashtable_remove (i_hashtable *table, i_hashtable_key *key)
{
	i_hashtable_cell *cell;
	i_hashtable_cell *prev = NULL;

	if (!table || !key) return -1;	

	cell=table->table[key->hash];							/* Go to the cell */

	while (cell != NULL)								/* Move along the chain of cells */
	{
		if (cell->key->size == key->size)					/* Do the sizes look right? */
		{ 
			if ((memcmp(cell->key->key, key->key, key->size)) == 0) 
			{
				/* This is the cell to remove! */
				
				if (prev) prev->next = cell->next;			/* Bridge */
				else table->table[key->hash] = cell->next;		/* Move next to head */

                if (table->destructor) table->destructor (cell->data);
                i_hashtable_free_cell (cell);
                table->inuse--;
				return 0;
            } 
		}

		prev = cell;
		cell = cell->next;
	}

	return -1;									/* Not found */
}

