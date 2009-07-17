/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NINTENDO_ES_LIST_H_INCLUDED
#define NINTENDO_ES_LIST_H_INCLUDED

//
//                       List<>
//                      +--------+
//        +-------------|  head  |
//        |             +--------+
//        |             |  tail  |------------+
//        |             +--------+            |
//        |                                   |
//        |     elm                elm        |
//        +--->+--------+  ---   +--------+<--+
//             |        |   A    |        |
//             |        |   |    |        |
//             |        | offset |        |
//             |        |   |    |        |
//             | Link<> |   V    | Link<> |
//             +--------+  ---   +--------+
//        0 <--|  prev  |<-------|  prev  |
//             +--------+        +--------+
//             |  next  |------->|  next  |--> 0
//             +--------+        +--------+
//             |        |        |        |
//             |        |        |        |
//             +--------+        +--------+
//                  A                 A
//                  |                 |
//                  |   +--------+    |
//                  +---|  pre   |    |
//                      +--------+    |
//                      |  suc   |----+
//                      +--------+
//                      |  cur   |
//                      +--------+
//                       List<>::Iterator
//
// Figure:  Relationship between List<>, Link<>, List<>::Iterator,
//          elm, and offset.
//
//          Note cur points to the last elm returned by
//          Next() or Previous().
//

/** A template for a link embodied in elements in a linked list.
 * <p><code>Link&lt;elm&gt;</code></p>
 * @see List
 */
template <typename elm>
struct Link
{
    elm* prev;
    elm* next;

    /** Initializes this link
     */
    Link() throw();
};

template <typename elm>
inline Link<elm>::
Link() throw()
{
    prev = next = 0;
}

/** A template for a linked list implementation.
 * <p><code>ListBase&lt;elm&gt;</code></p>
 * @see List
 */
template <typename elm>
struct ListBase
{
    elm* head;
    elm* tail;
};

/** A template for a linked list implementation.
 * <p><code>List&lt;elm, &amp;elm::offset&gt;</code></p>
 * The <code>List</code>
 * template has the first template parameter, <code>elm</code>, which
 * specifies the type of the objects to be linked, and the second template
 * parameter, <code>offset</code>, which specifies the offset to the object
 * member of type <code>Link</code>, which is used to link objects.
 * @see Link
 */
template <typename elm, Link<elm> elm::* offset>
class List : protected ListBase<elm>
{
public:
    class Iterator;

    /** Constructs an empty list */
    List() throw();

    /** Returns the first element in this list.
     * @return          the first element in this list.
     */
    elm* getFirst() const;

    /** Returns the last element in this list.
     * @return          the last element in this list.
     */
    elm* getLast() const;

    /** Returns true if this collection contains no elements.
     * @return          <code>true</code> if this list contains no elements.
     */
    bool isEmpty() const;

    /** Appends all of the elements in the specified list to the end of this
     * list.
     * @param list      the list of elements to be inserted into this list.
     * @return          true if this list is changed
     *
     */
    bool addAll(List* list);

    /** Inserts the given element at the beginning of this list.
     * @param item      the element to be inserted at the beginning of this list.
     */
    void addFirst(elm* item);

    /** Inserts the given element at the end of this list.
     * @param item      the element to be inserted at the end of this list.
     */
    void addLast(elm* item);

    /** Removes the specified element in this list.
     * @param item      element to be removed from this list.
                        <code>item</code> must have been linked to this list.
     * @return          the removed element from this list.
     */
    elm* remove(elm* item);

    /** Removes and returns the first element from this list.
     * @return          the first element from this list.
     */
    elm* removeFirst();

    /** Removes and returns the last element from this list.
     * @return          the last element from this list.
     */
    elm* removeLast();

    /** Returns an iterator of the elements in this list starting at the
     * beginning of the list.
     * @return          an <code>Iterator</code> of the elements in this
     *                  list starting at the beginning of the list.
     */
    Iterator begin();

    /** Returns an iterator of the elements in this list starting at the
     * end of the list.
     * @return          an <code>Iterator</code> of the elements in this
     *                  list starting at the end of the list.
     */
    Iterator end();


    /** Returns true if the list contains the specified element.
     * @param           The element to test its presence.
     * @return          <code>true</code> if the list contains the specified element.
     */
    bool contains(elm* item);

    /** Returns an iterator of the elements in this list starting from
     * the specified item.
     * @return          an <code>Iterator</code> of the elements in this
     *                  list starting from the specified item of the list.
     */
    Iterator list(elm* item);

private:
    void addAfter(elm* before, elm* item);
    void addBefore(elm* item, elm* after);
    friend class Iterator;
};

/** An iterator implementation over a <code>List</code>.
 * @see List
 */
template <typename elm, Link<elm> elm::* offset>
class List<elm, offset>::Iterator
{
    friend class List<elm, offset>;

    List<elm, offset>& list;
    elm* pre;
    elm* suc;
    elm* cur;

private:
    Iterator(List<elm, offset>& list, elm* pre, elm* suc) throw();
    Iterator(List<elm, offset>& list, elm* cur) throw();

public:
    Iterator& operator=(const Iterator& other)
    {
        list = other.list;
        pre = other.pre;
        suc = other.suc;
        cur = other.cur;
        return *this;
    }

    /** Returns <code>true</code> if the iteration has more elements in the
     * forward direction.
     * @return          <code>true</code> if the iterator has more elements.
     */
    bool hasNext() const;

    /** Returns the next element in the list.
     * @return          The next element in the list.
     */
    elm* next();

    /** Returns <code>true</code> if the iteration has more elements in the
     * reverse direction.
     * @return          <code>true</code> if the iterator has more elements.
     */
    bool hasPrevious() const;

    /** Returns the previous element in the list.
     * @return          The previous element in the list.
     */
    elm* previous();

    /** Removes the last element returned by the iterator from the collection.
     */
    elm* remove();

    /** Inserts the specified element into the list.
     * @param           The element to insert.
     */
    void add(elm* item);
};

template <typename elm, Link<elm> elm::* offset>
inline List<elm, offset>::
List() throw()
{
    this->head = this->tail = 0;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::
getFirst() const
{
    return this->head;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::
getLast() const
{
    return this->tail;
}

template <typename elm, Link<elm> elm::* offset>
inline bool List<elm, offset>::
isEmpty() const
{
    return !this->head ? true : false;
}

template <typename elm, Link<elm> elm::* offset>
inline void List<elm, offset>::
addAfter(elm* before, elm* item)
{
    (item->*offset).prev = before;
    (item->*offset).next = (before->*offset).next;
    (before->*offset).next = item;
    if (!(item->*offset).next)
    {
        this->tail = item;
    }
    else
    {
        (((item->*offset).next)->*offset).prev = item;
    }
}

template <typename elm, Link<elm> elm::* offset>
inline void List<elm, offset>::
addBefore(elm* item, elm* after)
{
    (item->*offset).prev = (after->*offset).prev;
    (item->*offset).next = after;
    (after->*offset).prev = item;
    if (!(item->*offset).prev)
    {
        this->head = item;
    }
    else
    {
        (((item->*offset).prev)->*offset).next = item;
    }
}

template <typename elm, Link<elm> elm::* offset>
inline bool List<elm, offset>::
addAll(List* list)
{
    if (!list->head)
    {
        return false;
    }

    (this->tail->*offset).next = list->head;
    (list->head->*offset).prev = this->tail;
    this->tail = list->tail;
    list->head = list->tail = 0;
    return true;
}

template <typename elm, Link<elm> elm::* offset>
inline void List<elm, offset>::
addFirst(elm* item)
{
    elm* next = this->head;
    if (!next)
    {
        this->tail = item;
    }
    else
    {
        (next->*offset).prev = item;
    }
    (item->*offset).next = next;
    (item->*offset).prev = 0;
    this->head = item;
}

template <typename elm, Link<elm> elm::* offset>
inline void List<elm, offset>::
addLast(elm* item)
{
    elm* prev = this->tail;
    if (!prev)
    {
        this->head = item;
    }
    else
    {
        (prev->*offset).next = item;
    }
    (item->*offset).prev = prev;
    (item->*offset).next = 0;
    this->tail = item;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::
remove(elm* item)
{
    elm* next = (item->*offset).next;
    elm* prev = (item->*offset).prev;
    if (!next)
    {
        this->tail = prev;
    }
    else
    {
        (next->*offset).prev = prev;
    }
    if (!prev)
    {
        this->head = next;
    }
    else
    {
        (prev->*offset).next = next;
    }
    return item;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::
removeFirst()
{
    elm* item = this->head;
    if (!item)
    {
        return 0;
    }
    elm* next = (item->*offset).next;
    if (!next)
    {
        this->tail = 0;
    }
    else
    {
        (next->*offset).prev = 0;
    }
    this->head = next;
    return item;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::
removeLast()
{
    elm* item = this->tail;
    if (!item)
    {
        return 0;
    }
    elm* prev = (item->*offset).prev;
    if (!prev)
    {
        this->head = 0;
    }
    else
    {
        (prev->*offset).next = 0;
    }
    this->tail = prev;
    return item;
}

template <typename elm, Link<elm> elm::* offset>
inline typename List<elm, offset>::Iterator List<elm, offset>::
begin()
{
    return Iterator(*this, 0, this->head);
}

template <typename elm, Link<elm> elm::* offset>
inline typename List<elm, offset>::Iterator List<elm, offset>::
end()
{
    return Iterator(*this, this->tail, 0);
}

template <typename elm, Link<elm> elm::* offset>
inline bool List<elm, offset>::
contains(elm* item)
{
    elm* e;

    Iterator iter(begin());
    while ((e = iter.next()))
    {
        if (e == item)
        {
            return true;
        }
    }
    return false;
}

template <typename elm, Link<elm> elm::* offset>
inline typename List<elm, offset>::Iterator List<elm, offset>::
list(elm* item)
{
    return Iterator(*this, item);
}

template <typename elm, Link<elm> elm::* offset>
inline List<elm, offset>::Iterator::
Iterator(List<elm, offset>& list, elm* pre, elm* suc) throw() :
    list(list), pre(pre), suc(suc), cur(0)
{
}

template <typename elm, Link<elm> elm::* offset>
inline List<elm, offset>::Iterator::
Iterator(List<elm, offset>& list, elm* cur) throw() :
    list(list), pre(cur), suc((cur->*offset).next), cur(cur)
{
}

template <typename elm, Link<elm> elm::* offset>
inline bool List<elm, offset>::Iterator::
hasNext() const
{
    return suc ? true : false;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::Iterator::
next()
{
    cur = suc;
    if (suc)
    {
        pre = suc;
        suc = (suc->*offset).next;
    }
    return cur;
}

template <typename elm, Link<elm> elm::* offset>
inline bool List<elm, offset>::Iterator::
hasPrevious() const
{
    return pre ? true : false;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::Iterator::
previous()
{
    cur = pre;
    if (pre)
    {
        suc = pre;
        pre = (pre->*offset).prev;
    }
    return cur;
}

template <typename elm, Link<elm> elm::* offset>
inline elm* List<elm, offset>::Iterator::
remove()
{
    elm* item = cur;
    if (cur)
    {
        if (cur == suc)
        {
            suc = (suc->*offset).next;
        }
        else if (cur == pre)
        {
            pre = (pre->*offset).prev;
        }
        list.remove(cur);
        cur = 0;
    }
    return item;
}

template <typename elm, Link<elm> elm::* offset>
inline void List<elm, offset>::Iterator::
add(elm* item)
{
    if (pre)
    {
        list.addAfter(pre, item);
    }
    else if (suc)
    {
        list.addBefore(item, suc);
    }
    else
    {
        list.addFirst(item);
    }
    pre = item;
}

#endif // NINTENDO_ES_LIST_H_INCLUDED
