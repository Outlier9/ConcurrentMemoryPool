#pragma once
#include<iostream>
#include<vector>
#include<time.h>
#include<thread>
using std::cout;
using std::endl;
#include<assert.h>
#include <thread>
#include <mutex>

//小于等于MAX_BYTES，就找ThreadCache申请
//大于MAX_BYTES，就直接找PageCache或者系统堆申请
static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;
static const size_t NPAGES = 128;

#ifdef   _WIN64_WIN32
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#endif //  _WIN32


static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

//管理切分好的小对象的自由链表
class FreeList
{
public:
	void Push(void* obj)
	{
		//头插
		//*(void**)obj = _freeList;
		NextObj(obj) = _freeList;
		_freeList = obj;
	}

	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}

	void* Pop()
	{
		assert(_freeList);
		//头删
		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxSize;
	}

private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
};

//管理对齐和映射等关系
class SizeClass
{
public:
	//整体控制在最多10%左右的内碎片浪费
	//[1,128]				8byte对齐			freelist[0, 16)
	//[128 + 1, 1024]		16byte对齐			freelist[16, 72)
	//[1024+1,8*1024]		128byte对齐			freelist[72, 128)
	//[8*1024+1,64*1024]	1024byte对齐		freelist[128, 184)
	//[64*1024+1,256*1024]	8*1024byte对齐		freelist[184, 208)

	//向上对齐数
	//将 size 向上对齐为 alignNum 的整数倍（如：37 → 40）
	static inline size_t _RoundUp(size_t size, size_t alignNum)
	{
		size_t alignSize;
		if (size % 8 != 0)
		{
			alignSize = (size / alignNum + 1) * alignNum;

		}
		else
		{
			alignSize = size;
		}
		return alignSize;
	}
	//区间对齐入口函数
	//根据 size 所处的区间，调用 _RoundUp(size, 对应对齐粒度)
	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8*1024);
		}
		else
		{
			assert(false);
			return -1;
		}
	}
	//返回当前分组的下标偏移量
	//将 bytes 映射为以 alignNum 为粒度的第几个桶
	static inline size_t _Index(size_t bytes, size_t alignNum)
	{
		if (bytes % alignNum == 0)
		{
			return bytes / alignNum - 1;
		}
		else
		{
			return bytes / alignNum;
		}
	}

	//映射为 freelist 的下标
	//将 bytes 对应到 freelist 的下标（共 208 个）
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		// 每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}

		return -1;
	}

	// 一次thread cache从中心缓存获取多少个
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);

		// [2, 512]，一次批量移动多少个对象的(慢启动)上限值
		// 小对象一次批量上限高
		// 小对象一次批量上限低
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}
};


//管理多个连续页
struct Span
{
	PAGE_ID _pageId = 0; // 大块内存起始页的页号
	size_t  _n = 0;      // 页的数量

	Span* _next = nullptr;	// 双向链表的结构
	Span* _prev = nullptr;

	size_t _useCount = 0; // 切好小块内存，被分配给thread cache的计数
	void* _freeList = nullptr;  // 切好的小块内存的自由链表
};


// 带头双向循环链表 
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

private:
	Span* _head;
public:
	std::mutex _mtx; // 桶锁

};