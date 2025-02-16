/*
 * Copyright 2019 The Android Open Source Project
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

#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <binder/Parcelable.h>

namespace android {

/*
 * class for transporting gpu global stats from GpuService to authorized
 * recipents. This class is intended to be a data container.
 */
class GpuStatsGlobalInfo : public Parcelable {
public:
    GpuStatsGlobalInfo() = default;
    GpuStatsGlobalInfo(const GpuStatsGlobalInfo&) = default;
    virtual ~GpuStatsGlobalInfo() = default;
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* parcel);
    std::string toString() const;

    std::string driverPackageName = "";
    std::string driverVersionName = "";
    uint64_t driverVersionCode = 0;
    int64_t driverBuildTime = 0;
    int32_t glLoadingCount = 0;
    int32_t glLoadingFailureCount = 0;
    int32_t vkLoadingCount = 0;
    int32_t vkLoadingFailureCount = 0;
    int32_t vulkanVersion = 0;
    int32_t cpuVulkanVersion = 0;
    int32_t glesVersion = 0;
    int32_t angleLoadingCount = 0;
    int32_t angleLoadingFailureCount = 0;
};

/*
 * class for transporting gpu app stats from GpuService to authorized recipients.
 * This class is intended to be a data container.
 */
class GpuStatsAppInfo : public Parcelable {
public:
    // This limits the worst case number of extensions to be tracked.
    static const uint32_t MAX_NUM_EXTENSIONS = 100;
    // Max number of vulkan engine names for a single GpuStatsAppInfo
    static const uint32_t MAX_VULKAN_ENGINE_NAMES = 16;
    // Max length of a vulkan engine name string
    static const size_t MAX_VULKAN_ENGINE_NAME_LENGTH = 50;

    GpuStatsAppInfo() = default;
    GpuStatsAppInfo(const GpuStatsAppInfo&) = default;
    virtual ~GpuStatsAppInfo() = default;
    virtual status_t writeToParcel(Parcel* parcel) const;
    virtual status_t readFromParcel(const Parcel* parcel);
    std::string toString() const;

    std::string appPackageName = "";
    uint64_t driverVersionCode = 0;
    std::vector<int64_t> glDriverLoadingTime = {};
    std::vector<int64_t> vkDriverLoadingTime = {};
    std::vector<int64_t> angleDriverLoadingTime = {};
    bool cpuVulkanInUse = false;
    bool falsePrerotation = false;
    bool gles1InUse = false;
    bool angleInUse = false;
    bool createdGlesContext = false;
    bool createdVulkanDevice = false;
    bool createdVulkanSwapchain = false;
    uint32_t vulkanApiVersion = 0;
    uint64_t vulkanDeviceFeaturesEnabled = 0;
    std::vector<int32_t> vulkanInstanceExtensions = {};
    std::vector<int32_t> vulkanDeviceExtensions = {};
    std::vector<std::string> vulkanEngineNames = {};

    std::chrono::time_point<std::chrono::system_clock> lastAccessTime;
};

/*
 * class for holding the gpu stats in GraphicsEnv before sending to GpuService.
 */
class GpuStatsInfo {
public:
    enum Api {
        API_GL = 0,
        API_VK = 1,
    };

    enum Driver {
        NONE = 0,
        GL = 1,
        GL_UPDATED = 2,
        VULKAN = 3,
        VULKAN_UPDATED = 4,
        ANGLE = 5, // cover both system ANGLE and ANGLE APK
    };

    enum Stats {
        CPU_VULKAN_IN_USE = 0,
        FALSE_PREROTATION = 1,
        GLES_1_IN_USE = 2,
        CREATED_GLES_CONTEXT = 3,
        CREATED_VULKAN_API_VERSION = 4,
        CREATED_VULKAN_DEVICE = 5,
        CREATED_VULKAN_SWAPCHAIN = 6,
        VULKAN_DEVICE_FEATURES_ENABLED = 7,
        VULKAN_INSTANCE_EXTENSION = 8,
        VULKAN_DEVICE_EXTENSION = 9,
    };

    enum GLTelemetryHints {
        NO_HINT = 0,
        SKIP_TELEMETRY = 1,
    };

    GpuStatsInfo() = default;
    GpuStatsInfo(const GpuStatsInfo&) = default;
    virtual ~GpuStatsInfo() = default;

    std::string driverPackageName = "";
    std::string driverVersionName = "";
    uint64_t driverVersionCode = 0;
    int64_t driverBuildTime = 0;
    std::string appPackageName = "";
    int32_t vulkanVersion = 0;
    Driver glDriverToLoad = Driver::NONE;
    Driver vkDriverToLoad = Driver::NONE;
    Driver vkDriverFallback = Driver::NONE;
    bool glDriverToSend = false;
    bool vkDriverToSend = false;
    int64_t glDriverLoadingTime = 0;
    int64_t vkDriverLoadingTime = 0;
};

} // namespace android
