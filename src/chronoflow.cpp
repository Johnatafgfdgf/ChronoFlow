#include "chronoflow/chronoflow.h"
#include <cmath>
#include <algorithm>
#include <limits>

extern "C" {
uint32_t cf_api_version(void) { return CF_API_VERSION; }

cf_result cf_choose_policy(const cf_telemetry *t, cf_policy *out) {
  if (!t || !out || !std::isfinite(t->display_hz) || t->display_hz < 15 ||
      !std::isfinite(t->gpu_ms_at_full_resolution) ||
      !std::isfinite(t->gpu_ms_per_generated_frame) ||
      !std::isfinite(t->sim_cpu_ms_per_tick) ||
      !std::isfinite(t->render_cpu_ms_per_real_frame) ||
      !std::isfinite(t->thermal_headroom) ||
      !std::isfinite(t->motion_confidence) ||
      !std::isfinite(t->queued_frames) ||
      t->gpu_ms_at_full_resolution < 0 || t->gpu_ms_per_generated_frame < 0 ||
      t->sim_cpu_ms_per_tick < 0 || t->render_cpu_ms_per_real_frame < 0 ||
      t->queued_frames < 0) return CF_BAD_ARGUMENT;

  const double refresh = std::min(t->display_hz, 120.0);
  const double thermal = std::clamp(t->thermal_headroom, 0.0, 1.0);
  const double budget = 0.53 + 0.33 * thermal; // reserve GPU headroom
  const double cpu_budget = 0.85;
  const double sim_cpu = 60.0 * t->sim_cpu_ms_per_tick / 1000.0;
  const bool cpu_ok = sim_cpu < cpu_budget;
  const bool safe = cpu_ok && thermal >= 0.3 && t->motion_confidence >= 0.68 &&
       t->queued_frames <= 1.0 && t->has_motion_vectors &&
       t->has_depth && t->can_generate_frames;
  const double rates[] = {15., 20., 24., 30., 40., 45., 60., 90., 120.};
  const double scales[] = {1.0, 0.85, 0.70, 0.60};
  double best_score = -std::numeric_limits<double>::infinity();
  cf_policy best{};
  for (double real : rates) {
    if (real > refresh + 0.0001) continue;
    for (double scale : scales) {
      if (!t->can_change_render_scale && scale < 0.999) continue;
      for (int gen=0; gen<2; ++gen) {
        if (gen && (!safe || real < 24 || refresh / real > 2.5 ||
                    refresh - real < 10)) continue;
        const double visual = gen ? refresh : real;
        const double gpu = (real * t->gpu_ms_at_full_resolution * scale * scale +
            (visual - real) * t->gpu_ms_per_generated_frame) / 1000.0;
        const double cpu = (60 * t->sim_cpu_ms_per_tick +
            real * t->render_cpu_ms_per_real_frame) / 1000.0;
        if (gpu > budget || cpu > cpu_budget) continue;
        const double score = 0.55 * real / refresh + 0.35 * visual / refresh +
                             0.10 * scale - (gen ? 0.025 : 0.0);
        if (score > best_score) {
          best_score=score;
          best={real,visual,scale,gpu,cpu,gen,cpu_ok ? 1 : 0,CF_REASON_OK};
        }
      }
    }
  }
  if (!std::isfinite(best_score)) {
    const double scale = t->can_change_render_scale ? 0.60 : 1.0;
    const double gpu = 15.0 * t->gpu_ms_at_full_resolution * scale * scale / 1000.;
    const double cpu = (60.0 * t->sim_cpu_ms_per_tick +
                          15.0 * t->render_cpu_ms_per_real_frame) / 1000.;
    best={15.,15.,scale,gpu,cpu,0,cpu_ok ? 1 : 0,CF_REASON_GPU_LIMIT};
  }
  if (!cpu_ok) best.reason=CF_REASON_CPU_LIMIT;
  else if (thermal < 0.3) best.reason=CF_REASON_THERMAL;
  else if (t->queued_frames > 1.0) best.reason=CF_REASON_QUEUE;
  else if (t->motion_confidence < 0.68) best.reason=CF_REASON_UNCERTAIN;
  else if (best.real_hz < refresh) best.reason=CF_REASON_GPU_LIMIT;
  *out=best;
  return CF_OK;
}

cf_result cf_apply_policy(const cf_host_v1 *host, const cf_policy *p) {
  if (!host || !p || !host->set_real_fps_target ||
      !std::isfinite(p->real_hz) || p->real_hz <= 0 ||
      !std::isfinite(p->render_scale) || p->render_scale <= 0 ||
      !std::isfinite(p->present_hz) || p->present_hz < p->real_hz)
    return CF_BAD_ARGUMENT;
  /* Avoid falsely claiming to enable unavailable features. */
  if (p->render_scale < 0.999 && !host->set_render_scale) return CF_UNSUPPORTED;
  if (p->enable_generation && !host->enable_frame_generation) return CF_UNSUPPORTED;
  host->set_real_fps_target(host->user, p->real_hz);
  if (host->set_render_scale) host->set_render_scale(host->user,p->render_scale);
  if (host->enable_frame_generation)
    host->enable_frame_generation(host->user,p->enable_generation ? 1 : 0);
  return CF_OK;
}

cf_result cf_timeline_init(cf_timeline *clock,double hz,int max_catchup) {
  if (!clock || !std::isfinite(hz) || hz <= 0 || hz > 1000 ||
      max_catchup < 1 || max_catchup > 10000) return CF_BAD_ARGUMENT;
  *clock={0.,1.0/hz,max_catchup};
  return CF_OK;
}

cf_result cf_timeline_advance(cf_timeline *c,double delta,cf_timeline_result *out) {
  if (!c || !out || !std::isfinite(delta) || delta < 0 ||
      !std::isfinite(c->accumulated_seconds) || c->accumulated_seconds < 0 ||
      !std::isfinite(c->fixed_dt_seconds) || c->fixed_dt_seconds <= 0 ||
      c->max_steps_per_frame < 1) return CF_BAD_ARGUMENT;
  c->accumulated_seconds += delta;
  int steps=0;
  while (steps < c->max_steps_per_frame &&
         c->accumulated_seconds + 1e-10 >= c->fixed_dt_seconds) {
    c->accumulated_seconds -= c->fixed_dt_seconds;
    ++steps;
  }
  out->steps_to_run=steps;
  out->remaining_debt_seconds=std::max(0.,c->accumulated_seconds);
  out->realtime=(out->remaining_debt_seconds < c->fixed_dt_seconds + 1e-10) ? 1:0;
  return CF_OK;
}
}
