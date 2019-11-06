// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <vector>

#include <xnnpack.h>

#include <benchmark/benchmark.h>

#include "bench/utils.h"
#include "models/models.h"
#include <xnnpack/dwconv.h>
#include <xnnpack/params.h>


static void DWConvEnd2EndBenchmark(
  benchmark::State& state,
  models::ExecutionPlanFactory model_factory, 
  xnn_f32_dwconv_up_ukernel_function dwconv,
  uint8_t cr, uint8_t mr)
{
  if (xnn_initialize() != xnn_status_success) {
    state.SkipWithError("failed to initialize XNNPACK");
    return;
  }

  // Override microkernels chosen in xnn_initialize
  for (size_t i = 0; i < XNN_MAX_F32_DWCONV_UKERNELS; i++) {
    // Replace only the microkernel the matching kernel size.
    if (xnn_params.f32.dwconv[i].mr == mr) {
      xnn_params.f32.dwconv[i] = (struct dwconv_parameters) {
        .up = (xnn_dwconv_up_ukernel_function) dwconv,
        .cr = cr,
        .mr = mr,
      };
      break;
    }
  }

  auto execution_plan = model_factory(nullptr);
  if (execution_plan.empty()) {
    state.SkipWithError("failed to create a model");
    return;
  }

  for (auto _ : state) {
    for (const std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)>& op : execution_plan) {
      xnn_status status = xnn_run_operator(op.get(), nullptr);
      if (status != xnn_status_success) {
        state.SkipWithError("failed to run a model");
        return;
      }
    }
  }
  state.counters["Freq"] = benchmark::utils::GetCurrentCpuFrequency();
}

DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up1x25__scalar)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up1x4__scalar)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up1x9__scalar)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x25__psimd)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x25__sse)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x4__psimd)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x4__sse)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__aarch64_neonfma)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__aarch64_neonfma_cortex_a55)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__neon)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__neonfma)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__psimd)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up4x9__sse)
DECLARE_F32_DWCONV_UNIPASS_UKERNEL_FUNCTION(xnn_f32_dwconv_ukernel_up8x9__neonfma)

#if XNN_ARCH_ARM64 && XNN_ENABLE_ASSEMBLY
  static void f32_dwconv_up4x9__aarch64_neonfma(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__aarch64_neonfma,
      4 /* cr */, 9 /* mr */);
  }

  static void f32_dwconv_up4x9__aarch64_neonfma_cortex_a55(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__aarch64_neonfma_cortex_a55,
      4 /* cr */, 9 /* mr */);
  }

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__aarch64_neonfma, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__aarch64_neonfma, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__aarch64_neonfma_cortex_a55, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__aarch64_neonfma_cortex_a55, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();
#endif  // XNN_ARCH_ARM64 && XNN_ENABLE_ASSEMBLY

#if XNN_ARCH_ARM || XNN_ARCH_ARM64
  static void f32_dwconv_up4x9__neon(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__neon,
      4 /* cr */, 9 /* mr */);
  }

  static void f32_dwconv_up4x9__neonfma(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__neonfma,
      4 /* cr */, 9 /* mr */);
  }

  static void f32_dwconv_up8x9__neonfma(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up8x9__neonfma,
      8 /* cr */, 9 /* mr */);
  }

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__neon, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__neon, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__neonfma, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__neonfma, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();

  BENCHMARK_CAPTURE(f32_dwconv_up8x9__neonfma, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up8x9__neonfma, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();
#endif  // XNN_ARCH_ARM || XNN_ARCH_ARM64


#if XNN_ARCH_X86 || XNN_ARCH_X86_64
  static void f32_dwconv_up4x9__sse(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__sse,
      4 /* cr */, 9 /* mr */);
  }

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__sse, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__sse, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();
#endif  // XNN_ARCH_X86 || XNN_ARCH_X86_64

#if !XNN_ARCH_WASM && !XNN_ARCH_ASMJS
  static void f32_dwconv_up4x9__psimd(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_f32_dwconv_ukernel_up4x9__psimd,
      4 /* cr */, 9 /* mr */);
  }

  BENCHMARK_CAPTURE(f32_dwconv_up4x9__psimd, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
  BENCHMARK_CAPTURE(f32_dwconv_up4x9__psimd, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();
#endif  // !XNN_ARCH_WASM && !XNN_ARCH_ASMJS

static void f32_dwconv_up1x9__scalar(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_f32_dwconv_ukernel_up1x9__scalar,
      1 /* cr */, 9 /* mr */);
}

BENCHMARK_CAPTURE(f32_dwconv_up1x9__scalar, mobilenet_v1, models::MobileNetV1)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK_CAPTURE(f32_dwconv_up1x9__scalar, mobilenet_v2, models::MobileNetV2)->Unit(benchmark::kMicrosecond)->UseRealTime();

#ifndef XNNPACK_BENCHMARK_NO_MAIN
BENCHMARK_MAIN();
#endif