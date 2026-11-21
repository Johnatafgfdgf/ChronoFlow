#ifndef CHRONOFLOW_H
#define CHRONOFLOW_H
/* ChronoFlow portable host SDK v0.3: planning & timing, NOT a GPU frame generator.
 * C99 ABI (usable from C++ and Android NDK). */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CF_API_VERSION 3
typedef enum {
  CF_OK=0, CF_BAD_ARGUMENT=1, CF_UNSUPPORTED=2
} cf_result;
typedef enum {
  CF_REASON_OK=0, CF_REASON_CPU_LIMIT=1, CF_REASON_GPU_LIMIT=2,
  CF_REASON_THERMAL=3, CF_REASON_UNCERTAIN=4, CF_REASON_QUEUE=5
} cf_reason;
typedef struct {
  /* Costs MUST be measured by the integrating host in milliseconds. */
  double display_hz;
  double gpu_ms_at_full_resolution;
  double gpu_ms_per_generated_frame;
  double sim_cpu_ms_per_tick;     /* 60 Hz simulation reference */
  double render_cpu_ms_per_real_frame;
  double thermal_headroom;       /* 0..1 estimate; host maps ADPF if available */
  double motion_confidence;      /* 0..1 estimate; low => no generation */
  double queued_frames;
  int has_motion_vectors;
  int has_depth;
  int can_change_render_scale;
  int can_generate_frames;       /* host supplies actual GPU implementation */
} cf_telemetry;
typedef struct {
  double real_hz;                /* suggested REAL render target, not achieved FPS */
  double present_hz;             /* desired presentation cadence */
  double render_scale;           /* linear resolution, only when host supports it */
  double estimated_gpu_util;
  double estimated_cpu_util;
  int enable_generation;
  int simulation_realtime_possible; /* capacity estimate, not a guarantee */
  cf_reason reason;
} cf_policy;
typedef struct {
  void *user;
  void (*set_real_fps_target)(void *, double);
  void (*set_render_scale)(void *, double);
  void (*enable_frame_generation)(void *, int);
} cf_host_v1;
typedef struct {
  double accumulated_seconds;
  double fixed_dt_seconds;
  int max_steps_per_frame;
} cf_timeline;
typedef struct {
  int steps_to_run;
  double remaining_debt_seconds;
  int realtime; /* 0 means simulation is behind: never silently slow time */
} cf_timeline_result;
uint32_t cf_api_version(void);
cf_result cf_choose_policy(const cf_telemetry *telemetry, cf_policy *out_policy);
cf_result cf_apply_policy(const cf_host_v1 *host, const cf_policy *policy);
cf_result cf_timeline_init(cf_timeline *timeline, double tick_hz, int max_catchup);
cf_result cf_timeline_advance(cf_timeline *timeline, double elapsed_seconds, cf_timeline_result *out);
#ifdef __cplusplus
}
#endif
#endif
