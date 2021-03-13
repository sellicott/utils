/*
 * ringbuf.c - C ring buffer (FIFO) implementation.
 *
 * Written in 2011 by Drew Hess <dhess-src@bothan.net>.
 * Modified in 2021 by Sam Ellicott <sellicott@cedarville.edu>
 *
 * Original code licensed under the CC0 Public Domain Dedication
 * <http://creativecommons.org/publicdomain/zero/1.0/>,
 * Modified code is licensed under the MIT licence
 * <https://mit-license.org/>.
 */

#include "ringbuf.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * The code is written for clarity, not cleverness or performance, and
 * contains many assert()s to enforce invariant assumptions and catch
 * bugs. Feel free to optimize the code and to remove asserts for use
 * in your own projects, once you're comfortable that it functions as
 * intended.
 */

/*
 * The struct is setup such that back always points to a position just
 * after where data is stored. Additionally, the buffer size holds one
 * extra byte as a sentinel value so that the empty and full conditions can be
 * distinguished.
 */
struct ringbuf_t
{
	uint8_t *buf;
	uint8_t *front, *back;
	size_t capacity;
};

ringbuf_t *ringbuf_new( size_t capacity )
{
	ringbuf_t *rb = malloc( sizeof( ringbuf_t ) );
	if ( rb )
	{

		/* One byte is used for detecting the full condition. */
		rb->capacity = capacity + 1;
		rb->buf = malloc( rb->capacity );
		if ( rb->buf )
			ringbuf_reset( rb );
		else
		{
			free( rb );
			return NULL;
		}
	}
	return rb;
}

size_t ringbuf_buffer_capacity( const ringbuf_t *rb )
{
	return rb->capacity - 1;
}

void ringbuf_reset( ringbuf_t *rb )
{
	rb->front = rb->back = rb->buf;
}

void ringbuf_free( ringbuf_t *rb )
{
	assert( rb );
	free( rb->buf );
	free( rb );
	rb = NULL;
}

/*
 * Return a pointer to one-past-the-end of the ring buffer's
 * contiguous buffer. You shouldn't normally need to use this function
 * unless you're writing a new ringbuf_* function.
 */
static const uint8_t *ringbuf_end( const ringbuf_t *rb )
{
	return rb->buf + rb->capacity;
}

size_t ringbuf_bytes_free( const ringbuf_t *rb )
{
	assert( rb );
	if ( rb->back > rb->front )
	{
		return rb->capacity - ( rb->back - rb->front ) - 1;
	}
	else
	{
		return rb->front - rb->back - 1;
	}
}

size_t ringbuf_bytes_used( const ringbuf_t *rb )
{
	return rb->size;
}

int ringbuf_is_full( const ringbuf_t *rb )
{
	return rb->size == 0;
}

int ringbuf_is_empty( const ringbuf_t *rb )
{
	return rb->size == rb->capacity;
}

const void *ringbuf_back( const ringbuf_t *rb )
{
	return rb->back;
}

const void *ringbuf_front( const struct ringbuf_t *rb )
{
	return rb->front;
}

/*
 * Given a ring buffer rb and a pointer to a location within its
 * contiguous buffer, return the a pointer to the next logical
 * location in the ring buffer.
 */
static uint8_t *ringbuf_nextp( ringbuf_t *rb, const uint8_t *p )
{
	/*
	 * The assert guarantees the expression (++p - rb->buf) is
	 * non-negative; therefore, the modulus operation is safe and
	 * portable.
	 */
	assert( ( p >= rb->buf ) && ( p < ringbuf_end( rb ) ) );
	size_t back_offset = ( ++p - rb->buf ) % rb->capacity;
	return rb->buf + back_offset;
}

size_t ringbuf_memset( ringbuf_t *rb, int c, size_t len )
{
	const uint8_t *bufend = ringbuf_end( rb );
	size_t nwritten = 0;
	size_t count = MIN( len, ringbuf_buffer_size( rb ) );
	int overflow = count > ringbuf_bytes_free( rb );

	while ( nwritten != count )
	{

		/* don't copy beyond the end of the buffer */
		assert( bufend > rb->head );
		size_t n = MIN( bufend - rb->head, count - nwritten );
		memset( rb->head, c, n );
		rb->head += n;
		nwritten += n;

		/* wrap? */
		if ( rb->head == bufend )
			rb->head = rb->buf;
	}

	if ( overflow )
	{
		rb->tail = ringbuf_nextp( rb, rb->head );
		assert( ringbuf_is_full( rb ) );
	}

	return nwritten;
}

void *ringbuf_push_back( ringbuf_t *rb, const void *src, size_t count )
{
	const uint8_t *u8src = src;
	const uint8_t *bufend = ringbuf_end( rb );
	int overflow = count > ringbuf_bytes_free( rb );
	size_t nread = 0;

	while ( nread != count )
	{
		/* don't copy beyond the end of the buffer */
		assert( bufend > rb->head );
		size_t n = MIN( bufend - rb->head, count - nread );
		memcpy( rb->head, u8src + nread, n );
		rb->head += n;
		nread += n;

		/* wrap? */
		if ( rb->head == bufend )
			rb->head = rb->buf;
	}

	if ( overflow )
	{
		rb->tail = ringbuf_nextp( rb, rb->head );
		assert( ringbuf_is_full( rb ) );
	}

	return rb->head;
}

void *ringbuf_memcpy_from( void *rb, ringbuf_t *src, size_t count )
{
	size_t bytes_used = ringbuf_bytes_used( src );
	if ( count > bytes_used )
		return 0;

	uint8_t *u8dst = rb;
	const uint8_t *bufend = ringbuf_end( src );
	size_t nwritten = 0;
	while ( nwritten != count )
	{
		assert( bufend > src->tail );
		size_t n = MIN( bufend - src->tail, count - nwritten );
		memcpy( u8dst + nwritten, src->tail, n );
		src->tail += n;
		nwritten += n;

		/* wrap ? */
		if ( src->tail == bufend )
			src->tail = src->buf;
	}

	assert( count + ringbuf_bytes_used( src ) == bytes_used );
	return src->tail;
}

void *ringbuf_copy( ringbuf_t *dst, ringbuf_t *src, size_t count )
{
	size_t src_bytes_used = ringbuf_bytes_used( src );
	if ( count > src_bytes_used )
		return 0;
	int overflow = count > ringbuf_bytes_free( dst );

	const uint8_t *src_bufend = ringbuf_end( src );
	const uint8_t *dst_bufend = ringbuf_end( dst );
	size_t ncopied = 0;
	while ( ncopied != count )
	{
		assert( src_bufend > src->tail );
		size_t nsrc = MIN( src_bufend - src->tail, count - ncopied );
		assert( dst_bufend > dst->head );
		size_t n = MIN( dst_bufend - dst->head, nsrc );
		memcpy( dst->head, src->tail, n );
		src->tail += n;
		dst->head += n;
		ncopied += n;

		/* wrap ? */
		if ( src->tail == src_bufend )
			src->tail = src->buf;
		if ( dst->head == dst_bufend )
			dst->head = dst->buf;
	}

	assert( count + ringbuf_bytes_used( src ) == src_bytes_used );

	if ( overflow )
	{
		dst->tail = ringbuf_nextp( dst, dst->head );
		assert( ringbuf_is_full( dst ) );
	}

	return dst->head;
}
