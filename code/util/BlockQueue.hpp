#pragma once
#include<queue>
#include<mutex>
#include<thread>
#include<condition_variable>
#include<vector>
#include<atomic>
#include<functional>
namespace MindbniM
{
    using task=std::function<void()>;
    int MBLOCKQUEUEMAXNUM=10;
    int MBLOCKQUEUETHREADNUM=3;
#define SET_ASYN_BLOCKQUEUE_THREAD_NUM(num) do{MBLOCKQUEUETHREADNUM=num;}while(0)
#define SET_ASYN_BLOCKQUEUE_MAXDATA_NUM(num) do{MBLOCKQUEUEMAXNUM=num;}while(0)
    template<class T>
    class blockqueue
    {
    public:
        static blockqueue<T>* GetInstance() 
        {
            static blockqueue<T> bq;
            return &bq;
        }
        void push(const T& data)
        {
            if(m_flag==false) return;
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock,[=]{return m_num<m_max;});
            m_q.push(data);
            ++m_num;
            //std::cout<<"push"<<std::endl;
            if(waitnum>0) m_cv.notify_all();
        }
        T pop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            ++waitnum;
            m_cv.wait(lock,[=]{return m_num>0||m_flag==false;});
            --waitnum;
            if(m_flag==false&&m_num==0) return T();
            T temp=m_q.front();
            m_q.pop();
            --m_num;
            //std::cout<<"pop"<<std::endl;
            return temp;
        }
        int size(){return m_q.size();}
        void run()
        {
            while(1)
            {
                if(m_flag==false&&m_num==0)
                {
                    //std::cout<<"thread quit"<<std::endl;
                    return;
                }
                T temp=pop();
                if(temp) temp();
            }
        }
        ~blockqueue()
        {
            m_flag=false;
            m_cv.notify_all();
            for(auto& t:m_pool)
            {
                t.join();
            }
        }
    private:
        blockqueue(int max_task_num=MBLOCKQUEUEMAXNUM,int thread_num=MBLOCKQUEUETHREADNUM):m_max(max_task_num),m_thread_num(thread_num)
        {
            m_pool.resize(thread_num);
            m_flag=true;
            for(int i=0;i<thread_num;i++)
            {
                m_pool[i]=std::thread(&blockqueue::run,this);
                //std::cout<<"thread create"<<std::endl;
            }
            //std::cout<<"blockqueue start"<<std::endl;
        }
        blockqueue(const blockqueue<T>& bq)=delete;
        void operator=(const blockqueue<T>& bq)=delete;
        int m_thread_num;
        int waitnum=0;
        std::vector<std::thread> m_pool;
        std::queue<T> m_q;
        int m_max;
        int m_num=0;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::atomic<bool> m_flag;
    };

}
