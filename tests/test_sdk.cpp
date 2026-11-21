#include "chronoflow/chronoflow.h"
#include <cassert>
#include <cmath>
static int calls=0;
static void fps(void *,double) { calls++; }
static void scale(void *,double) { calls++; }
static void gen(void *,int) { calls++; }
int main() {
  assert(cf_api_version()==3);
  cf_telemetry t={60,12,1,1.5,0.8,1,1,0,1,1,1,1};
  cf_policy p{};
  assert(cf_choose_policy(&t,&p)==CF_OK);
  assert(p.real_hz>=30 && p.present_hz>=p.real_hz);
  cf_host_v1 host{nullptr,fps,scale,gen};
  assert(cf_apply_policy(&host,&p)==CF_OK);
  assert(calls==3);
  t.motion_confidence=0.2;
  assert(cf_choose_policy(&t,&p)==CF_OK && !p.enable_generation);
  t.sim_cpu_ms_per_tick=20;
  assert(cf_choose_policy(&t,&p)==CF_OK && !p.simulation_realtime_possible);
  t.display_hz=0;
  assert(cf_choose_policy(&t,&p)==CF_BAD_ARGUMENT);
  cf_timeline clock;
  cf_timeline_result r;
  assert(cf_timeline_init(&clock,60,2)==CF_OK);
  assert(cf_timeline_advance(&clock,1.0/30.0,&r)==CF_OK && r.steps_to_run==2 && r.realtime);
  assert(cf_timeline_advance(&clock,1.0,&r)==CF_OK && r.steps_to_run==2 && !r.realtime && r.remaining_debt_seconds>0.9);
  assert(cf_timeline_advance(&clock,-1,&r)==CF_BAD_ARGUMENT);
}
