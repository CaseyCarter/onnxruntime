#pragma once

#include <memory>

#include "core/framework/execution_provider.h"

namespace onnxruntime {

// Information needed to construct CPU/MKL-DNN execution providers.
struct CPUExecutionProviderInfo {
  std::string name;
  bool create_arena{true};

  explicit CPUExecutionProviderInfo(const std::string& provider_name,
                                    bool use_arena = true)
      : name(provider_name), create_arena(use_arena) {}
  CPUExecutionProviderInfo() = default;
};

// Create CPU execution provider
std::unique_ptr<IExecutionProvider>
CreateBasicCPUExecutionProvider(const CPUExecutionProviderInfo& info);

// Create MKL-DNN execution provider
std::unique_ptr<IExecutionProvider>
CreateMKLDNNExecutionProvider(const CPUExecutionProviderInfo& info);

// Information needed to construct CUDA execution providers.
struct CUDAExecutionProviderInfo {
  std::string name;
  int device_id{0};
};

// Create cuda execution provider
std::unique_ptr<IExecutionProvider>
CreateCUDAExecutionProvider(const CUDAExecutionProviderInfo& info);

// Information needed to construct Nuphar execution providers.
struct NupharExecutionProviderInfo {
  std::string name;
  int device_id{0};
  // By default, construct "stackvm" TVM target, for which the default device_type is kDLCPU.
  std::string target_str{"stackvm"};

  explicit NupharExecutionProviderInfo(const std::string& provider_name,
                                    int dev_id = 0,
                                    const std::string& tgt_str = "stackvm")
      : name(provider_name), device_id(dev_id), target_str(tgt_str) {}
  NupharExecutionProviderInfo() = default;
};

// Create tvm execution provider
std::unique_ptr<IExecutionProvider>
CreateNupharExecutionProvider(const NupharExecutionProviderInfo& info);

}  // namespace onnxruntime