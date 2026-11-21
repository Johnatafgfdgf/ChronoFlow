#include "chronoflow/chronoflow.h"
#include <stdio.h>
static void set_fps(void *ctx,double fps) { (void)ctx; printf("real FPS target = %.0f\n",fps); }
static void set_scale(void *ctx,double s) { (void)ctx; printf("resolution scale = %.2f\n",s); }
static void enable_fg(void *ctx,int enabled) { (void)ctx; printf("host framegen = %s\n",enabled?"enabled":"disabled"); }
int main(void) {
  /* Replace these example values with actual host measurements. */
  cf_telemetry t={60,21.0,1.2,1.6,1.0,0.8,0.9,0,1,1,1,1};
  cf_policy policy={0};
  if(cf_choose_policy(&t,&policy)!=CF_OK) return 1;
  cf_host_v1 host={0,set_fps,set_scale,enable_fg};
  if(cf_apply_policy(&host,&policy)!=CF_OK) return 2;
  cf_timeline clock;
  cf_timeline_result res;
  if(cf_timeline_init(&clock,60,8)!=CF_OK) return 3;
  if(cf_timeline_advance(&clock,1.0/30.0,&res)!=CF_OK) return 4;
  printf("simulation ticks = %d, debt = %.6f s\n",res.steps_to_run,res.remaining_debt_seconds);
  return res.steps_to_run==2?0:5;
}
