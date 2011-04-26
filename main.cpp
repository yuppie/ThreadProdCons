#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <vector>

const int t1 = 500;             ///< Time in millisec of producer writing
const int t2 = 500;             ///< Time in millisec of customer reading & deleting
const int N = 10;               ///< Buffer size
const int randProd = 1000;      ///< Coefficient of producer random creating data
const int randCons = 3000;      ///< Coefficient of consumer random processing data

boost::mutex mutex;
std::vector<int> data (N, 1);

/// Flags for condition variables
static bool b_condFull = (data.size() == N);
static bool b_condEmpty = data.empty();

boost::condition_variable condFull;
boost::condition_variable condEmpty;


void wait(long milliseconds)
{
   boost::this_thread::sleep(boost::posix_time::millisec(milliseconds));
}

/// Producer thread function
void product()
{
    long generate(0);

    while (true)
    {
        /// Emulation of creation of package of data
        generate = (double (rand())/RAND_MAX)*randProd;
        wait(generate);

        /// Lock mutex with unique lock to enable conditon variables work
        boost::unique_lock<boost::mutex> lock(mutex);
        std::cout<<boost::get_system_time()<<" Producer created new pack of data. In buff: "<<data.size()<<'\n';

        if(data.size() < N)
        {
            /// Emulate writing data
            wait(t1);
            data.push_back(1);
            std::cout<<boost::get_system_time()<<" Producer wrote data. In buff:"<<data.size()<<'\n';
            lock.unlock();

            /// If buffer was empty we need to notify appropriate cond. variable
            b_condEmpty = false;
            condEmpty.notify_one();
        }
        else
        {
            /// If buffer is full
            b_condFull = true;
            std::cout<<boost::get_system_time()<<" Producer waiting...(Buffer is full)\n";

            /// Wait while consumer read and delete data package
            while(b_condFull)
            {
                condFull.wait(lock);
            }
        }
    }
}

/// Consumer thread function
void consume()
{
    long generate(0);

    while (true)
    {
        /// Try to get some data
        boost::unique_lock<boost::mutex> lock(mutex);
        if(!data.empty())
        {
            /// Emulate reading and deleting
            wait(t2);
            data.pop_back();
            std::cout<<boost::get_system_time()<<" Consumer read & deleted data. In buff:"<<data.size()<<'\n';
            lock.unlock();

            /// If buffer was full we need to notify appropriate cond. variable
            b_condFull = false;
            condFull.notify_one();
        }
        else
        {
            /// If buffer is empty
            b_condEmpty = true;
            std::cout<<boost::get_system_time()<<" Consumer waiting... (Empty buff)"<<'\n';

            /// Wait while producer write some data
            while(b_condEmpty)
            {
                condEmpty.wait(lock);
            }
        }

        /// Emulate data processing
        generate = (double (rand())/RAND_MAX)*randCons;
        wait(generate);
        std::cout<<boost::get_system_time()<<" Consumer processed data. In buff: "<<data.size()<<'\n';
    }
}


int main()
{
  boost::thread producer(product);
  boost::thread consumer(consume);
  producer.join();
  consumer.join();
}