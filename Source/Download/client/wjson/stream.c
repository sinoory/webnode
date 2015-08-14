  /*
 * Packet interface
 * Copyright (C) 1999 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include "zebra.h"


/* Tests whether a position is valid */ 
#define GETP_VALID(S,G) \
  ((G) <= (S)->endp)
#define PUT_AT_VALID(S,G) GETP_VALID(S,G)
#define ENDP_VALID(S,E) \
  ((E) <= (S)->size)

/* asserting sanity checks. Following must be true before
 * stream functions are called:
 *
 * Following must always be true of stream elements
 * before and after calls to stream functions:
 *
 * getp <= endp <= size
 *
 * Note that after a stream function is called following may be true:
 * if (getp == endp) then stream is no longer readable
 * if (endp == size) then stream is no longer writeable
 *
 * It is valid to put to anywhere within the size of the stream, but only
 * using stream_put..._at() functions.
 */
 #undef zlog_warn
#define    zlog_warn( fmt, args... )
 
#define STREAM_WARN_OFFSETS(S) \
  zlog_warn ("&(struct stream): %p, size: %lu, getp: %lu, endp: %lu\n", \
             (S), \
             (unsigned long) (S)->size, \
             (unsigned long) (S)->getp, \
             (unsigned long) (S)->endp)\

#define STREAM_VERIFY_SANE(S) \
  do { \
    if ( !(GETP_VALID(S, (S)->getp)) && ENDP_VALID(S, (S)->endp) ) \
      { STREAM_WARN_OFFSETS(S); }\
    assert ( GETP_VALID(S, (S)->getp) ); \
    assert ( ENDP_VALID(S, (S)->endp) ); \
  } while (0)

#define STREAM_BOUND_WARN(S, WHAT) \
  do { \
    zlog_warn ("%s: Attempt to %s out of bounds", __func__, (WHAT)); \
    STREAM_WARN_OFFSETS(S); \
    assert (0); \
  } while (0)

/* XXX: Deprecated macro: do not use */
#define CHECK_SIZE(S, Z) \
  do { \
    if (((S)->endp + (Z)) > (S)->size) \
      { \
        zlog_warn ("CHECK_SIZE: truncating requested size %lu\n", \
                   (unsigned long) (Z)); \
        STREAM_WARN_OFFSETS(S); \
        (Z) = (S)->size - (S)->endp; \
      } \
  } while (0);

/* Make stream buffer. */
struct stream *
stream_new (size_t size)
{
  struct stream *s;

  assert (size > 0);
  
  if (size == 0)
    {
      zlog_warn ("stream_new(): called with 0 size!");
      return NULL;
    }
  
  s = XCALLOC (MTYPE_STREAM, sizeof (struct stream));

  if (s == NULL)
    return s;
  
  if ( (s->data = XMALLOC (MTYPE_STREAM_DATA, size)) == NULL)
    {
      XFREE (MTYPE_STREAM, s);
      return NULL;
    }
  
  s->size = size;
  return s;
}

/* Free it now. */
void
stream_free (struct stream *s)
{
  if (!s)
    return;
  
  XFREE (MTYPE_STREAM_DATA, s->data);
  XFREE (MTYPE_STREAM, s);
}

struct stream *
stream_copy (struct stream *new, struct stream *src)
{
  STREAM_VERIFY_SANE (src);
  
  assert (new != NULL);
  assert (STREAM_SIZE(new) >= src->endp);

  new->endp = src->endp;
  new->getp = src->getp;
  
  memcpy (new->data, src->data, src->endp);
  
  return new;
}

struct stream *
stream_dup (struct stream *s)
{
  struct stream *new;

  STREAM_VERIFY_SANE (s);

  if ( (new = stream_new (s->endp)) == NULL)
    return NULL;

  return (stream_copy (new, s));
}

size_t
stream_resize (struct stream *s, size_t newsize)
{
  u_char *newdata;
  STREAM_VERIFY_SANE (s);
  
  newdata = XREALLOC (MTYPE_STREAM_DATA, s->data, newsize);
  
  if (newdata == NULL)
    return s->size;
  
  s->data = newdata;
  s->size = newsize;
  
  if (s->endp > s->size)
    s->endp = s->size;
  if (s->getp > s->endp)
    s->getp = s->endp;
  
  STREAM_VERIFY_SANE (s);
  
  return s->size;
}


size_t
stream_get_endp (struct stream *s)
{
  STREAM_VERIFY_SANE(s);
  return s->endp;
}

size_t
stream_get_size (struct stream *s)
{
  STREAM_VERIFY_SANE(s);
  return s->size;
}

/* Stream structre' stream pointer related functions.  */
void
stream_set_getp (struct stream *s, size_t pos)
{
  STREAM_VERIFY_SANE(s);
  
  if (!GETP_VALID (s, pos))
    {
      STREAM_BOUND_WARN (s, "set getp");
      pos = s->endp;
    }

  s->getp = pos;
}

void
stream_set_endp (struct stream *s, size_t pos)
{
  STREAM_VERIFY_SANE(s);

  if (!GETP_VALID (s, pos))
    {
      STREAM_BOUND_WARN (s, "set endp");
      pos = s->endp;
    }

  s->endp = pos;
}

/* Forward pointer. */
void
stream_forward_getp (struct stream *s, size_t size)
{
  STREAM_VERIFY_SANE(s);
  
  if (!GETP_VALID (s, s->getp + size))
    {
      STREAM_BOUND_WARN (s, "seek getp");
      return;
    }
  
  s->getp += size;
}

void
stream_forward_endp (struct stream *s, size_t size)
{
  STREAM_VERIFY_SANE(s);
  
  if (!ENDP_VALID (s, s->endp + size))
    {
      STREAM_BOUND_WARN (s, "seek endp");
      return;
    }
  
  s->endp += size;
}

/* Copy from stream to destination. */
void
stream_get (void *dst, struct stream *s, size_t size)
{
  STREAM_VERIFY_SANE(s);
  
  if (STREAM_READABLE(s) < size)
    {
      STREAM_BOUND_WARN (s, "get");
      return;
    }
  
  memcpy (dst, s->data + s->getp, size);
  s->getp += size;
}

/* Get next character from the stream. */
u_char
stream_getc (struct stream *s)
{
  u_char c;
  
  STREAM_VERIFY_SANE (s);

  if (STREAM_READABLE(s) < sizeof (u_char))
    {
      STREAM_BOUND_WARN (s, "get char");
      return 0;
    }
  c = s->data[s->getp++];
  
  return c;
}

/* Get next character from the stream. */
u_char
stream_getc_from (struct stream *s, size_t from)
{
  u_char c;

  STREAM_VERIFY_SANE(s);
  
  if (!GETP_VALID (s, from + sizeof (u_char)))
    {
      STREAM_BOUND_WARN (s, "get char");
      return 0;
    }
  
  c = s->data[from];
  
  return c;
}

/* Copy to source to stream.
 *
 * XXX: This uses CHECK_SIZE and hence has funny semantics -> Size will wrap
 * around. This should be fixed once the stream updates are working.
 *
 * stream_write() is saner
 */
void
stream_put (struct stream *s, const void *src, size_t size)
{

  /* XXX: CHECK_SIZE has strange semantics. It should be deprecated */
  CHECK_SIZE(s, size);
  
  STREAM_VERIFY_SANE(s);
  
  if (STREAM_WRITEABLE (s) < size)
    {
      STREAM_BOUND_WARN (s, "put");
      return;
    }
  
  if (src)
    memcpy (s->data + s->endp, src, size);
  else
    memset (s->data + s->endp, 0, size);

  s->endp += size;
}

/* Put character to the stream. */
int
stream_putc (struct stream *s, u_char c)
{
  STREAM_VERIFY_SANE(s);
  
  if (STREAM_WRITEABLE (s) < sizeof(u_char))
    {
      STREAM_BOUND_WARN (s, "put");
      return 0;
    }
  
  s->data[s->endp++] = c;
  return sizeof (u_char);
}


/* Check does this stream empty? */
int
stream_empty (struct stream *s)
{
  STREAM_VERIFY_SANE(s);

  return (s->endp == 0);
}

/* Reset stream. */
void
stream_reset (struct stream *s)
{
  STREAM_VERIFY_SANE (s);

  s->getp = s->endp = 0;
}

/* Write stream contens to the file discriptor. */
int
stream_flush (struct stream *s, int fd)
{
  int nbytes;
  
  STREAM_VERIFY_SANE(s);
  
  nbytes = write (fd, s->data + s->getp, s->endp - s->getp);
  
  return nbytes;
}

