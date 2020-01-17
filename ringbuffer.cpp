

                    Ring Buffer
                  Data Structure
                        by
                    Paul Caron
                       2020
                   
A ring buffer is basically an array that loops end back to the beginning. It is useful for streaming data into and out the ring buffer. It is convenient because the memory has a fixed size and it doesnt need any maintenance.

Here is my own implementation of a ring buffer. There is a data stream that is being written into the buffer from the first thread. Then the data is being read from the buffer in the second thread. It all happens in parallel and a simple mutex is protecting the data structure.

*/
#include <iostream>
#include <optional>
#include <thread>
#include <algorithm>
#include <mutex>
#define BUFFSIZE 20
#define STREAMSIZE 200
using namespace std;

template <class T, size_t Size>
class RingBuffer{
    public:
    mutex mtx;
    class iterator{
        public:
        RingBuffer* parent;
        using difference_type =   size_t;
        using value_type =        T;
        using pointer =           T*;
        using reference =         T&;
        using iterator_category = forward_iterator_tag;
        size_t pos=0;
        unsigned long long cycle=0;
        iterator(RingBuffer*rb):parent{rb}{}
        iterator(RingBuffer*rb,size_t i,size_t c=0):parent{rb},pos{i},cycle{c}{}
        T& operator*(){
            return *(parent->arr+pos);
        }
        iterator operator+(size_t index){
            unsigned addCycle=pos+index>=Size?1:0;
            size_t p=(pos+index)%Size;
            size_t c=cycle+addCycle;
            return iterator(parent, p, c);
        }
        iterator& operator++(){
            ++pos;
            unsigned addCycle=pos>=Size?1:0;
            pos%=Size;
            cycle+=addCycle;
            return *this;
        }
        iterator operator++(int){
            iterator temp(*this);
            operator++();
            return temp;
        }
        bool operator!=(const iterator& other){
            return !operator==(other);
        }
        bool operator==(iterator other){
            return (pos==other.pos&&cycle==other.cycle);
        }
        bool operator<(iterator other){
            return pos+cycle*Size<other.pos+other.cycle*Size;
        }
        bool operator<=(iterator other){
            return pos+cycle*Size<=other.pos+other.cycle*Size;
        }
        bool operator>(iterator other){
            return pos+cycle*Size>other.pos+other.cycle*Size;
        }
        bool operator>=(iterator other){
            return pos+cycle*Size>=other.pos+other.cycle*Size;
        }
    };
    T arr[Size]={};
    RingBuffer(){}
    bool push(T& value){
        lock_guard<mutex>lock(mtx);
        if(write<read+Size){
            *(write++) = value;
            return true;
        }
        return false;
    }
    optional<T> pop(){
        lock_guard<mutex>lock(mtx);
        if(read!=write) return *(read++);
        resetCycle();
        return {};
    }
    iterator begin(){return read;}
    iterator end(){return write;}
    private:
    iterator read{this};
    iterator write{this};
    void resetCycle(){
        read.cycle=0;
        write.cycle=0;
    }
};


void writeToBuffer(RingBuffer<int,BUFFSIZE>& rb){
    for(int i=0;i<STREAMSIZE;i++){
        if(rb.push(i)) continue ;
        i--;
    }
}

void readFromBuffer(RingBuffer<int,BUFFSIZE>& rb){
    optional<int>oi;
    for(int i=0;i<STREAMSIZE;i++){
        cout<<"distance from read to write iterators: "
            <<distance(rb.begin(),rb.end())
            <<endl;
        if((oi=rb.pop())){
            cout<<"expected: "
                <<i
                <<" read value: "
                <<oi.value()
                <<endl<<endl;
        }else i--;
    }
}

int main() {
    RingBuffer<int,BUFFSIZE>rb{};
    thread t1(writeToBuffer, ref(rb));
    thread t2(readFromBuffer, ref(rb));
    t1.join();
    t2.join();
    return 0;
}









