#ifndef LinkedList_H
#define LinkedList_H

template <typename T> struct LinkedListNode
{
public:
	LinkedListNode(T value)
	{
		Value = value;
		Next = Previous = 0;
	}
	T Value;
	LinkedListNode<T> *Next;
	LinkedListNode<T> *Previous;
};

template <typename T> class LinkedList
{
private:
	unsigned int count;
public:
	LinkedListNode<T> *First;
	LinkedListNode<T> *Last;

	LinkedList()
	{
		First = Last = new LinkedListNode<T>(0);
		count = 0;
	}
	LinkedList(T firstItem)
	{
		First = Last = new LinkedListNode<T>(firstItem);

		if(firstItem)
			count = 1;
		else
			count = 0;
	}
	~LinkedList()
	{
		LinkedListNode<T> *nd = First;

		while(nd != 0)
		{
			LinkedListNode<T> *next = nd->Next;

			delete nd;
			nd = next;
		}
	}
	T GetItem(unsigned int index)
	{
		unsigned int searchIndex = 0;

		for(LinkedListNode<T> *nd = this->First; nd != 0; nd = nd->Next, searchIndex++)
			if(searchIndex == index)
				return nd->Value;
		return 0;
	}
	unsigned int GetCount()
	{
		return count;
	}
	unsigned int Find(T item)
	{
		unsigned int searchIndex = 0;

		for(LinkedListNode<T> *nd = this->First; nd != 0; nd = nd->Next, searchIndex++)
			if(nd->Value == item)
				return searchIndex;
		return 0;
	}
	unsigned int Add(T item)
	{
		LinkedListNode<T> *nd = new LinkedListNode<T>(item);

		if(count == 0)
		{
			nd->Previous = 0;
			this->First = this->Last = nd;
		}
		else
		{
			if(this->Last != 0 && this->Last->Value != 0)
			{
				LinkedListNode<T> *oldLast = this->Last;

				oldLast->Next = nd;
				nd->Previous = oldLast;
			}
			this->Last = nd;
		}
		return ++count;
	}
	void Remove(unsigned int index)
	{
		unsigned int removalIndex = 0;

		if(index == 0 && this->First)
		{
			this->First = this->First->Next;
			count--;
			return;
		}
		if(index == count && this->Last)
		{
			this->Last = this->Last->Previous;
			count--;
			return;
		}
		for(LinkedListNode<T> *nd = this->First; nd != 0 && nd->Value != 0; nd = nd->Next, removalIndex++)
		{
			if(removalIndex == index)
			{
				nd->Previous->Next = nd->Next;
				nd->Next->Previous = nd->Previous;
				return;
			}
		}
	}
	void Remove(T item)
	{
		Remove(Find(item));
	}
	void Clear()
	{
		LinkedListNode<T> *oldFirst = this->First;

		this->First = this->Last = 0;
		for(LinkedListNode<T> *nd = oldFirst; nd != 0 && nd->Value != 0; nd = nd->Next)
		{
			if(nd->Previous)
				nd->Previous->Next = 0;
			nd->Previous = 0;
		}
	}
};
#endif
