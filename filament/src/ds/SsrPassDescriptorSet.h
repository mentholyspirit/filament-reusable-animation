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

#ifndef TNT_FILAMENT_SSRPASSDESCRIPTORSET_H
#define TNT_FILAMENT_SSRPASSDESCRIPTORSET_H

#include "DescriptorSet.h"

#include "TypedUniformBuffer.h"

#include <private/filament/UibStructs.h>

#include <backend/Handle.h>

#include <math/mat4.h>

namespace filament {

class DescriptorSetLayout;
class FEngine;

struct ScreenSpaceReflectionsOptions;

class SsrPassDescriptorSet {

    using TextureHandle = backend::Handle<backend::HwTexture>;

public:
    SsrPassDescriptorSet(FEngine& engine,
            TypedUniformBuffer<PerViewUib>& uniforms) noexcept;

    void terminate(backend::DriverApi& driver);

    void prepareStructure(TextureHandle structure) noexcept;

    void prepareHistorySSR(TextureHandle ssr,
            math::mat4f const& historyProjection,
            math::mat4f const& uvFromViewMatrix,
            ScreenSpaceReflectionsOptions const& ssrOptions) noexcept;


    // update local data into GPU UBO
    void commit(backend::DriverApi& driver) noexcept;

    // bind this descriptor set
    void bind(backend::DriverApi& driver) noexcept;

private:
    DescriptorSetLayout const& mDescriptorSetLayout;
    TypedUniformBuffer<PerViewUib>& mUniforms;
    DescriptorSet mDescriptorSet;
};

} // namespace filament

#endif //TNT_FILAMENT_SSRPASSDESCRIPTORSET_H
