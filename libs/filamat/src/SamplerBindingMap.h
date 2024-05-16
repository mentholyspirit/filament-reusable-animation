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

#ifndef TNT_FILAMENT_DRIVER_SAMPLERBINDINGMAP_H
#define TNT_FILAMENT_DRIVER_SAMPLERBINDINGMAP_H

#include <filament/MaterialEnums.h>

#include <backend/DriverEnums.h>

#include <utils/CString.h>

#include <algorithm>
#include <optional>
#include <utility>
#include <unordered_map>

#include <stddef.h>
#include <stdint.h>

namespace filament {

class SamplerInterfaceBlock;

/*
 * SamplerBindingMap maps filament's (BindingPoints, offset) to a global offset.
 * This global offset is used in shaders to set the `layout(binding=` of each sampler.
 *
 * It also keeps a map of global offsets to the sampler name in the shader.
 *
 * SamplerBindingMap is flattened into the material file and used on the filament side to
 * create the backend's programs.
 */
class SamplerBindingMap {
public:

    struct pair_hash {
        template<class T1, class T2>
        size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };

    // map of sampler shader binding to sampler shader name
    using SamplerBindingToNameMap = std::unordered_map<
            std::pair<backend::descriptor_set_t, backend::descriptor_binding_t>, utils::CString, pair_hash>;

    // Initializes the SamplerBindingMap.
    // Assigns a range of finalized binding points to each sampler block.
    // If a per-material SIB is provided, then material samplers are also inserted (always at the
    // end).
    void init(MaterialDomain materialDomain,
            SamplerInterfaceBlock const& perMaterialSib);

    std::optional<utils::CString> getSamplerName(
            backend::descriptor_set_t set, backend::descriptor_binding_t binding) const noexcept {
        std::optional<utils::CString> result;
        auto pos = mSamplerNamesBindingMap.find({ set, binding });
        if (pos != mSamplerNamesBindingMap.end()) {
            result = pos->second;
        }
        return result;
    }

private:
    SamplerBindingToNameMap mSamplerNamesBindingMap;
};

} // namespace filament

#endif // TNT_FILAMENT_DRIVER_SAMPLERBINDINGMAP_H
