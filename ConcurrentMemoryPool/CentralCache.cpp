#include "CentralCache.h"

CentralCache CentralCache::_sInst;

// 获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	return nullptr;
}

// 从CentralCache获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	//加锁保护
	_spanLists[index]._mtx.lock();
	
	//获取一个有对象的 Span
	Span* span = GetOneSpan(_spanLists[index], size);//从当前 size class 的 span 链表中找一个有可用对象的 Span
	assert(span);
	assert(span->_freeList);//这个 Span 中包含的空闲对象链表

	// 从span中获取batchNum个对象
	// 如果不够batchNum个，有多少拿多少
	start = span->_freeList;
	end = start;
	size_t i = 0;
	//实际拿到的数量
	size_t actualNum = 1;
	while (i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	// 解锁后返回你实际拿到的对象数量，供 ThreadCache 判断是否需要缓存其余部分
	_spanLists[index]._mtx.unlock();

	return actualNum;
}
