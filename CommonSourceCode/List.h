#ifndef List_H
#define List_H

#include <Memory.h>

template <typename T> class List
{
private:
	unsigned int count;
	T *array;
public:
	List()
	{
		count = 0;
		array = new T[1];
	}
	List(T firstItem)
	{
		count = 1;
		array = new T[1];
		array[0] = firstItem;
	}
	~List()
	{
		delete[] array;
	}

	T GetItem(unsigned int index)
	{
		if(index > count)
			return 0;
		return array[index];
	}

	unsigned int GetCount()
	{
		return count;
	}

	unsigned int Find(T item)
	{
		for(unsigned int i = 0; i < count; i++)
		{
			if(array[i] == item)
				return i;
		}
		return 0;
	}

	unsigned int Add(T item)
	{
		T *newArray = new T[count + 1];

		Memory::Copy(newArray, array, count * sizeof(T));
		newArray[count] = item;
		delete array;
		array = newArray;
		return ++count;
	}

	void Remove(unsigned int idx)
	{
		if(idx > count)
			return;
		array[idx] = 0;
		//If the array can be retracted, do so
		if((count >= 1) && (idx == count - 1))
		{
			T *newArray = new T[idx - 1];

			//Copy over every item except the last
			Memory::Copy(newArray, array, (idx - 1) * sizeof(T));
			delete array;
			array = newArray;
		}
		//Keep the count consistent with expectations
		count--;
	}

	void Remove(T item)
	{
		Remove(Find(item));
	}

	void Clear()
	{
		delete[] array;
		count = 0;
		array = new T[0];
	}
};

#endif
