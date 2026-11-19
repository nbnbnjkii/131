#pragma once
#include "SandboxWorld.h"
#include <array>
#include <limits>
#include <queue>

namespace city {
struct SpawnPoint {float x=0,z=0;size_t reachableCells=0;bool playerInRegion=false;};
// Prefer the requested sizeable street component; fall back to the largest region.
inline bool findRoadSpawn(const sandbox::World &world,const std::array<float,3>&lo,
                          const std::array<float,3>&hi,float preferredX,float preferredZ,SpawnPoint &out,std::vector<std::array<float,2>> *reachable=nullptr){
    constexpr float step=.25f;
    int width=int((hi[0]-lo[0])/step),depth=int((hi[2]-lo[2])/step);
    if(width<3||depth<3||width>512||depth>512)return false;
    std::vector<unsigned char> free(size_t(width)*depth),seen(free.size());
    auto px=[&](int i){return lo[0]+(i%width+.5f)*step;};
    auto pz=[&](int i){return lo[2]+(i/width+.5f)*step;};
    for(int i=0;i<width*depth;i++)free[i]=!world.blocked({px(i),0,pz(i),.62f,2,.62f,0,0,0});
    std::vector<int> largest,preferredRegion;
    int preferredCellX=int(std::floor((preferredX-lo[0])/step)),preferredCellZ=int(std::floor((preferredZ-lo[2])/step));
    int preferredCell=preferredCellX>=0&&preferredCellX<width&&preferredCellZ>=0&&preferredCellZ<depth?preferredCellZ*width+preferredCellX:-1;
    for(int start=0;start<width*depth;start++){
        if(!free[start]||seen[start])continue;
        std::vector<int> component{start};seen[start]=1;
        for(size_t head=0;head<component.size();head++){
            int i=component[head],x=i%width,z=i/width;
            for(auto d:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){
                int nx=x+d[0],nz=z+d[1];if(nx<0||nx>=width||nz<0||nz>=depth)continue;
                int next=nz*width+nx;if(free[next]&&!seen[next]){seen[next]=1;component.push_back(next);}
            }
        }
        // An outside-of-buildings area can be larger than the actual street.
        // Prefer the requested street component, but reject small fenced pockets (<16 m²).
        if(component.size()>=256&&std::find(component.begin(),component.end(),preferredCell)!=component.end())preferredRegion=component;
        if(component.size()>largest.size())largest=std::move(component);
    }
    if(!preferredRegion.empty())largest=std::move(preferredRegion);
    if(largest.size()<64)return false;
    if(reachable){reachable->clear();for(int i:largest)reachable->push_back({px(i),pz(i)});}
    int currentX=int(std::floor((world.x-lo[0])/step)),currentZ=int(std::floor((world.z-lo[2])/step));
    bool playerInRegion=currentX>=0&&currentX<width&&currentZ>=0&&currentZ<depth&&std::find(largest.begin(),largest.end(),currentZ*width+currentX)!=largest.end();
    // Prefer at least 1.5 m from surrounding obstacles; only relax on narrow maps.
    for(float clearance:{3.f,2.f,1.2f}){
        float best=std::numeric_limits<float>::max();bool found=false;
        for(int i:largest){float x=px(i),z=pz(i);if(world.blocked({x,0,z,clearance,2,clearance,0,0,0}))continue;
            float score=(x-preferredX)*(x-preferredX)+(z-preferredZ)*(z-preferredZ);
            if(score<best){best=score;out={x,z,largest.size(),playerInRegion};found=true;}
        }
        if(found)return true;
    }
    return false;
}
inline bool findCarParking(const sandbox::World &world,const std::array<float,3>&lo,
                            const std::array<float,3>&hi,float nearX,float nearZ,float &x,float &z){
    float best=std::numeric_limits<float>::max();bool found=false;
    // Park in the main reachable region, with a walkable perimeter on all four sides.
    SpawnPoint road;std::vector<std::array<float,2>> reachable;
    if(!findRoadSpawn(world,lo,hi,nearX,nearZ,road,&reachable))return false;
    for(const auto &point:reachable){float px=point[0],pz=point[1];
        if(world.blocked({px,0,pz,3.1f,1.2f,5.7f,0,0,0}))continue;
        if(std::hypot(px-world.x,pz-world.z)<6)continue;
        float d=(px-nearX)*(px-nearX)+(pz-nearZ)*(pz-nearZ);
        if(d<best){best=d;x=px;z=pz;found=true;}
    }
    return found;
}
}
