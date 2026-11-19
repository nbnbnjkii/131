// Offline software thumbnails from the SAME ObjectMesh and UAZ GLB used by Android.
// g++ -std=c++17 -O2 tools/render_android_catalog.cpp sys/android/app/src/main/cpp/{GltfScene,ImageDecode}.cpp -o /tmp/catalog
// /tmp/catalog /tmp/catalog-output ; convert PPMs to WebP with Pillow.
#include "../sys/android/app/src/main/cpp/ObjectMesh.h"
#include "../sys/android/app/src/main/cpp/GltfScene.h"
#include "../sys/android/app/src/main/cpp/third_party/stb_image.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using sandbox::MeshVertex;
struct Image{int w=0,h=0;std::vector<unsigned char> rgb;};
int main(int argc,char**argv){
 if(argc!=2)return 1;
 std::filesystem::create_directories(argv[1]);
 const std::string root="sys/android/app/src/main/assets/";
 const char*names[]={"asphalt","concrete","brick","rust","plain","wood","fabric","glass"};
 std::vector<Image> images(8);
 for(int i=0;i<8;i++)if(i!=4){int n;std::ifstream f(root+"textures/"+names[i]+".png",std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});auto*p=stbi_load_from_memory(bytes.data(),int(bytes.size()),&images[i].w,&images[i].h,&n,3);if(!p)return 2;images[i].rgb.assign(p,p+images[i].w*images[i].h*3);stbi_image_free(p);}
 const char*items[]={"wall","floor","window","door","table","chair","cabinet","sofa","uaz","fpv_remote","fpv_drone"};
 for(int type=0;type<11;type++){
  std::vector<MeshVertex> mesh;auto textures=images;
  if(type<8)sandbox::appendObject(mesh,{type,0,0,0});
  else{
   city::Scene scene;city::LoadOptions options;options.level=false;options.buildPhysics=false;options.targetExtent=4.025f;options.yaw=1.57079632679f;std::string log;
   if(!scene.load(root+std::string("models/")+(type==9?"chasing_m2_rc_underwater_drone_remote.glb":type==10?"fpv_drone.glb":"uaz_469_1973_classic.glb"),[](const std::string&p,std::vector<unsigned char>&b){std::ifstream f(p,std::ios::binary);if(!f)return false;b.assign(std::istreambuf_iterator<char>(f),{});return true;},log,options))return 3;
   for(const auto&im:scene.images){Image t;t.w=im.width;t.h=im.height;for(size_t j=0;j<im.rgba.size();j+=4)for(int c=0;c<3;c++)t.rgb.push_back(im.rgba[j+c]);textures.push_back(std::move(t));}
   for(const auto&batch:scene.batches){city::Material fallback;const auto&m=batch.material>=0?scene.materials[batch.material]:fallback;
    for(const auto&v:batch.vertices)mesh.push_back({v.x,v.y,v.z,m.color[0],m.color[1],m.color[2],v.nx,v.ny,v.nz,v.u,v.v,float(m.image>=0?8+m.image:4)});
   }
  }
  // Orthographic view from the front and right, high enough to see tabletops.
  auto project=[](const MeshVertex&v){return std::array<float,3>{.82f*v.x+.57f*v.z,v.y*.88f-v.z*.39f+v.x*.27f,-.50f*v.x+.46f*v.y+.72f*v.z};};
  float minX=1e6f,maxX=-1e6f,minY=1e6f,maxY=-1e6f;
  for(const auto&v:mesh){auto p=project(v);minX=std::min(minX,p[0]);maxX=std::max(maxX,p[0]);minY=std::min(minY,p[1]);maxY=std::max(maxY,p[1]);}
  constexpr int S=320;float scale=272/std::max(maxX-minX,maxY-minY);
  std::vector<unsigned char> pixels(S*S*3);std::vector<float> depth(S*S,-1e9f);
  for(int y=0;y<S;y++)for(int x=0;x<S;x++)for(int c=0;c<3;c++)pixels[(y*S+x)*3+c]=static_cast<unsigned char>(c==0?30:c==1?40:46);
  for(size_t i=0;i+2<mesh.size();i+=3){
   std::array<float,3> p[3];for(int j=0;j<3;j++){p[j]=project(mesh[i+j]);p[j][0]=160+(p[j][0]-(minX+maxX)/2)*scale;p[j][1]=160-(p[j][1]-(minY+maxY)/2)*scale;}
   float area=(p[1][0]-p[0][0])*(p[2][1]-p[0][1])-(p[1][1]-p[0][1])*(p[2][0]-p[0][0]);if(std::abs(area)<.00001f)continue;
   int xa=std::max(0,int(std::floor(std::min({p[0][0],p[1][0],p[2][0]})))),xb=std::min(S-1,int(std::ceil(std::max({p[0][0],p[1][0],p[2][0]}))));
   int ya=std::max(0,int(std::floor(std::min({p[0][1],p[1][1],p[2][1]})))),yb=std::min(S-1,int(std::ceil(std::max({p[0][1],p[1][1],p[2][1]}))));
   const auto&v=mesh[i];int layer=int(v.layer);const auto&t=textures[layer];
   float shade=.63f+.37f*std::abs(v.nx*.35f+v.ny*.82f+v.nz*.45f);
   for(int y=ya;y<=yb;y++)for(int x=xa;x<=xb;x++){
    float a=((p[1][0]-x)*(p[2][1]-y)-(p[1][1]-y)*(p[2][0]-x))/area;
    float b=((p[2][0]-x)*(p[0][1]-y)-(p[2][1]-y)*(p[0][0]-x))/area;float c=1-a-b;
    if(a<0||b<0||c<0)continue;
    if(type==2&&layer==7&&(x+y*2)%4!=0)continue;
    float z=a*p[0][2]+b*p[1][2]+c*p[2][2];int index=y*S+x;if(z<=depth[index])continue;depth[index]=z;
    float u=a*mesh[i].u+b*mesh[i+1].u+c*mesh[i+2].u,w=a*mesh[i].v+b*mesh[i+1].v+c*mesh[i+2].v;
    int tx=t.w?int((u-std::floor(u))*t.w)%t.w:0,ty=t.h?int((w-std::floor(w))*t.h)%t.h:0;
    const float tint[]={v.r,v.g,v.b};for(int channel=0;channel<3;channel++){float sample=t.rgb.empty()?255:t.rgb[(ty*t.w+tx)*3+channel];pixels[index*3+channel]=static_cast<unsigned char>(std::clamp(sample*tint[channel]*shade,0.f,255.f));}
   }
  }
  std::ofstream out(std::string(argv[1])+"/"+items[type]+".ppm",std::ios::binary);out<<"P6\n"<<S<<" "<<S<<"\n255\n";out.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
  std::cout<<items[type]<<": "<<mesh.size()/3<<" triangles\n";
 }
}
