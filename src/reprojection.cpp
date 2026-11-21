#include "chronoflow/reprojection.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {
inline bool valid_image(cf_rgba8_image i) {
  return i.data && i.width && i.height &&
     uint64_t(i.width)*4 <= i.stride_bytes &&
     uint64_t(i.height)*i.stride_bytes <= SIZE_MAX;
}
inline uint8_t to_byte(float x) {
  return static_cast<uint8_t>(std::clamp(std::lround(x),0L,255L));
}
void sample(cf_rgba8_image image, float x, float y, float color[4]) {
  // Pixel centers use integer coordinates (0,0), (1,0), etc.
  x=std::clamp(x,0.f,float(image.width-1));
  y=std::clamp(y,0.f,float(image.height-1));
  uint32_t x0=static_cast<uint32_t>(x), y0=static_cast<uint32_t>(y);
  uint32_t x1=std::min(x0+1,image.width-1), y1=std::min(y0+1,image.height-1);
  float u=x-float(x0), v=y-float(y0);
  const uint8_t *a=image.data+size_t(y0)*image.stride_bytes+size_t(x0)*4;
  const uint8_t *b=image.data+size_t(y0)*image.stride_bytes+size_t(x1)*4;
  const uint8_t *c=image.data+size_t(y1)*image.stride_bytes+size_t(x0)*4;
  const uint8_t *d=image.data+size_t(y1)*image.stride_bytes+size_t(x1)*4;
  for(int ch=0;ch<4;++ch)
    color[ch]=(1-v)*((1-u)*a[ch]+u*b[ch])+v*((1-u)*c[ch]+u*d[ch]);
}
}
extern "C" cf_reprojection_result cf_interpolate_rgba8(
   cf_rgba8_image prev,cf_rgba8_image curr,const float*flow,const float*confidence,
   cf_reprojection_settings s,uint8_t*out,uint32_t out_stride) {
  if(!valid_image(prev) || !valid_image(curr) || !flow || !out ||
     prev.width!=curr.width || prev.height!=curr.height ||
     uint64_t(prev.width)*4>out_stride ||
     uint64_t(prev.height)*out_stride>SIZE_MAX ||
     uint64_t(prev.width)*prev.height>SIZE_MAX/(sizeof(float)*2) ||
     !std::isfinite(s.alpha) || s.alpha<0 || s.alpha>1 ||
     !std::isfinite(s.min_confidence) || s.min_confidence<0 || s.min_confidence>1 ||
     !std::isfinite(s.max_flow_pixels) || s.max_flow_pixels<=0)
    return CF_REPROJECTION_INVALID;

  for(uint32_t y=0;y<curr.height;++y) for(uint32_t x=0;x<curr.width;++x) {
    size_t index=size_t(y)*curr.width+x;
    float dx=flow[index*2],dy=flow[index*2+1];
    const uint8_t *fallback=curr.data+size_t(y)*curr.stride_bytes+size_t(x)*4;
    uint8_t *dst=out+size_t(y)*out_stride+size_t(x)*4;
    if(s.alpha==0.f) {
      const uint8_t *src=prev.data+size_t(y)*prev.stride_bytes+size_t(x)*4;
      for(int i=0;i<4;++i)dst[i]=src[i];
      continue;
    }
    if(s.alpha==1.f) {
      for(int i=0;i<4;++i)dst[i]=fallback[i];
      continue;
    }
    bool good=std::isfinite(dx)&&std::isfinite(dy) &&
      std::hypot(dx,dy)<=s.max_flow_pixels &&
      (!confidence || (std::isfinite(confidence[index]) && confidence[index]>=s.min_confidence));
    float px=float(x)+s.alpha*dx, py=float(y)+s.alpha*dy;
    float cx=float(x)-(1.f-s.alpha)*dx, cy=float(y)-(1.f-s.alpha)*dy;
    good=good && px>=0 && py>=0 && cx>=0 && cy>=0 &&
      px<=prev.width-1 && cx<=curr.width-1 &&
      py<=prev.height-1 && cy<=curr.height-1;
    if(!good) {for(int i=0;i<4;++i)dst[i]=fallback[i];continue;}
    float p[4],c[4];sample(prev,px,py,p);sample(curr,cx,cy,c);
    for(int i=0;i<4;++i)dst[i]=to_byte((1.f-s.alpha)*p[i]+s.alpha*c[i]);
  }
  return CF_REPROJECTION_OK;
}
