#include <os/Atomic.hpp>
#include <list>

#include <os/Thread.hpp>
#include <log/LogManager.hpp>

using namespace os;
using namespace std;
using namespace logger;

class Producer: public Runnable {
public:
    Producer( AtomicHolder< list<int> >& vec ):
        m_atomic(vec) {}

    void run() {
        for(int i = 0; i < 10000000; ++ i){
            m_atomic.get()->push_back(i);
        }
    }

    AtomicHolder< list<int> >& m_atomic;
};

class Consumer: public Runnable {
public:
    Consumer( AtomicHolder< list<int> >& vec ):
        m_atomic(vec) {}

    void run() {
        LogContext& log = LogManager::instance().getLogContext("Test", "Consumer");
        int x = 0;
        while(x < 1000000) {
            if(!m_atomic.get()->empty()) {
                x = m_atomic.get()->front();
                m_atomic.get()->pop_front();
                if(x % 10000 == 0) {
                    log.printfln(INFO, "%d%% complete", x/10000);
                }
            }
        }
    }

    AtomicHolder< list<int> >& m_atomic;
};

int main(int argc, char** argv) {
    AtomicHolder< list<int> > lst;

    Consumer consumer(lst);
    Producer producer(lst);

    Thread* th1 = new Thread(consumer);
    Thread* th2 = new Thread(producer);

    th1->start();
    th2->start();

    th1->join();
    return 0;
}
