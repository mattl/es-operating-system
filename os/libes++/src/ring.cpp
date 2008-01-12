/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include <string.h>
#include <es.h>
#include <es/ring.h>

long Ring::
peek(void* dst, long count, long offset) const
{
    ASSERT(0 <= offset);
    if (used < offset + count)
    {
        count = used - offset;
    }
    if (count <= 0)
    {
        return 0;
    }
    ASSERT(offset < used);

    u8* ptr = static_cast<u8*>(dst);
    const u8* end = buf + size;
    ASSERT(buf <= head && head < end);

    const u8* src = head + offset;
    if (end <= src)
    {
        src = buf + (offset - (end - src));
    }
    ASSERT(buf <= src && src < end);

    if (src + count < end)
    {
        //  buf      src            tail    end
        //  |        |              |       |
        //  +--------XXXXXXXXXXXXXXXX-------+
        //  |        |<- count ->|          |
        memmove(ptr, src, count);
    }
    else    // (end <= src + count)
    {
        //  buf      tail     src           end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  +->|              |<-- count ---+
        //
        //                or
        //
        //  buf      tail     src           end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  |                 |<-- count -->|
        long snip = end - src;
        ASSERT(snip <= count);
        memmove(ptr, src, snip);
        memmove(ptr + snip, buf, count - snip);
    }
    return count;
}

long Ring::
read(void* dst, long count)
{
    u8* ptr(static_cast<u8*>(dst));
    u8* end;

    if (used < count)
    {
        count = used;
    }
    if (count <= 0)
    {
        return 0;
    }

    end = buf + size;
    ASSERT(buf <= head && head < end);

    if (head + count < end)
    {
        //  buf      head         tail      end
        //  |        |            |         |
        //  +--------XXXXXXXXXXXXX----------+
        //  |        |<- count ->|            |
        memmove(ptr, head, count);
        head += count;
    }
    else    // (end <= head + count)
    {
        //  buf      tail     head         end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  +->|              |<-- count ---+
        //
        //                or
        //
        //  buf      tail     head         end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  |                 |<-- count -->|
        long snip = end - head;
        ASSERT(snip <= count);
        memmove(ptr, head, snip);
        memmove(ptr + size, buf, count - snip);
        head = buf + count - snip;
    }
    ASSERT(buf <= head && head < end);
    used -= count;
    return count;
}

long Ring::
discard(long count)
{
    u8* end;

    if (used < count)
    {
        count = used;
    }
    if (count <= 0)
    {
        return 0;
    }

    end = buf + size;
    ASSERT(buf <= head && head < end);

    if (head + count < end)
    {
        //  buf      head         tail      end
        //  |        |            |         |
        //  +--------XXXXXXXXXXXXX----------+
        //  |        |<- count ->|            |
        head += count;
    }
    else    // (end <= head + count)
    {
        //  buf      tail     head         end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  +->|              |<-- count ---+
        //
        //                or
        //
        //  buf      tail     head         end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        //  |                 |<-- count -->|
        long snip = end - head;
        ASSERT(snip <= count);
        head = buf + count - snip;
    }
    ASSERT(buf <= head && head < end);
    used -= count;
    return count;
}

long Ring::
write(const void* src, long count)
{
    const u8* ptr(static_cast<const u8*>(src));
    u8* end;
    u8* tail;
    long free;

    if (size < used + count)
    {
        count = size - used;
    }
    if (count <= 0)
    {
        return 0;
    }
    end = buf + size;
    ASSERT(buf <= head && head < end);
    tail = head + used;
    if (end <= tail)
    {
        tail -= size;
    }

    if (head <= tail)
    {
        //  buf      head         tail      end
        //  |        |            |         |
        //  +--------XXXXXXXXXXXXX----------+
        free = end - tail;
        if (count <= free)
        {
            memmove(tail, ptr, count);
        }
        else
        {
            memmove(tail, ptr, free);
            memmove(buf, ptr + free, count - free);
        }
    }
    else // (tail < head)
    {
        //  buf      tail     head         end
        //  |        |        |             |
        //  XXXXXXXXX---------XXXXXXXXXXXXXXX
        memmove(tail, ptr, count);
    }
    used += count;
    return count;
}

long Ring::
marge(u8* adv, long count, Vec* blocks, long maxblock, u8* tail)
{
    Vec* block;
    Vec* end;
    long pl;    // pos left
    long pr;    // pos right
    long pb;    // pos block

    ASSERT(1 < maxblock && blocks);
    ASSERT(0 <= count);

    pl = pos(tail, adv);
    pr = pl + count;

    end = &blocks[maxblock];

    if (tail == adv)
    {
        for (block = blocks; block < end && block->data; )
        {
            pb = pos(tail, (u8*) block->data);
            if (pb <= pr)
            {
                pr = max(pb + block->count, pr);
                count = pr - pl;
                // Delete block
                memmove(block, block + 1, (u8*) end - (u8*) (block + 1));
                memset(end - 1, 0, sizeof(Vec));
                continue;
            }
            ++block;
        }
        used += count;
        return count;
    }

    // tail != adv
    for (block = blocks; block < end && block->data; )
    {
        pb = pos(tail, (u8*) block->data);
        if (pl <= pb + block->count && pb <= pr)
        {
            pr = max(pb + block->count, pr);
            if (pb < pl)
            {
                pl = pb;
                adv = (u8*) block->data;
            }
            count = pr - pl;
            // Delete block
            memmove(block, block + 1, (u8*) end - (u8*) (block + 1));
            memset(end - 1, 0, sizeof(Vec));
            continue;
        }
        ++block;
    }
    if (block < end)
    {
        ASSERT(block->data == NULL);
        block->data = adv;
        block->count = count;
    }
    else
    {
        // Append new block after removing the oldest one
        memmove(blocks, blocks + 1, (u8*) end - (u8*) (blocks + 1));
        block = end - 1;
        block->data = adv;
        block->count = count;
    }
    return 0;
}

long Ring::
write(const void* src, long count, long offset, Vec* blocks, long maxblock)
{
    const u8* ptr(static_cast<const u8*>(src));
    u8* end;
    u8* tail;
    u8* adv;
    long free;

    ASSERT(0 <= offset);
    ASSERT(offset <= size);
    if (size < used + offset + count)
    {
        count = size - used - offset;
    }
    if (count <= 0)
    {
        return 0;
    }
    ASSERT(used + offset + count <= size);
    end = buf + size;
    ASSERT(buf <= head && head < end);
    tail = head + used;
    if (end <= tail)
    {
        tail -= size;
    }

    adv = tail + offset;
    if (end <= adv)
    {
        adv -= size;
    }

    if (head <= adv)
    {
        //  buf      head    tail adv      end
        //  |        |       |    |         |
        //  +--------XXXXXXXXOOOOO----------+
        free = end - adv;
        if (count <= free)
        {
            memmove(adv, ptr, count);
        }
        else
        {
            memmove(adv, ptr, free);
            memmove(buf, ptr + free, count - free);
        }
    }
    else // (adv < head)
    {
        //  buf tail adv      head         end
        //  |   |    |        |             |
        //  XXXXOOOOO---------XXXXXXXXXXXXXXX
        memmove(adv, ptr, count);
    }

    return marge(adv, count, blocks, maxblock, tail);
}
