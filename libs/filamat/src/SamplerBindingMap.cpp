/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "SamplerBindingMap.h"

#include "shaders/SibGenerator.h"

#include <filament/MaterialEnums.h>

#include <private/filament/EngineEnums.h>
#include <private/filament/SamplerInterfaceBlock.h>

#include <backend/DriverEnums.h>

#include <utils/BitmaskEnum.h>
#include <utils/Panic.h>
#include <utils/debug.h>

#include <stddef.h>

namespace filament {

using namespace utils;
using namespace backend;

void SamplerBindingMap::init(MaterialDomain materialDomain,
        SamplerInterfaceBlock const& perMaterialSib) {

    // Note: the material variant affects only the sampler types, but cannot affect
    // the actual bindings. For this reason it is okay to use the dummyVariant here.
    size_t vertexSamplerCount = 0;
    size_t fragmentSamplerCount = 0;

    auto processSamplerGroup = [&](DescriptorSetBindingPoints set){
        SamplerInterfaceBlock const* const sib =
                (set == DescriptorSetBindingPoints::PER_MATERIAL) ?
                &perMaterialSib : SibGenerator::getSib(set, {});
        if (sib) {
            const auto stageFlags = sib->getStageFlags();
            auto const& list = sib->getSamplerInfoList();
            const size_t samplerCount = list.size();

            if (any(stageFlags & ShaderStageFlags::VERTEX)) {
                vertexSamplerCount += samplerCount;
            }
            if (any(stageFlags & ShaderStageFlags::FRAGMENT)) {
                fragmentSamplerCount += samplerCount;
            }

            for (size_t i = 0; i < samplerCount; i++) {
                assert_invariant(
                        mSamplerNamesBindingMap.find({ +set, list[i].binding }) == mSamplerNamesBindingMap.end());
                mSamplerNamesBindingMap[{ +set, list[i].binding }] = list[i].uniformName;
            }
        }
    };

    switch(materialDomain) {
        case MaterialDomain::SURFACE:
            processSamplerGroup(DescriptorSetBindingPoints::PER_VIEW);
            processSamplerGroup(DescriptorSetBindingPoints::PER_RENDERABLE);
            processSamplerGroup(DescriptorSetBindingPoints::PER_MATERIAL);
            break;
        case MaterialDomain::POST_PROCESS:
        case MaterialDomain::COMPUTE:
            processSamplerGroup(DescriptorSetBindingPoints::PER_MATERIAL);
            break;
    }

    constexpr size_t MAX_VERTEX_SAMPLER_COUNT =
            backend::FEATURE_LEVEL_CAPS[+FeatureLevel::FEATURE_LEVEL_3].MAX_VERTEX_SAMPLER_COUNT;

    constexpr size_t MAX_FRAGMENT_SAMPLER_COUNT =
            backend::FEATURE_LEVEL_CAPS[+FeatureLevel::FEATURE_LEVEL_3].MAX_FRAGMENT_SAMPLER_COUNT;

    // we shouldn't be using more total samplers than supported
    ASSERT_PRECONDITION(vertexSamplerCount + fragmentSamplerCount <= MAX_SAMPLER_COUNT,
            "material using too many samplers");

    // Here we cannot check for overflow for a given feature level because we don't know
    // what feature level the backend will support. We only know the feature level declared
    // by the material. However, we can at least assert for the highest feature level.

    ASSERT_PRECONDITION(vertexSamplerCount <= MAX_VERTEX_SAMPLER_COUNT,
            "material using too many samplers in vertex shader");

    ASSERT_PRECONDITION(fragmentSamplerCount <= MAX_FRAGMENT_SAMPLER_COUNT,
            "material using too many samplers in fragment shader");
}

} // namespace filament
