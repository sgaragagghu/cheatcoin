/* синхронизация, T13.738-T13.764 $DVS:time$ */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "sync.h"
#include "hash.h"
#include "init.h"
#include "transport.h"
#include "utils/log.h"				// it is more an hashtable

#define SYNC_HASH_SIZE      0x10000		// list of block we have to sync
#define get_list(hash)      (g_sync_hash   + ((hash)[0] & (SYNC_HASH_SIZE - 1))) // it takes the hash last 4 number(in hex, so last 16 binary number)
#define get_list_r(hash)    (g_sync_hash_r + ((hash)[0] & (SYNC_HASH_SIZE - 1))) // list of block that refer or are referred (need to understand) 
#define REQ_PERIOD          64

struct sync_block {
	struct xdag_block b;
	xdag_hash_t hash;
	struct sync_block *next, *next_r;
	void *conn;
	time_t t;
	uint8_t nfield;
	uint8_t ttl;
};

static struct sync_block **g_sync_hash, **g_sync_hash_r;
static pthread_mutex_t g_sync_hash_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_xdag_sync_on = 0;

/* moves the block to the wait list, block with hash written to field 'nfield' of block 'b' is expected 
 (original russian comment was unclear too) */


// called only from this file
// it is pushing sync block in this hashtable system
static int push_block(struct xdag_block *b, void *conn, int nfield, int ttl)
{
	xdag_hash_t hash;
	struct sync_block **p, *q;
	int res;
	time_t t = time(0);

	xdag_hash(b, sizeof(struct xdag_block), hash);
	
	pthread_mutex_lock(&g_sync_hash_mutex);
		// check if the needed block (to load the actual one) is already in list
	for (p = get_list(b->field[nfield].hash), q = *p; q; q = q->next) {
		if (!memcmp(&q->b, b, sizeof(struct xdag_block))) {
			// refreshing the values and exit
			res = (t - q->t >= REQ_PERIOD);
			
			q->conn = conn;
			q->nfield = nfield;
			q->ttl = ttl;
			// if expired it send how much time is it expired
			if (res) q->t = t;
			
			pthread_mutex_unlock(&g_sync_hash_mutex);
			
			return res; // exit
		}
	}

	q = (struct sync_block *)malloc(sizeof(struct sync_block));
	if (!q) return -1;
	
	memcpy(&q->b, b, sizeof(struct xdag_block));
	memcpy(&q->hash, hash, sizeof(xdag_hash_t));
	
	// adding to list and list_r
	
	q->conn = conn;
	q->nfield = nfield; // field where we have the needed block to accept this block
	q->ttl = ttl;
	q->t = t;
	q->next = *p; // the next is the needed block ..!
	
	*p = q; // why = maybe just to initialize? setting the new first in get_list() probably!!
	// setting new first in list_r
	p = get_list_r(hash); // hash of this block BUT IN the list_r! Se 
	
	q->next_r = *p;	// hash of the node in list_r with actual block hash.
	*p = q;
	
	g_xdag_extstats.nwaitsync++;
	
	pthread_mutex_unlock(&g_sync_hash_mutex);
	
	return 1;
}

/* notifies synchronization mechanism about found block */

// this is called after a block is added in our dag
int xdag_sync_pop_block(struct xdag_block *b)
{
	struct sync_block **p, *q, *r;
	xdag_hash_t hash;

	xdag_hash(b, sizeof(struct xdag_block), hash); // just hash of the block
 
begin:
	pthread_mutex_lock(&g_sync_hash_mutex);
	// getlist just give a pointer to a pointer to a sync block that (supposed) to have that hash
	
	
	for (p = get_list(hash); (q = *p); p = &q->next) { // just exploring the list (of that hash...)
		if (!memcmp(hash, q->b.field[q->nfield].hash, sizeof(xdag_hashlow_t))) { // enter if equal (we got the right sync block)
			// just removing the sync block from the list
			*p = q->next;	
			g_xdag_extstats.nwaitsync--;
			
			// just removing that block from the r list
			for (p = get_list_r(q->hash); (r = *p) && r != q; p = &r->next_r);
				
			if (r == q) {
				*p = q->next_r;
			}

			pthread_mutex_unlock(&g_sync_hash_mutex);
			
			q->b.field[0].transport_header = q->ttl << 8 | 1;
			xdag_sync_add_block(&q->b, q->conn);			
			free(q);
			
			goto begin;
		}
	}

	pthread_mutex_unlock(&g_sync_hash_mutex);

	return 0;
}

/* checks a block and includes it in the database with synchronization, ruturs non-zero value in case of error */


// this is called when a block from the network arrive
int xdag_sync_add_block(struct xdag_block *b, void *conn)
{
	int res, ttl = b->field[0].transport_header >> 8 & 0xff;

	res = xdag_add_block(b); // if called from xdag_sync_pop_block it is trying to re-add the block a new time.. why?
	if (res >= 0) {
		// just remove this block from the listes...
		xdag_sync_pop_block(b); // it recall back a new time xdag_sync_pop_block 
		if (res > 0 && ttl > 2) {
			b->field[0].transport_header = ttl << 8;
			xdag_send_packet(b, (void*)((uintptr_t)conn | 1l)); 
		}
	} else if (g_xdag_sync_on && ((res = -res) & 0xf) == 5) { // err from add_nolock is 5 (we are missing a block that have a link to the block we are trying to add)
		res = (res >> 4) & 0xf; //taking the field of the block that failed the check (the field contain the hash of the block that we need).
		if (push_block(b, conn, res, ttl)) { // pushing in our hash table system that we had this issue from that block from that conn
			// we enter if the block wasn't already pushed
			struct sync_block **p, *q; //
			uint64_t *hash = b->field[res].hash; // we get the missing block hash this way
			time_t t = time(0); // actual time

			pthread_mutex_lock(&g_sync_hash_mutex);
 
begin:
			// searching if the block that we want ALREADY need another block !!!
			for (p = get_list_r(hash); (q = *p); p = &q->next_r) {
				if (!memcmp(hash, q->hash, sizeof(xdag_hashlow_t))) {
					if (t - q->t < REQ_PERIOD) {
						pthread_mutex_unlock(&g_sync_hash_mutex);
						return 0; // if already there and it didn't (expire?) exit 
					}

					q->t = t;
					hash = q->b.field[q->nfield].hash;

					goto begin;
				}
			}
				// if we got that we need a block with hash diffeerent than the block that we need,
				// it means we need another block first!
			pthread_mutex_unlock(&g_sync_hash_mutex);
			
			xdag_request_block(hash, (void*)(uintptr_t)1l); // requesting the block
			
			xdag_info("ReqBlk: %016llx%016llx%016llx%016llx", hash[3], hash[2], hash[1], hash[0]);
		}
	}

	return 0;
}

/* initialized block synchronization */
int xdag_sync_init(void)
{	// calloc is just malloc + memset 0
	g_sync_hash = (struct sync_block **)calloc(sizeof(struct sync_block *), SYNC_HASH_SIZE);
	g_sync_hash_r = (struct sync_block **)calloc(sizeof(struct sync_block *), SYNC_HASH_SIZE);

	if (!g_sync_hash || !g_sync_hash_r) return -1; 	// error check

	return 0;
}
