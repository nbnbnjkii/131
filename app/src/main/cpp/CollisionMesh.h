#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace physics {
struct Vec3 {
    float x=0,y=0,z=0;
    Vec3 operator+(Vec3 b)const{return {x+b.x,y+b.y,z+b.z};}
    Vec3 operator-(Vec3 b)const{return {x-b.x,y-b.y,z-b.z};}
    Vec3 operator*(float s)const{return {x*s,y*s,z*s};}
};
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(Vec3 a){return std::sqrt(dot(a,a));}
inline Vec3 normalized(Vec3 a){float l=length(a);return l>1e-7f?a*(1/l):Vec3{0,1,0};}
struct Triangle {Vec3 a,b,c;};
struct Bounds {
    Vec3 lo{1e30f,1e30f,1e30f},hi{-1e30f,-1e30f,-1e30f};
    void add(Vec3 p){lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
    bool overlaps(const Bounds&b)const{return lo.x<=b.hi.x&&hi.x>=b.lo.x&&lo.y<=b.hi.y&&hi.y>=b.lo.y&&lo.z<=b.hi.z&&hi.z>=b.lo.z;}
};
inline Vec3 closestTriangle(Vec3 p,const Triangle&t){
    Vec3 ab=t.b-t.a,ac=t.c-t.a,ap=p-t.a;float d1=dot(ab,ap),d2=dot(ac,ap);
    if(d1<=0&&d2<=0)return t.a;
    Vec3 bp=p-t.b;float d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3)return t.b;
    float vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return t.a+ab*(d1/(d1-d3));
    Vec3 cp=p-t.c;float d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6)return t.c;
    float vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return t.a+ac*(d2/(d2-d6));
    float va=d3*d6-d5*d4;if(va<=0&&d4-d3>=0&&d5-d6>=0)return t.b+(t.c-t.b)*((d4-d3)/((d4-d3)+(d5-d6)));
    float denominator=va+vb+vc;if(std::abs(denominator)<1e-12f)return t.a;
    return t.a+ab*(vb/denominator)+ac*(vc/denominator);
}
inline void closestSegments(Vec3 p,Vec3 q,Vec3 a,Vec3 b,Vec3 &onCapsule,Vec3 &onEdge){
    Vec3 d=q-p,e=b-a,r=p-a;float dd=dot(d,d),ee=dot(e,e),de=dot(d,e),dr=dot(d,r),er=dot(e,r);
    float s=0,t=0;
    if(dd<=1e-12f){t=ee>1e-12f?std::clamp(er/ee,0.f,1.f):0;}
    else if(ee<=1e-12f)s=std::clamp(-dr/dd,0.f,1.f);
    else{
        float denominator=dd*ee-de*de;
        if(denominator>1e-12f)s=std::clamp((de*er-dr*ee)/denominator,0.f,1.f);
        t=(de*s+er)/ee;
        if(t<0){t=0;s=std::clamp(-dr/dd,0.f,1.f);}else if(t>1){t=1;s=std::clamp((de-dr)/dd,0.f,1.f);}
    }
    onCapsule=p+d*s;onEdge=a+e*t;
}
inline float capsuleTriangle(Vec3 p,Vec3 q,const Triangle&t,Vec3 &normal){
    Vec3 bestP=p,bestT=closestTriangle(p,t);float best=dot(bestP-bestT,bestP-bestT);
    auto consider=[&](Vec3 a,Vec3 b){float d=dot(a-b,a-b);if(d<best){best=d;bestP=a;bestT=b;}};
    consider(q,closestTriangle(q,t));
    for(auto edge:std::array<std::array<Vec3,2>,3>{{{t.a,t.b},{t.b,t.c},{t.c,t.a}}}){
        Vec3 a,b;closestSegments(p,q,edge[0],edge[1],a,b);consider(a,b);
    }
    Vec3 n=normalized(cross(t.b-t.a,t.c-t.a)),direction=q-p;
    float denominator=dot(n,direction);
    if(std::abs(denominator)>1e-7f){float f=dot(n,t.a-p)/denominator;if(f>=0&&f<=1){Vec3 h=p+direction*f;if(length(h-closestTriangle(h,t))<.0001f)consider(h,h);}}
    float distance=std::sqrt(std::max(0.f,best));
    if(distance>1e-6f)normal=(bestP-bestT)*(1/distance);
    else{normal=n;if(dot((p+q)*.5f-t.a,normal)<0)normal=normal*-1;}
    return distance;
}
// Immutable after build(). Shared across rendering, physics and worker snapshots.
class CollisionMesh {
    struct Node{Bounds bounds;int begin=0,end=0,left=-1,right=-1;};
    std::vector<Triangle> triangles_;
    std::vector<int> order_;
    std::vector<Node> nodes_;
    int buildNode(int begin,int end){
        int index=int(nodes_.size());nodes_.push_back({});Bounds bounds,centers;
        for(int i=begin;i<end;i++){auto&t=triangles_[order_[i]];bounds.add(t.a);bounds.add(t.b);bounds.add(t.c);centers.add((t.a+t.b+t.c)*(1.f/3));}
        nodes_[index].bounds=bounds;nodes_[index].begin=begin;nodes_[index].end=end;
        if(end-begin>8){Vec3 extent=centers.hi-centers.lo;int axis=extent.y>extent.x?1:0;if(extent.z>(axis==0?extent.x:extent.y))axis=2;
            auto value=[&](int i){auto&t=triangles_[i];Vec3 c=t.a+t.b+t.c;return axis==0?c.x:axis==1?c.y:c.z;};
            int mid=(begin+end)/2;std::nth_element(order_.begin()+begin,order_.begin()+mid,order_.begin()+end,[&](int a,int b){return value(a)<value(b);});
            int left=buildNode(begin,mid),right=buildNode(mid,end);nodes_[index].left=left;nodes_[index].right=right;
        }return index;
    }
    template<class F> void visit(int index,const Bounds&b,F &f)const{
        auto&node=nodes_[index];if(!node.bounds.overlaps(b))return;
        if(node.left<0){for(int i=node.begin;i<node.end;i++)f(triangles_[order_[i]]);}
        else{visit(node.left,b,f);visit(node.right,b,f);}
    }
public:
    void add(Triangle t){if(length(cross(t.b-t.a,t.c-t.a))>1e-6f)triangles_.push_back(t);}
    void addBox(Vec3 lo,Vec3 hi){
        Vec3 p[8]={{lo.x,lo.y,lo.z},{hi.x,lo.y,lo.z},{hi.x,hi.y,lo.z},{lo.x,hi.y,lo.z},{lo.x,lo.y,hi.z},{hi.x,lo.y,hi.z},{hi.x,hi.y,hi.z},{lo.x,hi.y,hi.z}};
        const int faces[12][3]={{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,4,7},{0,7,3},{1,2,6},{1,6,5},{3,7,6},{3,6,2},{0,1,5},{0,5,4}};
        for(auto &f:faces)add({p[f[0]],p[f[1]],p[f[2]]});
    }
    void append(const CollisionMesh &other){triangles_.insert(triangles_.end(),other.triangles_.begin(),other.triangles_.end());}
    void build(){order_.resize(triangles_.size());std::iota(order_.begin(),order_.end(),0);nodes_.clear();if(!order_.empty())buildNode(0,int(order_.size()));}
    size_t size()const{return triangles_.size();}
    template<class F> void query(const Bounds&b,F f)const{if(!nodes_.empty())visit(0,b,f);}
    struct RayHit { Vec3 position,normal;float distance; };
    bool raycast(Vec3 origin,Vec3 direction,float maxDistance,RayHit &hit)const{
        direction=normalized(direction);float nearest=maxDistance;bool found=false;
        Bounds bounds;bounds.add(origin);bounds.add(origin+direction*maxDistance);
        query(bounds,[&](const Triangle&t){
            Vec3 e1=t.b-t.a,e2=t.c-t.a,h=cross(direction,e2);float det=dot(e1,h);
            if(std::abs(det)<1e-7f)return;
            float inv=1/det;Vec3 s=origin-t.a;float u=dot(s,h)*inv;if(u<0||u>1)return;
            Vec3 q=cross(s,e1);float v=dot(direction,q)*inv;if(v<0||u+v>1)return;
            float distance=dot(e2,q)*inv;if(distance<.001f||distance>nearest)return;
            Vec3 n=normalized(cross(e1,e2));if(dot(n,direction)>0)n=n*-1;
            nearest=distance;hit={origin+direction*distance,n,distance};found=true;
        });return found;
    }
    bool intersectsBox(Vec3 center,Vec3 half)const{
        Bounds b;b.add(center-half);b.add(center+half);bool hit=false;
        query(b,[&](const Triangle&t){
            if(hit)return;
            Vec3 a=t.a-center,c=t.b-center,d=t.c-center;
            auto separates=[&](Vec3 axis){float p=dot(a,axis),q=dot(c,axis),r=dot(d,axis);
                float radius=std::abs(axis.x)*half.x+std::abs(axis.y)*half.y+std::abs(axis.z)*half.z;
                return std::min({p,q,r})>radius||std::max({p,q,r})<-radius;};
            const Vec3 axes[3]={{1,0,0},{0,1,0},{0,0,1}};
            for(auto axis:axes)if(separates(axis))return;
            Vec3 edges[3]={c-a,d-c,a-d};
            if(separates(cross(edges[0],edges[1])))return;
            for(auto edge:edges)for(auto axis:axes)if(separates(cross(edge,axis)))return;
            hit=true;
        });return hit;
    }
    bool groundContact(float x,float z,float below,float above,RayHit &hit,bool upperOnly=false)const{
        bool found=false;float best=-std::numeric_limits<float>::infinity();Bounds b;b.add({x-.001f,above,z-.001f});b.add({x+.001f,below,z+.001f});
        query(b,[&](const Triangle&t){Vec3 n=cross(t.b-t.a,t.c-t.a);float l=length(n);if(l<1e-8f||std::abs(n.y)/l<.64f||(upperOnly&&n.y<0))return;
            float y=t.a.y-(n.x*(x-t.a.x)+n.z*(z-t.a.z))/n.y;
            if(y>below+.001f||y<above||y<=best)return;
            Vec3 p{x,y,z};if(length(p-closestTriangle(p,t))<.005f){best=y;n=n*(1/l);if(n.y<0)n=n*-1;hit={p,n,below-y};found=true;}
        });return found;
    }
    float groundHeight(float x,float z,float below=1000,float above=-1000)const{
        float best=-std::numeric_limits<float>::infinity();Bounds b;b.add({x-.001f,above,z-.001f});b.add({x+.001f,below,z+.001f});
        query(b,[&](const Triangle&t){Vec3 n=cross(t.b-t.a,t.c-t.a);float l=length(n);if(l<1e-8f||std::abs(n.y)/l<.64f)return;
            float y=t.a.y-(n.x*(x-t.a.x)+n.z*(z-t.a.z))/n.y;
            if(y>below+.001f||y<above||y<=best)return;
            Vec3 p{x,y,z};if(length(p-closestTriangle(p,t))<.005f)best=y;
        });return best;
    }
};
}
