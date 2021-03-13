#ifndef INCLUDED_RINGBUF_H
#define INCLUDED_RINGBUF_H

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

/*
 * A byte-addressable ring buffer FIFO implementation.
 *
 * The ring buffer's head pointer points to the starting location
 * where data should be written when copying data *into* the buffer
 * (e.g., with ringbuf_read). The ring buffer's tail pointer points to
 * the starting location where data should be read when copying data
 * *from* the buffer (e.g., with ringbuf_write).
 */

#include <stddef.h>
#include <sys/types.h>

typedef struct ringbuf_t ringbuf_t;

/*
 * Create a new ring buffer with the given capacity (usable
 * bytes). Note that the actual internal buffer size may be one or
 * more bytes larger than the usable capacity, for bookkeeping.
 *
 * Returns the new ring buffer object, or 0 if there's not enough
 * memory to fulfill the request for the given capacity.
 */
ringbuf_t *ringbuf_new( size_t capacity );

/*
 * The capacity of the internal buffer, in bytes.
 *
 * For the usable capacity of the ring buffer, use the
 * ringbuf_capacity function.
 */
size_t ringbuf_buffer_capacity( const ringbuf_t *rb );

/*
 * Deallocate a ring buffer, and, as a side effect, set the pointer to
 * 0.
 */
void ringbuf_free( ringbuf_t *rb );

/*
 * Reset a ring buffer to its initial state (empty).
 */
void ringbuf_reset( ringbuf_t *rb );

/*
 * The usable capacity of the ring buffer, in bytes. Note that this
 * value may be less than the ring buffer's internal buffer size, as
 * returned by ringbuf_buffer_size.
 */
size_t ringbuf_capacity( const ringbuf_t *rb );

/*
 * The number of free/available bytes in the ring buffer. This value
 * is never larger than the ring buffer's usable capacity.
 */
size_t ringbuf_bytes_free( const ringbuf_t *rb );

/*
 * The number of bytes currently being used in the ring buffer. This
 * value is never larger than the ring buffer's usable capacity.
 */
size_t ringbuf_bytes_used( const ringbuf_t *rb );

int ringbuf_is_full( const ringbuf_t *rb );

int ringbuf_is_empty( const ringbuf_t *rb );

/*
 * Const access to the head and tail pointers of the ring buffer.
 */
const void *ringbuf_front( const ringbuf_t *rb );

const void *ringbuf_back( const ringbuf_t *rb );


/*
 * Beginning at ring buffer rb's front pointer, fill the ring buffer
 * with a repeating sequence of len bytes, each of value c (converted
 * to an unsigned char). len can be as large as you like, but the
 * function will never write more than ringbuf_buffer_size(dst) bytes
 * in a single invocation, since that size will cause all bytes in the
 * ring buffer to be written exactly once each.
 *
 * Note that if len is greater than the number of free bytes in dst,
 * the ring buffer will overflow. When an overflow occurs, the state
 * of the ring buffer is guaranteed to be consistent, including the
 * head and tail pointers; old data will simply be overwritten in FIFO
 * fashion, as needed. However, note that, if calling the function
 * results in an overflow, the value of the ring buffer's tail pointer
 * may be different than it was before the function was called.
 *
 * Returns the actual number of bytes written to dst: len, if
 * len < ringbuf_buffer_size(dst), else ringbuf_buffer_size(dst).
 */
size_t ringbuf_memset( ringbuf_t *rb, int c, size_t len );

/*
 * Copy n bytes from a contiguous memory area src into the ring buffer
 * dst. Returns the ring buffer's new front pointer.
 *
 * It is possible to copy more data from src than is available in the
 * buffer; i.e., it's possible to overflow the ring buffer using this
 * function. When an overflow occurs, the state of the ring buffer is
 * guaranteed to be consistent, including the head and tail pointers;
 * old data will simply be overwritten in FIFO fashion, as
 * needed. However, note that, if calling the function results in an
 * overflow, the value of the ring buffer's front pointer may be
 * different than it was before the function was called.
 */

void *ringbuf_push_back( ringbuf_t *rb, const void *src, size_t count );

/*
 * Copy n bytes from the ring buffer src, starting from its front
 * pointer, into a contiguous memory area dst. Returns the value of
 * rb's front pointer after the copy is finished.
 *
 * Note that this copy is destructive with respect to the ring buffer:
 * the n bytes copied from the ring buffer are no longer available in
 * the ring buffer after the copy is complete, and the ring buffer
 * will have n more free bytes than it did before the function was
 * called.
 *
 * This function will *not* allow the ring buffer to underflow. If
 * count is greater than the number of bytes used in the ring buffer,
 * no bytes are copied, and the function will return NULL.
 */
void *ringbuf_pop_front( void *out, ringbuf_t *rb, size_t count );

/*
 * Copy count bytes from ring buffer src, starting from its tail
 * pointer, into ring buffer dst. Returns dst's new head pointer after
 * the copy is finished.
 *
 * Note that this copy is destructive with respect to the ring buffer
 * src: any bytes copied from src into dst are no longer available in
 * src after the copy is complete, and src will have 'count' more free
 * bytes than it did before the function was called.
 *
 * It is possible to copy more data from src than is available in dst;
 * i.e., it's possible to overflow dst using this function. When an
 * overflow occurs, the state of dst is guaranteed to be consistent,
 * including the head and tail pointers; old data will simply be
 * overwritten in FIFO fashion, as needed. However, note that, if
 * calling the function results in an overflow, the value dst's tail
 * pointer may be different than it was before the function was
 * called.
 *
 * It is *not* possible to underflow src; if count is greater than the
 * number of bytes used in src, no bytes are copied, and the function
 * returns 0.
 */
void *ringbuf_copy( ringbuf_t *dst, ringbuf_t *src, size_t count );

// Include the implementation for a "header only" version of the library
#ifdef RINGBUF_IMPLEMENTATION
#include "ringbuf.c"
#endif

#endif /* INCLUDED_RINGBUF_H */
