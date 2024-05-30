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

#include "private/filament/DescriptorSets.h"

#include <private/filament/EngineEnums.h>

#include <backend/DriverEnums.h>

#include <utils/CString.h>
#include <utils/debug.h>

#include <string_view>

namespace filament::descriptor_sets {

using namespace backend;

static backend::DescriptorSetLayout const postProcessDescriptorSetLayout{
        {{
                 DescriptorType::UNIFORM_BUFFER,
                 ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,
                 +PerViewBindingPoints::FRAME_UNIFORMS,
                 DescriptorFlags::NONE, 0 },
        }};

static backend::DescriptorSetLayout const depthVariantDescriptorSetLayout{
        {{
                 DescriptorType::UNIFORM_BUFFER,
                 ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,
                 +PerViewBindingPoints::FRAME_UNIFORMS,
                 DescriptorFlags::NONE, 0 },
        }};

static backend::DescriptorSetLayout const ssrVariantDescriptorSetLayout{
        {{
                 DescriptorType::UNIFORM_BUFFER,
                 ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,
                 +PerViewBindingPoints::FRAME_UNIFORMS,
                 DescriptorFlags::NONE, 0 },
         {
                 DescriptorType::SAMPLER,
                 ShaderStageFlags::FRAGMENT,
                 +PerViewBindingPoints::SSR,
                 DescriptorFlags::NONE, 0 },
         {
                 DescriptorType::SAMPLER,
                 ShaderStageFlags::FRAGMENT,
                 +PerViewBindingPoints::STRUCTURE,
                 DescriptorFlags::NONE, 0 },
        }};

static backend::DescriptorSetLayout perViewDescriptorSetLayout = {{
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::FRAME_UNIFORMS, DescriptorFlags::NONE, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::LIGHTS,         DescriptorFlags::NONE, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::SHADOWS,        DescriptorFlags::NONE, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::RECORD_BUFFER,  DescriptorFlags::NONE, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::FROXEL_BUFFER,  DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::SHADOW_MAP,     DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::IBL_DFG_LUT,    DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::IBL_SPECULAR,   DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::SSAO,           DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::SSR,            DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::STRUCTURE,      DescriptorFlags::NONE, 0 },
    { DescriptorType::SAMPLER,                                   ShaderStageFlags::FRAGMENT,  +PerViewBindingPoints::FOG,            DescriptorFlags::NONE, 0 },
}};

static backend::DescriptorSetLayout perRenderableDescriptorSetLayout = {{
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerRenderableBindingPoints::OBJECT_UNIFORMS,             DescriptorFlags::DYNAMIC_OFFSET, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerRenderableBindingPoints::BONES_UNIFORMS,              DescriptorFlags::DYNAMIC_OFFSET, 0 },
    { DescriptorType::UNIFORM_BUFFER, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT,  +PerRenderableBindingPoints::MORPHING_UNIFORMS,           DescriptorFlags::NONE,           0 },
    { DescriptorType::SAMPLER,        ShaderStageFlags::VERTEX                             ,  +PerRenderableBindingPoints::MORPH_TARGET_POSITIONS,      DescriptorFlags::NONE,           0 },
    { DescriptorType::SAMPLER,        ShaderStageFlags::VERTEX                             ,  +PerRenderableBindingPoints::MORPH_TARGET_TANGENTS,       DescriptorFlags::NONE,           0 },
    { DescriptorType::SAMPLER,        ShaderStageFlags::VERTEX                             ,  +PerRenderableBindingPoints::BONES_INDICES_AND_WEIGHTS,   DescriptorFlags::NONE,           0 },
}};

backend::DescriptorSetLayout const& getPostProcessLayout() noexcept {
    return postProcessDescriptorSetLayout;
}

backend::DescriptorSetLayout const& getDepthVariantLayout() noexcept {
    return depthVariantDescriptorSetLayout;
}

backend::DescriptorSetLayout const& getSsrVariantLayout() noexcept {
    return ssrVariantDescriptorSetLayout;
}

backend::DescriptorSetLayout const& getPerViewLayout() noexcept {
    return perViewDescriptorSetLayout;
}

backend::DescriptorSetLayout const& getPerRenderableLayout() noexcept {
    return perRenderableDescriptorSetLayout;
}

utils::CString getDescriptorName(DescriptorSetBindingPoints set,
        backend::descriptor_binding_t binding) noexcept {
    using namespace std::literals;
    constexpr const std::string_view set0[] = {
            "FrameUniforms"sv,
            "LightsUniforms"sv,
            "ShadowUniforms"sv,
            "FroxelRecordUniforms"sv,
            "FroxelsUniforms"sv,
            "sampler0_shadowMap"sv,
            "sampler0_iblDFG"sv,
            "sampler0_iblSpecular"sv,
            "sampler0_ssao"sv,
            "sampler0_ssr"sv,
            "sampler0_structure"sv,
            "sampler0_fog"sv,
    };
    constexpr const std::string_view set1[] = {
            "ObjectUniforms"sv,
            "BonesUniforms"sv,
            "MorphingUniforms"sv,
            "sampler1_positions"sv,
            "sampler1_tangents"sv,
            "sampler1_indicesAndWeights"sv,
    };

    switch (set) {
        case DescriptorSetBindingPoints::PER_VIEW: {
            assert_invariant(binding < perViewDescriptorSetLayout.bindings.size());
            std::string_view const& s = set0[binding];
            return { s.data(), s.size() };
        }
        case DescriptorSetBindingPoints::PER_RENDERABLE: {
            assert_invariant(binding < perRenderableDescriptorSetLayout.bindings.size());
            std::string_view const& s = set1[binding];
            return { s.data(), s.size() };
        }
        case DescriptorSetBindingPoints::PER_MATERIAL: {
            assert_invariant(binding < 1);
            return "MaterialParams";
        }
    }
}

} // namespace filament::descriptor_sets
