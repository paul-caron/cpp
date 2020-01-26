//author: Paul Caron
//year 2020

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>

#define ms 43//console width

using namespace std;

constexpr double cos30 = 0.86602540378;
constexpr double sin30 = 0.5;

struct Point2d{
    const double x,y;
    Point2d(double x,double y):x(x),y(y){}
    Point2d(Point2d&&p):x(move(p.x)),y(move(p.y)){}
    Point2d(const Point2d& p):x(p.x),y(p.y){}
};

struct Segment{
    const Point2d A,B;
    const double m,c;
    Segment(Point2d&& a,Point2d&& b):
        A(forward<Point2d>(a.x<b.x?a:b)),
        B(forward<Point2d>(a.x<b.x?b:a)),
        m((B.y-A.y)/(B.x-A.x)),
        c(A.y-m*A.x){}
    Segment(Segment&&s):A(move(s.A)),
                        B(move(s.B)),
                        m(move(s.m)),
                        c(move(s.c)){}
    Segment(const Segment& s):A(s.A),
                              B(s.B),
                              m(s.m),
                              c(s.c){}
};

bool segments_intersect(const Segment& s1,const Segment& s2){
    if(s1.m==s2.m) return false;
    if(s1.m==INFINITY||s1.m==-INFINITY){
        const double x=s1.A.x;
        if(x<s2.A.x||x>s2.B.x) return false;
        const double y=s2.m*x+s2.c;
        if(y<min(s1.A.y,s1.B.y)||y>max(s1.A.y,s1.B.y)) return false;
        return true;
    }
    if(s2.m==INFINITY||s2.m==-INFINITY){
        const double x=s2.A.x;
        if(x<s1.A.x||x>s1.B.x) return false;
        const double y=s1.m*x+s1.c;
        if(y<min(s2.A.y,s2.B.y)||y>max(s2.A.y,s2.B.y)) return false;
        return true;
    }
    const double x=(s2.c-s1.c)/(s1.m-s2.m);
    if(x<s1.A.x||x<s2.A.x||x>s1.B.x||x>s2.B.x)
        return false;
    const double y=s1.m*x+s1.c;
    if(y<min({s1.A.y,s1.B.y,s2.A.y,s2.B.y})||
       y>max({s1.A.y,s1.B.y,s2.A.y,s2.B.y}))
        return false;
    return true;
}

struct Point{
    double x,y,z;
    Point(double x,double y,double z):x(x),y(y),z(z){}
};

struct Cube{
    vector<Point>points;
    vector<Point>frontface;
    vector<Point>sideface;
    vector<Point>bottomface;
    Cube(double height,double size,double x,double y,double z){
        double left=x-size/2,
            right=x+size/2,
            top=y+size/2,
            bottom=y-size/2-height*size,
            front=z-size/2,
            back=z+size/2;
        points.emplace_back(left,top,front);
        points.emplace_back(left,top,back);
        points.emplace_back(right,top,back);
        points.emplace_back(right,top,front);
        points.emplace_back(left,bottom,front);
        points.emplace_back(left,bottom,back);
        points.emplace_back(right,bottom,back);
        points.emplace_back(right,bottom,front);
        frontface.emplace_back(left,bottom,front);
        frontface.emplace_back(left,top,front);
        frontface.emplace_back(right,top,front);
        frontface.emplace_back(right,bottom,front);
        sideface.emplace_back(right,bottom,front);
        sideface.emplace_back(right,top,front);
        sideface.emplace_back(right,top,back);
        sideface.emplace_back(right,bottom,back);
        bottomface.emplace_back(left,bottom,front);
        bottomface.emplace_back(left,bottom,back);
        bottomface.emplace_back(right,bottom,back);
        bottomface.emplace_back(right,bottom,front);
    }
};

inline double projectionX(const Point & p){
    return p.x*cos30+p.z*cos30;
}

inline double projectionY(const Point & p){
    return p.y+p.x*sin30-p.z*sin30;
}

void cubes(){
    const vector<char>row(ms,' ');
    vector<vector<char>>matrix(ms*2,row);
    const double mid=((ms-1)/2.0)/cos30;
    double cs=mid/4.0;//3×3 columns
    vector<Cube>cubes{
    {(double)(rand()%3),cs,mid/8.0,11.5*mid/8.0+cs*sin30,mid/8.0+3*cs},
    {(double)(rand()%3),cs,mid/8.0+cs,11.5*mid/8.0+cs*sin30,mid/8.0+3*cs},
    {(double)(rand()%3),cs,mid/8.0+2*cs,11.5*mid/8.0+cs*sin30,mid/8.0+3*cs},
    {(double)(rand()%3),cs,mid/8.0+3*cs,11.5*mid/8.0+cs*sin30,mid/8.0+3*cs},
    //back row
    {(double)(rand()%3),cs,mid/8.0,11.5*mid/8.0+cs*sin30,mid/8.0+2*cs},
    {(double)(rand()%3),cs,mid/8.0+cs,11.5*mid/8.0+cs*sin30,mid/8.0+2*cs},
    {(double)(rand()%3),cs,mid/8.0+2*cs,11.5*mid/8.0+cs*sin30,mid/8.0+2*cs},
    {(double)(rand()%3),cs,mid/8.0+3*cs,11.5*mid/8.0+cs*sin30,mid/8.0+2*cs},
    //mid row
    {(double)(rand()%3),cs,mid/8.0,11.5*mid/8.0+cs*sin30,mid/8.0+cs},
    {(double)(rand()%3),cs,mid/8.0+cs,11.5*mid/8.0+cs*sin30,mid/8.0+cs},
    {(double)(rand()%3),cs,mid/8.0+2*cs,11.5*mid/8.0+cs*sin30,mid/8.0+cs},
    {(double)(rand()%3),cs,mid/8.0+3*cs,11.5*mid/8.0+cs*sin30,mid/8.0+cs},
    //front row
    {(double)(rand()%3),cs,mid/8.0,11.5*mid/8.0+cs*sin30,mid/8.0},
    {(double)(rand()%3),cs,mid/8.0+cs,11.5*mid/8.0+cs*sin30,mid/8.0},
    {(double)(rand()%3),cs,mid/8.0+2*cs,11.5*mid/8.0+cs*sin30,mid/8.0},
    {(double)(rand()%3),cs,mid/8.0+3*cs,11.5*mid/8.0+cs*sin30,mid/8.0},
    
    };
    for(auto c:cubes){
        Point p0=c.frontface.at(0);
        Point p1=c.frontface.at(1);
        Point p2=c.frontface.at(2);
        Point p3=c.frontface.at(3);
        vector<Segment>segments{};
        segments.push_back(Segment(
                {projectionX(p0),projectionY(p0)},
                {projectionX(p1),projectionY(p1)})
        );
        segments.push_back(Segment(
                {projectionX(p1),projectionY(p1)},
                {projectionX(p2),projectionY(p2)})
        );
        segments.push_back(Segment(
                {projectionX(p2),projectionY(p2)},
                {projectionX(p3),projectionY(p3)})
        );
        segments.push_back(Segment(
                {projectionX(p3),projectionY(p3)},
                {projectionX(p0),projectionY(p0)})
        );
        
        for(double y=0;y<ms*2;y++){
            for(double x=0;x<ms;x++){
                int count=0;
                for(const auto &s:segments){
                    Segment ray({-1.0,y},{x,y});
                    if(segments_intersect(s,ray)==true){
                        count++;
                    }
                }
                if(count%2==1){
                    matrix.at(round(y))
                          .at(round(x))='!';
                }
                
            } 
        }
        
        
        p0=c.sideface.at(0);
        p1=c.sideface.at(1);
        p2=c.sideface.at(2);
        p3=c.sideface.at(3);
        segments.clear();
        segments.push_back(Segment(
                {projectionX(p0),projectionY(p0)},
                {projectionX(p1),projectionY(p1)})
        );
        segments.push_back(Segment(
                {projectionX(p1),projectionY(p1)},
                {projectionX(p2),projectionY(p2)})
        );
        segments.push_back(Segment(
                {projectionX(p2),projectionY(p2)},
                {projectionX(p3),projectionY(p3)})
        );
        segments.push_back(Segment(
                {projectionX(p3),projectionY(p3)},
                {projectionX(p0),projectionY(p0)})
        );
        for(double y=0;y<ms*2;y++){
            for(double x=0;x<ms;x++){
                int count=0;
                for(const auto &s:segments){
                    Segment ray({-1.0,y},{x,y});
                    if(segments_intersect(s,ray)==true){
                        count++;
                    }
                }
                if(count%2==1){
                    matrix.at(round(y))
                          .at(round(x))='/';
                }
                
            } 
        }
        
        
        p0=c.bottomface.at(0);
        p1=c.bottomface.at(1);
        p2=c.bottomface.at(2);
        p3=c.bottomface.at(3);
        segments.clear();
        segments.push_back(Segment(
                {projectionX(p0),projectionY(p0)},
                {projectionX(p1),projectionY(p1)})
        );
        segments.push_back(Segment(
                {projectionX(p1),projectionY(p1)},
                {projectionX(p2),projectionY(p2)})
        );
        segments.push_back(Segment(
                {projectionX(p2),projectionY(p2)},
                {projectionX(p3),projectionY(p3)})
        );
        segments.push_back(Segment(
                {projectionX(p3),projectionY(p3)},
                {projectionX(p0),projectionY(p0)})
        );
        for(double y=0;y<ms*2;y++){
            for(double x=0;x<ms*2;x++){
                int count=0;
                for(const auto &s:segments){
                    Segment ray({-1.0,y},{x,y});
                    if(segments_intersect(s,ray)==true){
                        count++;
                    }
                }
                if(count%2==1){
                    matrix.at(round(y))
                          .at(round(x))='%';
                }
            } 
        }
        for(auto p:c.points){
            double x=projectionX(p);
            double y=projectionY(p);
            matrix.at(round(y)).at(round(x))='+';
        }
    }
    for(const auto& row:matrix){
        for(const auto& value:row)
            cout<<value;
        cout<<endl;
    }
}

int main() {
    srand(time(0));
    for(int i=0;i<5;i++){
        cout<<string(43,'_')<<endl;
        cubes();
        cout<<string(43,'_')<<endl;
    }
    return 0;
}
