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
#include "../module_builder.h"

using namespace nncase;
using namespace nncase::codegen;
using namespace nncase::codegen::stackvm;
using namespace nncase::ir;

void stackvm_module_builder::emit(binary &node, stackvm_op_builder &builder)
{
    builder.lea_buffer(allocation(node.input_a()));
    builder.lea_buffer(allocation(node.input_b()));
    builder.lea_buffer(allocation(node.output()));

    builder.stshape(0, node.input_a().shape());
    builder.stshape(0, node.input_b().shape());
    builder.tensor_binary_(node.input_a().type(), 0, 1, node.binary_op(), node.fused_activation().min, node.fused_activation().max);
}