#pragma once
#include<iostream>
#include<vector>
#include<time.h>
using std::cout;
using std::endl;
// 定长内存池
//固定大小的内存申请释放需求特点：
//1.性能达到极致
//2.不考虑内存碎片等问题


//定义一个模板类，可管理任意类型 T 的对象池
template <class T>
class ObjectPool
{
//公有接口
public:
	T* New()
	{
		T* obj = nullptr; //声明指针 obj，用于存储返回的对象指针

		//用完后换回来的内存块存储到了freeList中，但是这部分内存还可以再利用
		//因此可以优先把还回来的内存块对象，再次重复利用
		if (_freeList)
		{
			void* next = *((void**)_freeList);
			obj = (T*)_freeList;
			_freeList = next;
		}
		//否则，需要从大块内存中切出新对象
		else
		{
			// 当剩余内存不够一个对象大小时，则重新开辟大块空间
			if (_remainBytes < sizeof(T))
			{
				//申请 128KB 的内存作为新内存池，用于后续切分
				_remainBytes = 128 * 1024;
				_memory = (char*)malloc(_remainBytes); //转换为 char* 是为了支持指针偏移
				if (_memory == nullptr)
				{
					throw std::bad_alloc(); //分配失败时抛出异常，使用标准的 C++ 异常类 std::bad_alloc
				}
			}

			//将当前 _memory 指针作为分配给用户的对象地址
			//比如：sizeof(TreeNode) 小于 8 字节时，我们仍然分配 8 字节，避免 freeList 崩溃
			obj = (T*)_memory;
			//对齐优化：为保证 obj 可作为链表节点，必须至少能存下一个指针大小
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//更新 _memory 和 _remainBytes，为下一次分配做好准备
			_memory += objSize;
			_remainBytes -= objSize;
			
		}
		//定位new，显示调用T的构造函数初始化,在指定内存地址调用构造函数初始化对象
		new (obj)T;

		return obj;
	}

	//用户释放对象时调用此函数，手动将其还回池中
	void Delete(T* obj)
	{
		//显示调用析构函数,注意这只是调用析构，不会释放内存
		obj->~T();

		//头插
		//*(int*)obj = nullpte; //32位操作系统下没问题，但64位下会报错
		*(void**)obj = nullptr; //这种写法不会报错
		_freeList = obj;
	}


private:
	char* _memory = nullptr; //指向开辟的大块内存指针
	size_t _remainBytes = 0; //大块内存在切分过程中剩余字节数
	void* _freeList = nullptr; //还回来过程中的链表的自由链表的指针

};


//简单二叉树节点结构，包含值与左右子树
struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		,_left(nullptr)
		,_right(nullptr)
	{}
};


void TestObjectPool()
{
	//申请释放的轮次
	const size_t Rounds = 3;

	//每轮申请释放多少次
	const size_t N = 10000;


	//原生方式测试
	
	size_t begin1 = clock(); //clock() 开始计时
	std::vector<TreeNode*> v1;
	v1.reserve(N);
	//每轮创建 N 个节点，用系统堆分配并销毁
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}

	size_t end1 = clock();// 记录结束时间

	ObjectPool<TreeNode> TNPool; //创建内存池
	size_t begin2 = clock(); //开始计时
	std::vector<TreeNode*> v2;
	v2.reserve(N);

	//同样是 N 次申请与释放，不过全部走内存池
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i <N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}

	//第二段测试结束时间
	size_t end2 = clock();

	cout << "new cost time" << end1 - begin1 << endl;
	cout << "object pool cost time" << end2 - begin2 << endl;
}


