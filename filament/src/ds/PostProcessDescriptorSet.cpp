/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PostProcessDescriptorSet.h"

#include "TypedUniformBuffer.h"

#include "details/Engine.h"

#include <private/filament/EngineEnums.h>
#include <private/filament/UibStructs.h>

#include <backend/DriverEnums.h>

#include <memory>

namespace filament {

using namespace backend;
using namespace math;

PostProcessDescriptorSet::PostProcessDescriptorSet() noexcept = default;

void PostProcessDescriptorSet::init(FEngine& engine) noexcept {
    // set-up the descriptor-set layout information
    backend::DescriptorSetLayout const descriptorSetLayout{
            {{
                     DescriptorType::UNIFORM_BUFFER,
                     ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,
                     +PerViewBindingPoints::FRAME_UNIFORMS,
                     DescriptorFlags::NONE, 0 },
            }};

    // create the descriptor-set layout
    mDescriptorSetLayout = filament::DescriptorSetLayout{
            engine.getDriverApi(), descriptorSetLayout };

    // create the descriptor-set from the layout
    mDescriptorSet = DescriptorSet{ mDescriptorSetLayout };
}

void PostProcessDescriptorSet::terminate(DriverApi& driver) {
    mDescriptorSet.terminate(driver);
}

void PostProcessDescriptorSet::setFrameUniforms(DriverApi& driver,
        TypedUniformBuffer<PerViewUib>& uniforms) noexcept {
    // initialize the descriptor-set
    mDescriptorSet.setBuffer(+PerViewBindingPoints::FRAME_UNIFORMS,
            uniforms.getUboHandle(), 0, uniforms.getSize());

    mDescriptorSet.commit(mDescriptorSetLayout, driver);
}

void PostProcessDescriptorSet::bind(backend::DriverApi& driver) noexcept {
    mDescriptorSet.bind(driver, DescriptorSetBindingPoints::PER_VIEW);
}

} // namespace filament

