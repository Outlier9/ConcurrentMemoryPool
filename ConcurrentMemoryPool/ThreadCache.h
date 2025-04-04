#pragma once

#include "Common.h"

class ThreadCache
{
public:
	//申请和释放对象接口
	void* Allocate(size_t size);

	void Deallocate(void* ptr, size_t size);

	// 从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);

private:
	FreeList _freeLists[NFREELIST];
	
};

//TLS thread local stroage
//定义了一个线程私有的全局变量 pTLSThreadCache，
//每个线程都会有自己独立的一份副本，初始值为 nullptr
//_declspec(thread)		Windows专用TLS修饰符		Windows		✅ 推荐用于 Win32 / VC++
//thread_local			C++11 标准					跨平台		✅ 推荐现代 C++ 使用
//_thread				GCC 扩展					Linux		✅ Linux 下轻量 TLS

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;

