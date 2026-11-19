#include "../../sys/android/app/src/main/cpp/FpvDrone.h"
#include <cassert>
#include <iostream>
int main(){
 physics::CollisionMesh city,built;city.addBox({-20,-.2f,-20},{20,0,20});city.addBox({1,0,-10},{1.1f,6,10});city.build();built.build();
 sandbox::FpvDrone d;
 assert(d.take({0,1.7f,0},0,city,built));assert(d.mode==d.InHands);
 assert(d.place(city,built));assert(d.mode==d.Ground);
 auto body=physics::Vec3{0,1.7f,0};float pitch=std::atan2(d.position.y-body.y,.75f);
 assert(d.connect(body,0,pitch,city,built));assert(d.piloting());
 auto start=d.camera();assert(physics::length(start-body)<.001f);
 auto feed=d.feedEye();assert(physics::length(feed-body)>.5f);
 d.update(.01f,0,0,0,0,0,city,built);
 assert(physics::length(d.camera()-body)<1e-6f); // first displayed frame is player camera
 float previousDistance=0;
 for(int i=0;i<80;i++){
  d.update(.01f,0,0,0,0,0,city,built);
  float distance=physics::length(d.camera()-body);assert(distance>=previousDistance-1e-6f);previousDistance=distance;
 }
 assert(d.mode==d.Entering);assert(physics::length(d.camera()-(body+(feed-body)*.5f))<.001f);
 assert(physics::length(d.feedEye()-feed)<1e-6f); // monitor is never the interpolated player view
 for(int i=0;i<90;i++)d.update(.01f,0,0,0,0,0,city,built);
 assert(physics::length(d.camera()-d.feedEye())<1e-6f);
 assert(d.mode==d.Flying);assert(physics::length(d.bodyEye-body)<.001f);
 for(int i=0;i<200;i++)d.update(.01f,1,0,1,0,0,city,built);
 assert(d.position.x<.79f);assert(d.position.y>2);
 d.leave();assert(d.mode==d.Leaving);
 for(int i=0;i<200;i++)d.update(.01f,0,0,0,0,0,city,built);
 assert(d.mode==d.Hover&&!d.piloting());
 float hoverHeight=d.position.y;
 for(int i=0;i<500;i++)d.update(.01f,0,0,0,0,0,city,built);
 assert(std::abs(d.position.y-hoverHeight)<.01f);
 assert(d.connect({-2,1.7f,0},0,0,city,built));
 d.leave();for(int i=0;i<200;i++)d.update(.01f,0,0,0,0,0,city,built);
 assert(d.returnHome({0,1.7f,0},0,city,built));
 for(int i=0;i<2500;i++)d.update(.01f,0,0,0,0,0,city,built);
 assert(d.mode==d.Hover&&physics::length(d.position-physics::Vec3{0,1.25f,-.9f})<.1f);
 assert(!d.take({1.05f,1.7f,0},0,city,built));

 // Free camera look must not change flight attitude or position.
 sandbox::FpvDrone viewer;viewer.mode=viewer.Flying;viewer.position={0,3,0};auto eyeBefore=viewer.feedEye();
 viewer.look(100,-100);assert(viewer.feedPitch()>.3f&&viewer.feedYaw()==viewer.yaw);
 assert(viewer.pitch==0&&viewer.yaw==0&&physics::length(viewer.feedEye()-eyeBefore)<1e-6f);
 viewer.look(0,10000);assert(viewer.feedPitch()>=-1.3f);
 viewer.mode=viewer.Hover;float cameraPitch=viewer.feedPitch();viewer.look(0,100);assert(viewer.feedPitch()==cameraPitch);
 // Orbit is independent of aircraft heading; view switches interpolate.
 viewer.mode=viewer.Flying;viewer.position={-5,5,-5};viewer.yaw=0;
 auto fpvEye=viewer.feedEye();viewer.toggleView(city,built);
 assert(viewer.thirdPerson&&physics::length(viewer.camera()-fpvEye)<1e-6f);
 for(int i=0;i<30;i++){viewer.update(.01f,0,0,0,0,0,city,built);viewer.updateChase(city,built);}
 assert(viewer.viewWeight()>.49f&&viewer.viewWeight()<.51f);
 for(int i=0;i<40;i++){viewer.update(.01f,0,0,0,0,0,city,built);viewer.updateChase(city,built);}
 assert(viewer.camera().z>viewer.position.z+2.4f&&viewer.viewRoll()==0);
 auto physicalPosition=viewer.position;viewer.look(-392.699f,0);viewer.updateChase(city,built);
 assert(viewer.camera().x>viewer.position.x+2.4f&&viewer.yaw==0);
 assert(physics::length(viewer.position-physicalPosition)<1e-6f);
 viewer.look(0,-250);viewer.updateChase(city,built);assert(viewer.camera().y>viewer.position.y+2);
 viewer.look(0,1000);viewer.updateChase(city,built);assert(viewer.camera().y<viewer.position.y-2);
 viewer.orbitYaw=0;viewer.orbitElevation=.35f;
 physics::CollisionMesh cameraWall;cameraWall.addBox({-8,0,-4},{-2,9,-3.9f});cameraWall.build();
 viewer.updateChase(cameraWall,built);assert(viewer.camera().z< -4.1f);
 assert(!cameraWall.intersectsBox(viewer.camera(),{.12f,.12f,.12f}));
 auto beforeSwitch=viewer.camera();viewer.toggleView(city,built);
 assert(physics::length(viewer.camera()-beforeSwitch)<.001f);
 for(int i=0;i<70;i++)viewer.update(.01f,0,0,0,0,0,city,built);
 assert(!viewer.thirdPerson&&physics::length(viewer.camera()-viewer.feedEye())<1e-6f);
 // Identical joystick inputs produce identical flight in either view.
 sandbox::FpvDrone first,orbit;first.mode=first.Flying;first.position={-5,5,-5};orbit=first;orbit.toggleView(city,built);orbit.look(300,-100);
 for(int i=0;i<100;i++){first.update(.01f,.4f,.5f,.2f,.3f,0,city,built);orbit.update(.01f,.4f,.5f,.2f,.3f,0,city,built);orbit.updateChase(city,built);}
 assert(physics::length(first.position-orbit.position)<1e-6f&&first.yaw==orbit.yaw&&first.pitch==orbit.pitch&&first.roll==orbit.roll);
 // The two sticks have independent axes: yaw/throttle never become strafe.
 sandbox::FpvDrone radio;radio.mode=radio.Flying;radio.position={-5,3,-5};
 for(int i=0;i<100;i++)radio.update(.01f,0,0,1,1,0,city,built);
 assert(radio.position.y>4&&radio.yaw< -1&&std::abs(radio.position.x+5)<.01f&&std::abs(radio.position.z+5)<.01f);
 radio.position={-5,3,-5};radio.velocity={};radio.yaw=0;
 for(int i=0;i<100;i++)radio.update(.01f,0,1,0,0,0,city,built);
 assert(radio.pitch<-.3f&&radio.position.z< -5.5f);
 radio.position={-5,3,-5};radio.velocity={};radio.pitch=0;
 for(int i=0;i<100;i++)radio.update(.01f,1,0,0,0,0,city,built);
 assert(radio.roll>.3f&&radio.position.x> -4.5f);
 radio.bodyEye={-5,1.7f,-5};radio.leave();auto exitPosition=radio.position;
 for(int i=0;i<600;i++)radio.update(.01f,0,0,0,0,0,city,built);
 assert(radio.mode==radio.Hover&&physics::length(radio.position-exitPosition)<.01f);
 // Enclosed return must stop safely, not tunnel through a roof.
 physics::CollisionMesh enclosure;enclosure.addBox({-6,3.3f,-6},{-3,3.5f,-3});enclosure.build();
 radio.position={-5,3,-5};radio.pitch=radio.roll=0;
 assert(radio.returnHome({-4,1.7f,-5},0,city,enclosure));
 for(int i=0;i<2000;i++)radio.update(.01f,0,0,0,0,0,city,enclosure);
 assert(radio.mode==radio.Hover&&!radio.blocked(radio.position,city,enclosure));
 std::cout<<"Drone hand/ground spawn, aim connect, camera transition, locked body, swept wall flight, two-stage exit, linked reconnection and return-to-home hover OK\n";
}
