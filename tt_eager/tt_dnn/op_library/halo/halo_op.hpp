// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <tuple>

#include "tensor/tensor.hpp"
#include "run_operation.hpp"
#include "sliding_window_op_infra/sliding_window.hpp"
#include "untilize/untilize_op.hpp"


namespace ttnn::operations {

namespace halo {

struct Halo {
    Tensor operator()(const Tensor& a,
                        const SlidingWindowConfig& config,
                        uint32_t pad_val = 0x0,
                        bool remote_read = false,
                        bool transpose_mcast = true,
                        uint32_t reshard_num_cores_nhw = 0,
                        MemoryConfig output_memory_config = operation::DEFAULT_OUTPUT_MEMORY_CONFIG) const {
        auto halo_op = [&config, pad_val, remote_read, transpose_mcast, reshard_num_cores_nhw, &output_memory_config]
            (const std::vector<Tensor>& input_tensors) -> std::vector<Tensor> {
            const auto& a = input_tensors.at(0);

            auto device = a.device();

            auto pad_metadata = sliding_window::generate_pad_metadata(config);
            auto op_trace_metadata = sliding_window::generate_op_trace_metadata(config);
            auto shard_boundaries = sliding_window::generate_shard_boundaries(config, op_trace_metadata);
            auto tensor_metadata = sliding_window::generate_tensor_metadata(pad_metadata, config, reshard_num_cores_nhw);
            auto kernel_config_tensors = sliding_window::generate_halo_kernel_config_tensors(tensor_metadata, shard_boundaries, remote_read, device);

            const auto& padding_config = std::get<0>(kernel_config_tensors);
            const auto& local_config = std::get<1>(kernel_config_tensors);
            const auto& remote_config = std::get<2>(kernel_config_tensors);
            uint32_t max_out_nsticks_per_core = std::get<3>(kernel_config_tensors);

            return operation::run(
                UntilizeWithHaloV2{
                    .pad_val_ = pad_val,
                    .ncores_nhw_ = config.num_cores_nhw_,
                    .max_out_nsticks_per_core_ = max_out_nsticks_per_core,
                    .out_mem_config_ = output_memory_config,
                    .remote_read_ = remote_read,
                    .transpose_mcast_ = transpose_mcast},
                {
                    a,
                    padding_config,
                    local_config,
                    remote_config
                })
                .at(0);
        };
        std::vector<Tensor> output_tensors = { Tensor(tt::tt_metal::operation::get_workers_for_op_output({a})) };
        operation::launch_op(halo_op, {a}, {output_tensors});

        return output_tensors.at(0);
    }
};

} // namespace halo

} // namespace ttnn::operations