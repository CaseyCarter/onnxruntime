#ifndef CORE_PROVIDERS_CPU_NN_POOL_BASE_H
#define CORE_PROVIDERS_CPU_NN_POOL_BASE_H

#include "core/common/common.h"
#include "core/common/logging.h"
#include "core/framework/op_kernel.h"
#include "core/providers/cpu/nn/autopad_type.h"

namespace Lotus {

class PoolBase : public OpKernel {
public:
PoolBase(OpKernelInfo info) : OpKernel(info) {
    const std::string op_name = info.get_kernel_def().OpName();
    global_pooling_ = (op_name == "GlobalAveragePool" || op_name == "GlobalMaxPool" || op_name == "GlobalLpPool");

    if (!global_pooling_) {
      LOTUS_ENFORCE(info.GetAttrs<int64_t>("kernel_shape", kernel_shape_).IsOK(),
                    "No kernel shape is set.");

      std::string auto_padding;
      LOTUS_ENFORCE(info.GetAttr<std::string>("auto_pad", &auto_padding).IsOK());
      auto_pad_ = StringToAutoPadType(auto_padding);

      LOTUS_ENFORCE(info.GetAttrs<int64_t>("pads", pads_).IsOK());
            
      LOTUS_ENFORCE(info.GetAttrs<int64_t>("strides", strides_).IsOK());
      LOTUS_ENFORCE(strides_.size() == kernel_shape_.size());
    }

    // Fill default values. 
    //TODO: remove it when LotusIR populates default values.
    if (kernel_shape_.size() == 0) {
      kernel_shape_.assign({ 0, 0 });
    }

    if (strides_.size() == 0) {
      strides_.resize(kernel_shape_.size(), 1);
    }

    if (pads_.size() == 0) {
      pads_.resize(kernel_shape_.size() * 2, 0);
    }
  }

  virtual ~PoolBase() {}
  
  std::vector<int64_t> SetOutputSize (
    const TensorShape& input_shape,
    int64_t output_channel,
    std::vector<int64_t>* pads) const {
    LOTUS_ENFORCE(input_shape.Size() > 0);
    std::vector<int64_t> output_dims;
    int64_t N = input_shape[0];
    InferOutputSize(
      input_shape.GetDims(),
      &output_dims,
      pads);

    output_dims.insert(output_dims.begin(), { N, output_channel });
    return output_dims;
  }

  inline void InferOutputSize (
    const vector<int64_t>& input_dims,
    vector<int64_t>* output_dims,
    vector<int64_t>* pads) const {
        
    if (global_pooling_) {
      output_dims->assign(input_dims.size()-2, 1);
    }
    else {
      for (int dim = 0; dim < input_dims.size()-2; ++dim) {
        int64_t dim_size = 0;
        ComputeSizeAndPad(
          static_cast<int>(input_dims[dim+2]),
          strides_[dim],
          kernel_shape_[dim],
          &pads->at(dim),
          &pads->at(input_dims.size() + dim -2),
          &dim_size);
        output_dims->push_back(dim_size);
      }
    }
  }

  inline void ComputeSizeAndPad(
    const int64_t in_size,
    const int64_t stride,
    const int64_t kernel,
    int64_t* pad_head,
    int64_t* pad_tail,
    int64_t* out_size) const {

    if (auto_pad_ != AutoPadType::NOTSET) {
      switch (auto_pad_) {
      case AutoPadType::VALID:
        *pad_head = 0;
        *pad_tail = 0;
        *out_size = (in_size - kernel) / stride + 1;
        break;
      case AutoPadType::SAME_UPPER: {
        int64_t legacy_target_size = (in_size + stride - 1) / stride;
        int64_t pad_needed = (legacy_target_size - 1) * stride + kernel - in_size;
        *pad_head = (pad_needed + 1) / 2;
        *pad_tail = pad_needed - *pad_head;
        *out_size = (in_size + pad_needed - kernel) / stride + 1;
        break;
      }
      case AutoPadType::SAME_LOWER: {
        int64_t legacy_target_size = (in_size + stride - 1) / stride;
        int64_t pad_needed = (legacy_target_size - 1) * stride + kernel - in_size;
        *pad_head = pad_needed / 2;
        *pad_tail = pad_needed - *pad_head;
        *out_size = (in_size + pad_needed - kernel) / stride + 1;
        break;
      }
      default: {
        LOTUS_THROW("Unsupported AutoPad Type.");
      }
      }
    }
    else {
      *out_size = static_cast<int64_t>(
        static_cast<float>(in_size + *pad_head + *pad_tail -kernel) / stride + 1);
    }
  }

protected:
  bool global_pooling_;
  std::vector<int64_t> kernel_shape_;
  std::vector<int64_t> pads_;
  std::vector<int64_t> strides_;

  AutoPadType auto_pad_;

  inline int64_t stride_h() const {
    return strides_[0];
  }

  inline int64_t stride_w() const {
    return strides_[1];
  }

};

}
#endif //!CORE_PROVIDERS_CPU_NN_POOL_BASE_H