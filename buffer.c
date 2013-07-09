/*
 * Copyright (c) 2013 Ahmed Samy  <f.fallen45@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "buffer.h"

typedef struct buffer {
	int wpos, rpos, size;
	uint8_t *data;
} buffer_t;

static __inline uint16_t u16(unsigned char *addr)
{
	return addr[1] << 8 | addr[0];
}

static __inline uint32_t u32(unsigned char *addr)
{
	return u16(addr + 2) << 16 | u16(addr);
}

static bool stat_file(const char *f, int *size)
{
	struct stat st;
	if (stat(f, &st) != 0)
		return false;
	*size = st.st_size;
	return true;
}

static void bresize(buffer_t *p, int nsize)
{
	uint8_t *n;
	if ((n = malloc(nsize)) != NULL) {
		p->size = nsize;
		memcpy(n, p->data, p->wpos);

		if (p->data)
			free(p->data);

		p->data = n;
		memset(p->data, 0, p->size - p->wpos);
	}
}

uint32_t bget32(buffer_t *bp)
{
	uint32_t tmp = u32(&bp->data[bp->rpos]);
	bp->rpos += 4;
	return tmp;
}

uint16_t bget16(buffer_t *bp)
{
	uint16_t tmp = u16(&bp->data[bp->rpos]);
	bp->rpos += 2;
	return tmp;
}

uint8_t bgetc(buffer_t *bp)
{
	uint8_t tmp = bp->data[bp->rpos];
	bp->rpos+=1;
	return tmp;
}

int btell(buffer_t *bp)
{
	return bp->rpos;
}

void badd(buffer_t *buffer, uint8_t byte)
{
	if (buffer->wpos+1 > buffer->size)
		bresize(buffer, (buffer->size * 3) / 2 + 1);

	buffer->data[buffer->wpos] = byte;
	++buffer->wpos;
}

void bskip(buffer_t *buffer, int size)
{
	if (buffer->rpos + size > buffer->size)
		return;
	buffer->rpos += size;
}

void bseek(buffer_t *buffer, int pos)
{
	if (pos > buffer->size)
		return;
	buffer->rpos = pos;
}

buffer_t *balloc(int bsize)
{
	buffer_t *b;
	if (!(b = malloc(sizeof(*b))))
		abort();

	if (!(b->data = malloc(bsize)))
		abort();

	memset(b->data, 0, bsize);
	b->size  = bsize;
	b->wpos  = b->rpos = 0;
	return b;
}

buffer_t* balloc_fp(const char* f)
{
	FILE *fp;
	buffer_t *res;
	int fsize;

	if (!stat_file(f, &fsize)) {
		fprintf(stderr, "could not stat %s\n", f);
		return NULL;
	}

	fp = fopen(f, "rb");
	if (!fp) {
		fprintf(stderr, "could not open file %s for caching %s\n", f, strerror(errno));
		return NULL;
	}

	res = balloc(fsize);
	if (!res) {
		fprintf(stderr, "could not allocate buffer for %s with size %d\n", f, fsize);
		goto out;
	}

	if (fread(res->data, 1, res->size, fp) != res->size) {
		fprintf(stderr, "could not cache file %s of size %d\n", f, res->size);
		bfree(&res);
		goto out;
	}

out:
	fclose(fp);
	return res;
}

void bfree(buffer_t **b)
{
	free((*b)->data);
	free(*b);
	*b = NULL;
}

