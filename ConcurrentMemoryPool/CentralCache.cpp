#include "CentralCache.h"

CentralCache CentralCache::_sInst;

// ��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	return nullptr;
}

// ��CentralCache��ȡһ�������Ķ����thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	//��������
	_spanLists[index]._mtx.lock();
	
	//��ȡһ���ж���� Span
	Span* span = GetOneSpan(_spanLists[index], size);//�ӵ�ǰ size class �� span ��������һ���п��ö���� Span
	assert(span);
	assert(span->_freeList);//��� Span �а����Ŀ��ж�������

	// ��span�л�ȡbatchNum������
	// �������batchNum�����ж����ö���
	start = span->_freeList;
	end = start;
	size_t i = 0;
	//ʵ���õ�������
	size_t actualNum = 1;
	while (i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	// �����󷵻���ʵ���õ��Ķ����������� ThreadCache �ж��Ƿ���Ҫ�������ಿ��
	_spanLists[index]._mtx.unlock();

	return actualNum;
}
