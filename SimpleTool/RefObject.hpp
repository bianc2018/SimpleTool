/************************************************************************
     共享指针
************************************************************************/
#ifndef _REFCOUNTED_INCLUDED_
#define _REFCOUNTED_INCLUDED_
#include <cassert>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
	#include <Windows.h>
	typedef LONG RefCountType;
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif  
	#include <pthread.h>
	typedef int RefCountType;
#else
	#error "不支持的平台"
#endif

#include <new>

namespace sim
{
	//引用计数对象
	class RefCountable 
	{
	public:
		RefCountType add_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedIncrement(&ref_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = ++ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType dec_ref(void) 
		{
#ifdef OS_WINDOWS
			return ::InterlockedDecrement(&ref_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = --ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
		}

		RefCountType get_ref_count(void) const
		{ 
			return ref_count_;
		}

		RefCountType add_weak_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedIncrement(&ref_weak_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = ++ref_weak_count_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType dec_weak_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedDecrement(&ref_weak_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = --ref_weak_count_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
		}

		RefCountType get_ref_weak_count(void) const
		{
			return ref_weak_count_;
		}

		RefCountable(RefCountType count=1) : ref_count_(count), ref_weak_count_(0)
		{
#ifdef OS_LINUX
			pthread_mutex_init(&lock_, NULL);
#endif
		}
		virtual ~RefCountable(void) { /*assert(0 == refCount_);*/ }
	private:
		RefCountable(const RefCountable&);
		RefCountable& operator = (const RefCountable&) { return *this; };

	private:
		RefCountType volatile ref_count_;
		//弱引用
		RefCountType volatile ref_weak_count_;
#ifdef OS_LINUX
		//linux上面用锁实现
		pthread_mutex_t lock_;
#endif
	};

	//引用归零的时候使用的删除器
	typedef void(*RefObjectDelete)(void* ptr, void*pdata);

	template <typename T>
	class RefWeakObject;

	template <typename T>
	class RefObject
	{
		friend class RefWeakObject<T>;
	public:
		RefObject(T* p=NULL, RefObjectDelete deleter=NULL, void*pdata=NULL)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(new RefCountable(1)), pdata_(pdata)
		{
			//新增引用
			//add_ref();
		}
		RefObject(RefCountable *ref_count_ptr,T* p, RefObjectDelete deleter, void* pdata)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(ref_count_ptr), pdata_(pdata)
		{
			//新增引用
			//add_ref();
		}
		virtual ~RefObject()
		{
			//释放
			release();
		}
		//// 浅拷贝
		RefObject(const RefObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			ref_count_ptr_->add_ref();
		}
		//// 浅拷贝
		virtual RefObject<T>& operator=(const RefObject<T>& rhs)
		{
			//自己赋值给自己的情况不算
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();

				//释放
				release();

				//赋值
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}
		
		virtual RefCountType getcount()
		{
			return ref_count_ptr_->get_ref_count();
		}

		virtual void reset(T* p = NULL)
		{
			release();
			ptr_ = p;
			ref_count_ptr_ = new RefCountable(1);
		}

		virtual T* operator->()
		{
			return ptr_;
		}
		virtual T& operator*()
		{
			return *ptr_;
		}
		//获取指针
		virtual T* get()
		{
			return ptr_;
		}
		virtual operator bool()
		{
			return NULL != ptr_;
		}
		/*operator nullptr()
		{
			return ptr_;
		}*/
	protected:
		void release()
		{
			//释放 强引用和弱引用 都没有之后
			if (ref_count_ptr_->dec_ref() <= 0&& ref_count_ptr_->get_ref_weak_count()<=0)
			{
				if (ptr_)
				{
					if (deleter_)
						deleter_(ptr_, pdata_);
					else
						delete ptr_;
				}
				ptr_ = NULL;

				//回收计数指针
				delete ref_count_ptr_;
				ref_count_ptr_ = NULL;
			}
		}
	protected:
		//指针
		T* ptr_;
		RefObjectDelete deleter_;
		void*pdata_;
		//引用计数指针
		RefCountable*ref_count_ptr_;
	};

	template <typename T>
	class RefWeakObject
	{
	public:
		//// 浅拷贝
		RefWeakObject(const RefObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			ref_count_ptr_->add_weak_ref();
		}
		//// 浅拷贝
		RefWeakObject(const RefWeakObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			ref_count_ptr_->add_weak_ref();
		}
		virtual ~RefWeakObject()
		{
			//释放
			release();
		}
		
		//// 浅拷贝
		virtual RefWeakObject<T>& operator=(const RefObject<T>& rhs)
		{
			{
				rhs.ref_count_ptr_->add_weak_ref();

				//释放
				release();

				//赋值
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}

		virtual RefWeakObject<T>& operator=(const RefWeakObject<T>& rhs)
		{
			//自己赋值给自己的情况不算
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_weak_ref();

				//释放
				release();

				//赋值
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}

		virtual RefObject<T> ref_object()
		{
			if (ref_count_ptr_->get_ref_count()<=0)
			{
				//返回空引用
				return RefObject<T>();
			}
			ref_count_ptr_->add_ref();
			return RefObject<T>(ref_count_ptr_, ptr_, deleter_, pdata_);
		}

		virtual RefCountType getcount()
		{
			return ref_count_ptr_->get_ref_weak_count();
		}

		virtual void reset(T* p = NULL)
		{
			release();
			ptr_ = p;
			ref_count_ptr_ = new RefCountable(0);
			ref_count_ptr_->add_weak_ref();
		}
	protected:
		void release()
		{
			//释放 强引用和弱引用 都没有之后
			if (ref_count_ptr_->dec_weak_ref() <= 0 && ref_count_ptr_->get_ref_count() <= 0 )
			{
				if (ptr_)
				{
					if (deleter_)
						deleter_(ptr_, pdata_);
					else
						delete ptr_;
				}
				ptr_ = NULL;

				//回收计数指针
				delete ref_count_ptr_;
				ref_count_ptr_ = NULL;
			}
		}
	protected:
		//指针
		T* ptr_;
		RefObjectDelete deleter_;
		void* pdata_;
		//引用计数指针
		RefCountable* ref_count_ptr_;
	};

	//引用缓存
	class RefBuff :public RefObject<char>
	{
		//typedef void(*RefObjectDelete)(void* ptr, void*pdata);
		static void RefBuffDelete(void* ptr, void*pdata)
		{
			delete[]((char*)ptr);
		}
	public:
		RefBuff(unsigned int buff_size) 
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			
		};
		
		RefBuff(unsigned int buff_size,char _val)
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			set(_val);
		};
		
		RefBuff(const char*pdata, unsigned int buff_size) 
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			::memcpy(get(), pdata, buff_size);
		}
		
		RefBuff(const char*pdata)
			:RefObject<char>(new char[::strlen(pdata)], &RefBuff::RefBuffDelete), buff_size_(::strlen(pdata))
		{
			::memcpy(get(), pdata, buff_size_);
		}

		RefBuff() :RefObject<char>(), buff_size_(0)
		{
		}
		
		//// 浅拷贝
		RefBuff(const RefBuff& orig):RefObject<char>(orig), buff_size_(orig.buff_size_)
		{
		}
		
		//// 浅拷贝
		virtual RefBuff& operator=(const RefBuff& rhs)
		{
			//自己赋值给自己的情况不算
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();

				release();

				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
				buff_size_ = rhs.buff_size_;
			}
			return *this;
		}
		
		virtual RefBuff operator+(const RefBuff& rhs)
		{
			if (this->buff_size_ + rhs.buff_size_ <= 0)
				return RefBuff();

			RefBuff temp(this->buff_size_ + rhs.buff_size_);
			if(this->buff_size_>0)
				memcpy(temp.ptr_, this->ptr_, this->buff_size_);
			if (rhs.buff_size_ > 0)
				memcpy(temp.ptr_+ this->buff_size_, rhs.ptr_, rhs.buff_size_);
			return temp;
		}
		
		//不做检查
		virtual char& operator[](unsigned int index)
		{
			return ptr_[index];
		}

		virtual unsigned int size()
		{
			return buff_size_;
		}

		//设置值
		virtual void set(int _val)
		{
			memset(ptr_, _val, buff_size_);
		}
	private:
		unsigned int buff_size_;
	};
}
 #endif // ifndef _REFCOUNTED_INCLUDED
