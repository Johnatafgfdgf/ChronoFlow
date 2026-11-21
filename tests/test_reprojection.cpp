#include "chronoflow/reprojection.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
int main() {
  const unsigned w=9,h=5;
  std::vector<uint8_t> prev(w*h*4,0), curr(w*h*4,0), out(w*h*4,0);
  // White column moves right by two pixels. Correct mid-frame at x=3.
  for(unsigned y=0;y<h;++y) {
    for(unsigned x=0;x<w;++x) {
      const unsigned k=(y*w+x)*4;
      prev[k+3]=curr[k+3]=255;
      if(x==2)for(int c=0;c<3;++c)prev[k+c]=255;
      if(x==4)for(int c=0;c<3;++c)curr[k+c]=255;
    }
  }
  std::vector<float> flow(w*h*2,0),conf(w*h,1);
  for(unsigned i=0;i<w*h;++i) flow[i*2]=-2.f;
  cf_rgba8_image a{prev.data(),w,h,w*4},b{curr.data(),w,h,w*4};
  cf_reprojection_settings s{0.5f,0.7f,4.f};
  assert(cf_interpolate_rgba8(a,b,flow.data(),conf.data(),s,out.data(),w*4)==CF_REPROJECTION_OK);
  for(unsigned y=0;y<h;++y) {
    assert(out[(y*w+3)*4]==255);
    assert(out[(y*w+2)*4]==0);
    assert(out[(y*w+4)*4]==0);
  }
  s.alpha=0.f;
  assert(cf_interpolate_rgba8(a,b,flow.data(),conf.data(),s,out.data(),w*4)==CF_REPROJECTION_OK);
  assert(out==prev);
  s.alpha=1.f;
  assert(cf_interpolate_rgba8(a,b,flow.data(),conf.data(),s,out.data(),w*4)==CF_REPROJECTION_OK);
  assert(out==curr);
  s.alpha=.5f;conf[2*w+3]=0.f;
  assert(cf_interpolate_rgba8(a,b,flow.data(),conf.data(),s,out.data(),w*4)==CF_REPROJECTION_OK);
  assert(out[(2*w+3)*4]==curr[(2*w+3)*4]);
  flow[(2*w+3)*2]=std::numeric_limits<float>::quiet_NaN();
  assert(cf_interpolate_rgba8(a,b,flow.data(),nullptr,s,out.data(),w*4)==CF_REPROJECTION_OK);
  s.alpha=2.f;
  assert(cf_interpolate_rgba8(a,b,flow.data(),nullptr,s,out.data(),w*4)==CF_REPROJECTION_INVALID);
  assert(cf_interpolate_rgba8(a,b,nullptr,nullptr,{.5f,.5f,4.f},out.data(),w*4)==CF_REPROJECTION_INVALID);
  std::puts("ChronoFlow CPU reprojection: all tests passed");
}
