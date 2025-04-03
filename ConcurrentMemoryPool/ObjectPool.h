#pragma once
#include<iostream>
#include<vector>
#include<time.h>
using std::cout;
using std::endl;
// �����ڴ��
//�̶���С���ڴ������ͷ������ص㣺
//1.���ܴﵽ����
//2.�������ڴ���Ƭ������


//����һ��ģ���࣬�ɹ����������� T �Ķ����
template <class T>
class ObjectPool
{
//���нӿ�
public:
	T* New()
	{
		T* obj = nullptr; //����ָ�� obj�����ڴ洢���صĶ���ָ��

		//����󻻻������ڴ��洢����freeList�У������ⲿ���ڴ滹����������
		//��˿������Ȱѻ��������ڴ������ٴ��ظ�����
		if (_freeList)
		{
			void* next = *((void**)_freeList);
			obj = (T*)_freeList;
			_freeList = next;
		}
		//������Ҫ�Ӵ���ڴ����г��¶���
		else
		{
			// ��ʣ���ڴ治��һ�������Сʱ�������¿��ٴ��ռ�
			if (_remainBytes < sizeof(T))
			{
				//���� 128KB ���ڴ���Ϊ���ڴ�أ����ں����з�
				_remainBytes = 128 * 1024;
				_memory = (char*)malloc(_remainBytes); //ת��Ϊ char* ��Ϊ��֧��ָ��ƫ��
				if (_memory == nullptr)
				{
					throw std::bad_alloc(); //����ʧ��ʱ�׳��쳣��ʹ�ñ�׼�� C++ �쳣�� std::bad_alloc
				}
			}

			//����ǰ _memory ָ����Ϊ������û��Ķ����ַ
			//���磺sizeof(TreeNode) С�� 8 �ֽ�ʱ��������Ȼ���� 8 �ֽڣ����� freeList ����
			obj = (T*)_memory;
			//�����Ż���Ϊ��֤ obj ����Ϊ����ڵ㣬���������ܴ���һ��ָ���С
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//���� _memory �� _remainBytes��Ϊ��һ�η�������׼��
			_memory += objSize;
			_remainBytes -= objSize;
			
		}
		//��λnew����ʾ����T�Ĺ��캯����ʼ��,��ָ���ڴ��ַ���ù��캯����ʼ������
		new (obj)T;

		return obj;
	}

	//�û��ͷŶ���ʱ���ô˺������ֶ����仹�س���
	void Delete(T* obj)
	{
		//��ʾ������������,ע����ֻ�ǵ��������������ͷ��ڴ�
		obj->~T();

		//ͷ��
		//*(int*)obj = nullpte; //32λ����ϵͳ��û���⣬��64λ�»ᱨ��
		*(void**)obj = nullptr; //����д�����ᱨ��
		_freeList = obj;
	}


private:
	char* _memory = nullptr; //ָ�򿪱ٵĴ���ڴ�ָ��
	size_t _remainBytes = 0; //����ڴ����зֹ�����ʣ���ֽ���
	void* _freeList = nullptr; //�����������е���������������ָ��

};


//�򵥶������ڵ�ṹ������ֵ����������
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
	//�����ͷŵ��ִ�
	const size_t Rounds = 3;

	//ÿ�������ͷŶ��ٴ�
	const size_t N = 10000;


	//ԭ����ʽ����
	
	size_t begin1 = clock(); //clock() ��ʼ��ʱ
	std::vector<TreeNode*> v1;
	v1.reserve(N);
	//ÿ�ִ��� N ���ڵ㣬��ϵͳ�ѷ��䲢����
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

	size_t end1 = clock();// ��¼����ʱ��

	ObjectPool<TreeNode> TNPool; //�����ڴ��
	size_t begin2 = clock(); //��ʼ��ʱ
	std::vector<TreeNode*> v2;
	v2.reserve(N);

	//ͬ���� N ���������ͷţ�����ȫ�����ڴ��
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

	//�ڶ��β��Խ���ʱ��
	size_t end2 = clock();

	cout << "new cost time" << end1 - begin1 << endl;
	cout << "object pool cost time" << end2 - begin2 << endl;
}


