// ThermalCamera.h
#pragma once

#include <opencv2/core.hpp>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include "i3system_TE.h"

namespace thermal {

// Enumerates supported camera models
enum class CameraModel { Q1, V1, Engine };

// Bundles configuration options for the camera
struct ThermalConfig {
    CameraModel model;
    unsigned int deviceNumber = 0;
    bool agc = true;
    float emissivity = 0.98f;
};

// Low-level device info
struct DeviceInfo {
    unsigned int deviceNumber;
    unsigned int productVersion;
    unsigned int serialNumber;
};

// Temperature statistics
struct TempStats {
    float minTemp;      // °C
    float maxTemp;      // °C
    cv::Point minLoc;   // pixel coords
    cv::Point maxLoc;
};

class ThermalCamera {
public:
    using HotplugFn = std::function<void(i3::TE_STATE)>;

    // Construct & open; throws on failure
    explicit ThermalCamera(const ThermalConfig& cfg);
    ~ThermalCamera();

    ThermalCamera(const ThermalCamera&) = delete;
    ThermalCamera& operator=(const ThermalCamera&) = delete;

    bool isOpen() const noexcept;

    // Grabs an 8-bit grayscale frame (uses AGC setting)
    cv::Mat captureGreyscale();

    // Grabs a 16-bit raw frame (no AGC)
    cv::Mat captureRaw();

    // Streaming
    void startStream(std::function<void(const cv::Mat&)> frameCb);
    void stopStream();

    // Temperature stats & queries
    TempStats getTemperatureStats();
    float temperatureAt(int x, int y);

    // Calibration & settings
    bool doCalibration();                // runs shutter calibration
    void setEmissivity(float emissivity);
    void setAgc(bool enable);

    // Static helpers
    static std::vector<DeviceInfo> scanDevices();
    static void setHotplugCallback(HotplugFn cb);

private:
    void doClose() noexcept;
    cv::Mat acquireRaw(bool applyAgc);
    void streamLoop();

    i3::TE_A* teA_{nullptr};
    i3::TE_B* teB_{nullptr};
    bool agc_{false};
    float emissivity_{1.0f};

    std::thread streamThread_;
    std::atomic<bool> streaming_{false};
    std::function<void(const cv::Mat&)> frameCallback_;

    static HotplugFn hotplugCallback_;
    static void hotplugProxy(i3::TE_STATE state);
};

} // namespace thermal
