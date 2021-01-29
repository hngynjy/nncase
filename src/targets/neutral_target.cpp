/* Copyright 2020 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <nncase/ir/opcode.h>
#include <nncase/schedule/buffer_allocator.h>
#include <nncase/targets/neutral_target.h>
#include <nncase/transforms/neutral/dequantize_motion.h>
#include <nncase/transforms/neutral/fold_bitcast.h>
#include <nncase/transforms/neutral/fold_constant.h>
#include <nncase/transforms/neutral/fold_conv2d_biasadd.h>
#include <nncase/transforms/neutral/fold_dilated_conv2d.h>
#include <nncase/transforms/neutral/fold_pad.h>
#include <nncase/transforms/neutral/fold_quantize.h>
#include <nncase/transforms/neutral/fold_slice.h>
#include <nncase/transforms/neutral/fold_transpose.h>
#include <nncase/transforms/neutral/fuse_clamp.h>
#include <nncase/transforms/neutral/fuse_pad.h>
#include <nncase/transforms/neutral/fuse_unary.h>
#include <nncase/transforms/neutral/global_reduce_window_to_reduce.h>
#include <nncase/transforms/neutral/quantize_motion.h>
#include <nncase/transforms/neutral/simplify_reduce.h>
#include <nncase/transforms/neutral/take_to_slice.h>
#include <nncase/transforms/neutral/transpose_motion.h>
#include <nncase/transforms/pass.h>

using namespace nncase;
using namespace nncase::targets;
using namespace nncase::schedule;

namespace nncase::codegen
{
void register_neutral_emitters();
}

namespace nncase::ir
{
void register_neutral_evaluators();
}

void neutral_target::register_allocators(allocator_map_t &allocators, std::vector<std::unique_ptr<buffer_allocator>> &allocator_holders)
{
    allocators.emplace(mem_input, allocator_holders.emplace_back(std::make_unique<linear_buffer_allocator>()).get());
    allocators.emplace(mem_output, allocator_holders.emplace_back(std::make_unique<linear_buffer_allocator>()).get());
    allocators.emplace(mem_rdata, allocator_holders.emplace_back(std::make_unique<linear_buffer_allocator>()).get());
    allocators.emplace(mem_data, allocator_holders.emplace_back(std::make_unique<first_fit_allocator>()).get());
}

void neutral_target::register_codegen_ops()
{
    using namespace nncase::codegen;

    register_neutral_emitters();
}

void neutral_target::register_evaluator_ops()
{
    using namespace nncase::ir;

    register_neutral_evaluators();
}

void neutral_target::add_default_transforms(ir::transforms::pass &pass, [[maybe_unused]] bool add_constant_folding)
{
    using namespace nncase::ir;
    using namespace nncase::ir::transforms;

    if (add_constant_folding)
        pass.emplace<fold_constant_transform>();
    pass.emplace<dequantize_transbin_motion_transform>();
    pass.emplace<dequantize_transpose_motion_transform>();
    pass.emplace<dequantize_bitcast_motion_transform>();
    pass.emplace<dequantize_reshape_motion_transform>();
    pass.emplace<dequantize_slice_motion_transform>();
    pass.emplace<dequantize_pad_motion_transform>();
    pass.emplace<quantize_pad_motion_transform>();
    //    pass.emplace<quantize_transbin_motion_transform>();
    pass.emplace<quantize_transpose_motion_transform>();
    pass.emplace<quantize_bitcast_motion_transform>();
    pass.emplace<quantize_reshape_motion_transform>();
    pass.emplace<quantize_slice_motion_transform>();

    pass.emplace<fold_nop_pad_transform>();
    pass.emplace<fold_nop_bitcast_transform>();
    pass.emplace<fold_slice_slice_transform>();
    pass.emplace<fold_pad_pad_transform>();
    pass.emplace<fold_pad_strided_slice_transform>();

    pass.emplace<fold_bitcast_transform>();

    pass.emplace<fuse_clamp_conv2d_transform>();
    pass.emplace<fuse_clamp_binary_transform>();
    pass.emplace<strided_slice_to_pad_transform>();

    pass.emplace<transpose_binary_motion_transform>();
    pass.emplace<transpose_constant_binary_motion_transform>();
    pass.emplace<transpose_concat_motion_transform>();
    pass.emplace<transpose_pad_motion_transform>();
    pass.emplace<transpose_clamp_motion_transform>();

    pass.emplace<fold_conv2d_biasadd_transform>();
    pass.emplace<transpose_reduce_motion_transform>();
    pass.emplace<transpose_unary_motion_transform>();
    pass.emplace<simplify_reduce_transform>();
    pass.emplace<global_reduce_window_to_reduce_transform>();
    pass.emplace<transpose_to_reshape_transform>();
    pass.emplace<take_to_slice_transform>();
    pass.emplace<fold_transpose_transform>();
    pass.emplace<fold_nop_transpose_transform>();
}

void neutral_target::fold_pad_conv_transform(ir::transforms::pass &pass, [[maybe_unused]] bool add_constant_folding)
{
    using namespace nncase::ir;
    using namespace nncase::ir::transforms;
    if (add_constant_folding)
        pass.emplace<fold_constant_transform>();
    pass.emplace<dequantize_pad_motion_transform>();
    pass.emplace<transpose_pad_motion_transform>();
    pass.emplace<fold_transpose_transform>();
    pass.emplace<fold_nop_transpose_transform>();
    pass.emplace<fold_pad_pad_transform>();
    pass.emplace<fuse_pad_conv2d_transform>();
    pass.emplace<fold_nop_pad_transform>();
}

void neutral_target::fold_dilated_conv_transform(ir::transforms::pass &pass, [[maybe_unused]] bool add_constant_folding)
{
    using namespace nncase::ir;
    using namespace nncase::ir::transforms;
    if (add_constant_folding)
        pass.emplace<fold_constant_transform>();
    pass.emplace<transpose_binary_motion_transform>();
    pass.emplace<quantize_transpose_motion_transform>();
    pass.emplace<dequantize_transpose_motion_transform>();
    pass.emplace<fold_transpose_transform>();
    pass.emplace<fold_nop_transpose_transform>();
    pass.emplace<dequantize_s2b_motion_transform>();
    pass.emplace<quantize_b2s_motion_transform>();
    pass.emplace<fold_dilated_conv2d>();
}

void neutral_target::register_target_independent_passes(ir::transforms::pass_manager &pass_mgr)
{
    using namespace nncase::ir;
    using namespace nncase::ir::transforms;

    //fold_pad_conv
    {
        pass p("fold_pad_conv");
        fold_pad_conv_transform(p, true);
        pass_mgr.add_pass(std::move(p));
    }
    //fold_dilated_conv
    {
        pass p("fold_dilated_conv");
        fold_dilated_conv_transform(p, true);
        pass_mgr.add_pass(std::move(p));
    }

    //target_independent_pass
    {
        pass p("target_independent_pass");
        add_default_transforms(p, true);
        pass_mgr.add_pass(std::move(p));
    }
}

void neutral_target::register_target_dependent_passes([[maybe_unused]] ir::transforms::pass_manager &pass_mgr)
{
}

void neutral_target::register_allocation_passes([[maybe_unused]] ir::transforms::pass_manager &pass_mgr)
{
}

std::unique_ptr<target_options> neutral_target::on_create_options()
{
    return std::make_unique<target_options>();
}