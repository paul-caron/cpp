 /*

           Metaprogramming
           type templates
          initializer_list 
          
           Data structure
          The Array Class
         (better than stl)
                by
             Paul Caron
               2020

*/
#include <iostream>
#include <iterator>
#include <algorithm>
#include <array>//only used for comparison demo
using namespace std;

template<class T,size_t Size>
class Array{
    public:
    T arr[Size];
    class iterator{
        public:
        Array* parent;
        using difference_type =   ptrdiff_t;
        using value_type =        T;
        using pointer =           T*;
        using reference =         T&;
        using iterator_category = random_access_iterator_tag;
        size_t pos;
        iterator(Array*p):parent{p},pos{0}{}
        iterator(Array*p,size_t i):parent{p},pos{i}{}
        iterator operator+(size_t i){
            if(i+pos<Size)
            return iterator(parent,pos+i);
            cerr<<"Index out range ";
            exit(1);
        }
        iterator operator-(size_t i){
            return iterator(parent,pos-i);
        }
        T& operator*(){
            if(pos<Size)
            return *(parent->arr+pos);
            cerr<<"Index out range ";
            exit(1);
        }
        iterator& operator++(){
            pos++;
            return *this;
        }
        iterator operator++(int){
            iterator temp(*this);
            operator++();
            return temp;
        }
        iterator& operator--(){
            pos--;
            return *this;
        }
        iterator operator--(int){
            iterator temp(*this);
            operator--();
            return temp;
        }
        bool operator!=(iterator other){
            return pos!=other.pos;
        }
        bool operator==(iterator other){
            return pos==other.pos;
        }
        bool operator<(iterator other){
            return pos<other.pos;
        }
        bool operator<=(iterator other){
            return pos<=other.pos;
        }
        bool operator>(iterator other){
            return pos>other.pos;
        }
        bool operator>=(iterator other){
            return pos>=other.pos;
        }
    };
    Array(){};
    template <class iterator>
    Array(iterator i, iterator j){
        if(size_t(distance(i,j))<=Size)
        copy(i,j,arr);
    }
    Array(initializer_list<T>lst){
        if(lst.size()<=Size)
        copy(lst.begin(),lst.end(),arr);
    }
    iterator begin(){return iterator(this);}
    iterator end(){return iterator(this, Size);}
    T& operator[](size_t index){
        if(index>=Size){
            cerr<<"Index out range ";
            exit(1);
        }
        return arr[index];
    }
    iterator operator+(size_t index){
        if(index>=Size){
            cerr<<"Index out range ";
            exit(1);
        }
        return iterator(this,index);
    }
    T& operator*(){
        return arr[0];
    }
};

int main() {
    cout<<"1d array of integers"<<endl;
    Array<int,4>a{1,2,3,4};
    for(auto i:a) cout<<i;
    cout<<*(a.begin()++)<<endl<<endl;

    cout<<"1d array of char"<<endl;
    Array<char,5>b{'a','b','c','d','e'};
    for(auto c:b) cout<<c;
    cout<<endl<<endl;
    
    cout<<"1d array of char(reverse)"<<endl;
    reverse(b.begin(),b.end());
    for(auto c:b) cout<<c;
    cout<<endl<<endl;
    
    string s="qwerty";
    cout<<"1d array from iterators"<<endl;
    Array<char,6>c{s.begin(),s.end()};
    for(auto d:c) cout<<d;
    cout<<endl<<endl;
    
    cout<<"2d array initialisation"<<endl;
    Array<Array<int,4>,2> arr={
        {1,2,3,4},
        {5,6,7,8}
    };
    for(auto i:arr){
        for(auto j:i)
            cout<<j<<" ";
        cout<<endl;
    }
    cout<<"3d array initialisation"<<endl;
    Array<Array<Array<int,2>,2>,2> arr3={
        {{1,2},{3,4}},
        {{5,6},{7,8}}
    };
    for(auto i:arr3){
        for(auto j:i){
            cout<<"{ ";
            for(auto k:j)
                cout<<k<<" ";
            cout<<"}";
        }
        cout<<endl;
    }
    cout<<endl; 
    cout<<"2d array initialisation with std::array and extra necessary curly brackets that are little bit sucky. Just to showcase the difference"<<endl;
    array<array<int,4>,2> arr2{{
        {1,2,3,4},
        {5,6,7,8}
    }};//notice the annoying extra curly brackets.
    for(auto i:arr2){
        for(auto j:i)
            cout<<j<<" ";
        cout<<endl;
    }
    return 0;
}












