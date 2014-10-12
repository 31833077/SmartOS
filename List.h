#ifndef _List_H_
#define _List_H_

#include <stddef.h>
#include "Sys.h"

// 数组长度
//#define ArrayLength(arr) sizeof(arr)/sizeof(arr[0])
// 从数组创建列表
#define MakeList(T, arr) List<T>(&arr[0], sizeof(arr)/sizeof(arr[0]))

// 迭代器
template<typename T>
class IEnumerator
{
public:
	virtual T& Current()	= 0;	// 获取集合中的当前元素
	virtual bool MoveNext()	= 0;	// 将枚举数推进到集合的下一个元素
	virtual void Reset()	= 0;	// 将枚举数设置为其初始位置，该位置位于集合中第一个元素之前

	virtual void Remove()	= 0;	// 从列表中移除当前节点
};

// 公开枚举数，该枚举数支持在指定类型的集合上进行简单迭代
template<typename T>
class IEnumerable
{
public:
	virtual IEnumerator<T>* GetEnumerator() = 0;	// 返回一个循环访问集合的枚举数
};

// 集合接口
template<typename T>
class ICollection
{
public:
	virtual int Count() const			= 0;	// 集合中包含的元素数
	virtual void Add(const T& item)		= 0;	// 将某项添加到集合
	virtual void Remove(const T& item)	= 0;	// 从集合中移除特定对象的第一个匹配项
	virtual bool Contains(const T& item)= 0;	// 确定集合是否包含特定值
	virtual void Clear()				= 0;	// 从集合中移除所有项
	virtual void CopyTo(T* arr)			= 0;	// 将集合元素复制到数组中
};

// foreach宏，用于遍历迭代器接口，必须和foreach_end成对出现
#define foreach(cls,cur,list)					\
	{												\
		IEnumerator<cls>* et = list.GetEnumerator();\
		while(et->MoveNext())						\
		{											\
			cls cur = et->Current();				\

#define foreach_end()	\
		}					\
		delete et;			\
	}

// 用于在迭代器中删除当前节点的宏
#define foreach_remove() et->Remove();

// 数组
class Array
{
private:
    void**	_Arr;
	int		_Count;
	int		_Capacity;

public:
	// 有效元素个数
    int Count() const { return _Count; }
	// 最大元素个数
    int Capacity() const { return _Capacity; }

	Array(int capacity = 0x10)
	{
		_Capacity = capacity;
		_Count = 0;

		_Arr = new void*[capacity];
		ArrayZero2(_Arr, capacity);
	}

	~Array()
	{
		if(_Arr) delete _Arr;
		_Arr = NULL;
	}

	// 压入一个元素
	int Push(void* item)
	{
		assert_param(_Count < _Capacity);

		// 找到空闲位放置
		int idx = _Count++;
		_Arr[idx] = item;

		return idx;
	}

	// 弹出一个元素
	const void* Pop()
	{
		assert_param(_Count > 0);

		return _Arr[--_Count];
	}

    // 重载索引运算符[]，让它可以像数组一样使用下标索引。
    void* operator[](int i)
	{
		assert_param(i >= 0 && i < _Count);
		return _Arr[i];
	}
	// 列表转为指针，注意安全
    //T* operator=(Array arr) { return arr.Arr; }
};

// 固定大小的指针数组。
/*
用于数据元素不多，有个上限的场合。主要为了方便遍历。
存储数据时，从头开始找到空位来存放，不用时清空相应空位。
所以需要注意的是，在数组里面存储的元素不一定连续。
*/
template<typename T, int ArraySize>
class FixedArray
{
private:
	uint	_Count;
	T*		Arr[ArraySize];

public:
	FixedArray()
	{
		_Count = 0;
		ArrayZero(Arr);
	}

	FixedArray(FixedArray& arr)
	{
		_Count = arr._Count;
		ArrayCopy(Arr, arr.Arr);
	}

    FixedArray& operator=(FixedArray& arr)
	{
		_Count = arr._Count;
		ArrayCopy(Arr, arr.Arr);

		return *this;
	}

	uint Count() const { return _Count; }

	// 压入一个元素。返回元素所存储的索引
	int Add(T* item)
	{
		assert_ptr(item);
		assert_param(_Count < ArraySize);

		// 找到空闲位放置
		int i = 0;
		for(; i < ArraySize && Arr[i]; i++);
		// 检查是否还有空位
		if(i >= ArraySize) return -1;

		Arr[i] = item;
		_Count++;

		return i;
	}

	// 弹出最后一个元素
	T* Pop()
	{
		if(_Count == 0) return NULL;

		// 找到最后一个元素
		int i = ArraySize - 1;
		for(; i >=0 && !Arr[i]; i--);

		T* item = Arr[i];
		_Count--;
		Arr[i] = NULL;

		return item;
	}

	// 删除元素
	bool Remove(T* item)
	{
		int idx = Find(item);
		if(idx < 0) return false;

		RemoveAt(idx);

		return true;
	}

	// 删除指定位置的元素
	void RemoveAt(int idx)
	{
		assert_param(idx >= 0 && idx < ArraySize);

		_Count--;
		Arr[idx] = NULL;
	}

	FixedArray& Clear()
	{
		_Count = 0;
		ArrayZero(Arr);

		return *this;
	}

	// 释放所有指针指向的内存
	FixedArray& DeleteAll()
	{
		for(int i=0; i < ArraySize; i++)
		{
			if(Arr[i]) delete Arr[i];
		}

		return *this;
	}

	// 查找元素
	int Find(T* item)
	{
		assert_ptr(item);

		int i = 0;
		for(; i < ArraySize && Arr[i] != item; i++);
		if(i >= ArraySize) return -1;

		return i;
	}

	// 移到下一个元素，累加参数，如果没有下一个元素则返回false。
	bool MoveNext(int& idx)
	{
		//assert_ptr(idx);

		// 从idx开始，找到第一个非空节点。注意，存储不一定连续
		int i = idx + 1;
		for(; i < ArraySize && !Arr[i]; i++);
		if(i >= ArraySize) return false;

		idx = i;

		return true;
	}

    // 重载索引运算符[]，让它可以像数组一样使用下标索引。
    /*T*& operator[](int i)
	{
		assert_param(i >= 0 && i < ArraySize);
		return Arr[i];
	}*/
	// 不能允许[]作为左值，否则外部直接设置数据以后，Count并没有改变
    T* operator[](int i)
	{
		assert_param(i >= 0 && i < ArraySize);
		return Arr[i];
	}
};

// 变长列表模版
template<typename T>
class List : public IEnumerable<T>, public ICollection<T>
{
private:
    T* _Arr;		// 存储数据的数组
    uint _Count;// 拥有实际元素个数
    uint _total;// 可容纳元素总数

    void ChangeSize(int newSize)
    {
        if(_total == newSize) return;

        T* arr2 = new T[newSize];
        if(_Arr)
        {
            // 如果新数组较小，则直接复制；如果新数组较大，则先复制，再清空余下部分
            if(newSize < _Count)
			{
				// 此时有数据丢失
                memcpy(arr2, _Arr, newSize * sizeof(T));
				_Count = newSize;
			}
            else
            {
                memcpy(arr2, _Arr, _Count * sizeof(T));
                memset(&arr2[_Count], 0, (newSize - _Count) * sizeof(T));
            }
            delete[] _Arr;
        }
		else
			ArrayZero2(arr2, newSize);
        _Arr = arr2;
		_total = newSize;
    }

    void CheckSize()
    {
        // 如果数组空间已用完，则两倍扩容
        if(_Count >= _total) ChangeSize(_Count > 0 ? _Count * 2 : 4);
    }

public:
    List(int size = 0)
    {
        _Count = 0;
        _total = size;
		_Arr = NULL;
		if(size)
		{
			_Arr = new T[size];
			ArrayZero2(_Arr, size);
		}
    }

    List(T* items, uint count)
    {
        _Arr = new T[count];

        _Count = 0;
        _total = count;
        for(int i=0; i<count; i++)
        {
            _Arr[_Count++] = *items++;
        }
    }

    ~List() { if(_Arr) delete[] _Arr; _Arr = NULL; }

	// 添加单个元素
    virtual void Add(const T& item)
    {
        // 检查大小
        CheckSize();

        _Arr[_Count++] = item;
    }

	// 添加多个元素
    void Add(T* items, int count)
    {
        int size = _Count + count;
        if(size >= _total) ChangeSize(_Count > 0 ? _Count * 2 : 4);

        for(int i=0; i<count; i++)
        {
            _Arr[_Count++] = *items++;
        }
    }

	// 查找元素
	int Find(const T& item)
	{
        for(int i=0; i<_Count; i++)
        {
            if(_Arr[i] == item) return i;
        }

		return -1;
	}

	virtual bool Contains(const T& item)
	{
		return Find(item) > -1;
	}

	// 删除指定位置元素
	void RemoveAt(int index)
	{
		if(_Count <= 0 || index >= _Count) return;

		// 复制元素
		if(index < _Count - 1) memcpy(&_Arr[index], &_Arr[index + 1], (_Count - index - 1) * sizeof(T));
		_Count--;
	}

	// 删除指定元素
	virtual void Remove(const T& item)
	{
		int index = Find(item);
		if(index >= 0) RemoveAt(index);
	}

	virtual void Clear()
	{
		_Count = 0;
	}

	virtual void CopyTo(T* arr)
	{
		assert_ptr(arr);
		if(!_Count) return;

		memcpy(arr, _Arr, _Count * sizeof(T));
	}

	// 返回内部指针
    const T* ToArray() { return _Arr; }

	// 有效元素个数
    virtual int Count() const { return _Count; }

	// 设置新的容量，如果容量比元素个数小，则会丢失数据
	void SetCapacity(int capacity) { ChangeSize(capacity); }

    // 重载索引运算符[]，让它可以像数组一样使用下标索引。
    T operator[](int i)
	{
		assert_param(i >= 0 && i < _Count);
		return _Arr[i];
	}

	// 列表转为指针，注意安全
    T* operator=(List& list) { return list.ToArray(); }

private:
	class ListEnumerator : public IEnumerator<T>
	{
	private:
		List*	_List;
		int		_Index;

	public:
		ListEnumerator(List* list)
		{
			_List = list;
			_Index = -1;
		}

		// 获取集合中的当前元素
		virtual T& Current() { return _List->_Arr[_Index]; }
		// 将枚举数推进到集合的下一个元素
		virtual bool MoveNext()
		{
			_Index++;
			if(_Index >= _List->_Count) _Index = -1;

			return _Index >= 0;
		}
		// 将枚举数设置为其初始位置，该位置位于集合中第一个元素之前
		virtual void Reset() { _Index = -1; }
		// 从列表中移除当前节点
		virtual void Remove()
		{
			// 删除当前节点后，索引后移一位，因为后面的数据前移了一位
			_List->RemoveAt(_Index);
			_Index--;
		}
	};

public:
	// 返回一个循环访问集合的枚举数。外部释放
	virtual IEnumerator<T>* GetEnumerator()
	{
		return new ListEnumerator(this);
	}
};

// 双向链表
template <class T> class LinkedList;

// 双向链表节点。实体类继承该类
template <class T> class LinkedNode
{
	// 友元类。允许链表类控制本类私有成员
    friend class LinkedList<T>;

public:
    T* Prev;	// 上一个节点
    T* Next;	// 下一个节点

    void Initialize()
    {
        Next = NULL;
        Prev = NULL;
    }

    // 从链表中删除。需要修改前后节点的指针指向，但当前节点仍然指向之前的前后节点
    void RemoveFromList()
    {
        if(Prev) Prev->Next = Next;
        if(Next) Next->Prev = Prev;
    }

	// 完全脱离链表。不再指向其它节点
    void Unlink()
    {
        if(Prev) Prev->Next = Next;
        if(Next) Next->Prev = Prev;

        Next = NULL;
        Prev = NULL;
    }

	// 把当前节点附加到另一个节点之后
	void LinkAfter(T* node)
	{
		node->Next = (T*)this;
		Prev = node;
		// 不能清空Next，因为可能是两个链表的合并
		//Next = NULL;
	}

	// 最后一个节点
	T* Last()
	{
		T* node = (T*)this;
		while(node->Next) node = node->Next;

		return node;
	}

	// 附加到当前节点后面
	void Append(T* node)
	{
		Next = node;
		node->Prev = (T*)this;
		//node->Next = NULL;
	}
};

// 双向链表
template <class T>
class LinkedList : public IEnumerable<T>, public ICollection<T>
{
public:
    // 链表节点。实体类不需要继承该内部类
	class Node
	{
	public:
		T		Item;	// 元素
		Node*	Prev;	// 前一节点
		Node*	Next;	// 下一节点

		Node()
		{
			Prev = NULL;
			Next = NULL;
		}

		// 从队列中脱离
		void RemoveFromList()
		{
			// 双保险，只有在前后节点指向当前节点时，才修改它们的值
			if(Prev && Prev->Next == this) Prev->Next = Next;
			if(Next && Next->Prev == this) Next->Prev = Prev;
		}

		// 附加到当前节点后面
		void Append(Node* node)
		{
			Next = node;
			node->Prev = this;
		}
	};

private:
	Node*	_Head;		// 链表头部
    Node*	_Tail;		// 链表尾部
	int 	_Count;		// 元素个数

	void Init()
	{
        _Head = NULL;
		_Tail = NULL;
		_Count = 0;
	}

	void Remove(Node* node)
	{
		// 脱离队列
		node->RemoveFromList();
		// 特殊处理头尾
		if(node == _Head) _Head = node->Next;
		if(node == _Tail) _Tail = node->Prev;

		delete node;
		_Count--;
	}

public:
    LinkedList()
    {
		Init();
    }

	// 计算链表节点数
    virtual int Count() const { return _Count; }

	// 将某项添加到集合
	virtual void Add(const T& item)
	{
		Node* node = new Node();
		node->Item = item;

		if(_Tail)
			_Tail->Append(node);
		else
			_Head = _Tail = node;

		_Count++;
	}

	// 从集合中移除特定对象的第一个匹配项
	virtual void Remove(const T& item)
	{
		if(!_Count) return;

		Node* node;
		for(node = _Head; node; node = node->Next)
		{
			if(node->Item == item) break;
		}

		if(node) Remove(node);
	}

	// 确定集合是否包含特定值
	virtual bool Contains(const T& item)
	{
		if(!_Count) return false;

		Node* node;
		for(node = _Head; node; node = node->Next)
		{
			if(node->Item == item) return true;
		}

		return false;
	}

	// 从集合中移除所有项。注意，该方法不会释放元素指针
	virtual void Clear()
	{
		if(!_Count) return;

		Node* node;
		for(node = _Head; node;)
		{
			// 先备份，待会删除可能影响指针
			Node* next = node->Next;
			delete node;
			node = next;
		}

		Init();
	}

	// 将集合元素复制到数组中
	virtual void CopyTo(T* arr)
	{
		assert_ptr(arr);

		if(!_Count) return;

		Node* node;
		for(node = _Head; node; node = node->Next)
		{
			*arr++ = node->Item;
		}
	}

private:
	// 链表迭代器
	class LinkedListEnumerator : public IEnumerator<T>
	{
	private:
		LinkedList* _List;
		Node*		_Current;	// 当前节点
		Node*		_Next;		// 下一个节点。考虑到中途可能删节点，必须提前备好下一个

	public:
		LinkedListEnumerator(LinkedList* list)
		{
			_List		= list;
			_Current	= NULL;
			_Next		= _List->_Head;
		}

		// 获取集合中的当前元素
		virtual T& Current() { return _Current->Item; }
		// 将枚举数推进到集合的下一个元素
		virtual bool MoveNext()
		{
			// 总是提前计算下一个节点，以免中途当前节点被删除
			_Current = _Next;
			if(_Current) _Next = _Current->Next;

			return _Current != NULL;
		}
		// 将枚举数设置为其初始位置，该位置位于集合中第一个元素之前
		virtual void Reset() { _Current = NULL; _Next = _List->_Head; }
		// 从列表中移除当前节点
		virtual void Remove()
		{
			_List->Remove(_Current);
		}
	};

public:
	// 返回一个循环访问集合的枚举数。外部释放
	virtual IEnumerator<T>* GetEnumerator()
	{
		return new LinkedListEnumerator(this);
	}

	T& First() { return _Head->Item; }
	T& Last() { return _Tail->Item; }

	// 释放第一个有效节点
    T& ExtractFirst()
    {
		if(!_Count) return NULL;

        Node* node = _Head;
        _Head = _Head->Next;
		// 可能只有一个节点
		if(!_Head)
			_Tail = NULL;
		else
			_Head->Prev = NULL;

		T& item = node->Item;
		delete node;
		_Count--;

        return item;
    }

	// 释放最后一个有效节点
    T& ExtractLast()
    {
		if(!_Count) return NULL;

        Node* node = _Tail;
        _Tail = _Tail->Prev;
		// 可能只有一个节点
		if(!_Tail)
			_Head = NULL;
		else
			_Tail->Next = NULL;

		T& item = node->Item;
		delete node;
		_Count--;

        return item;
    }
};

#endif //_List_H_
