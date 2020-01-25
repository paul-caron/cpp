//author:Paul Caron

#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

constexpr char chars[]=" .:;!?7#@#7?!;:. .  .   .    .     ";

inline void spiral(double k){
    const vector<double>row(41,0.0);
    vector<vector<double>>matrix(41,row);
    const double ms=matrix.size();
    const double m=round(ms/2)-1;
    for(double i=0;i<=100;i+=0.25){
        const double a=i*k,r=i/5;
        matrix.at(round(r*sin(a)+m))
              .at(round(r*cos(a)+m))+=r/1.5;
    }
    for(auto row:matrix){
        for(auto value:row)
            cout<<chars[static_cast<int>(round(value))];
        cout<<endl;
    }
}

int main() {
    for(double k=1;k<50;k+=1){
        cout<<k<<endl;
        spiral(k);
    }
    return 0;
}
