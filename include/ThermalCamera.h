#pragma once

#include <opencv2/core.hpp>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include "i3system_TE.h"

namespace thermal {

    struct DeviceInfo {
        unsigned int deviceNumber;   // 0–31
        unsigned int productVersion; // I3_TE_Q1, I3_TE_V1, I3_TE_EQ1, etc.
        unsigned int serialNumber;   // nCoreID
    };

    struct TempStats {
        float minTemp;      // in °C
        float maxTemp;      // in °C
        cv::Point minLoc;   // pixel coordinates
        cv::Point maxLoc;
    };


    class ThermalCamera {
        public:
            // now matches hotplug_callback_func: void(*)(i3::TE_STATE)
            using HotplugFn = std::function<void(i3::TE_STATE)>;
        
            ThermalCamera();
            ~ThermalCamera();
        
            // — Static device‐level operations — 
            static std::vector<DeviceInfo> scanDevices();  
            static void setHotplugCallback(HotplugFn cb);
        
            // — Connection management — 
            // model: 1=Q1, 2=V1, 3=Engine (EQ1/EV1/EQ2/EV2)
            bool open(int model, unsigned int deviceNumber);
            void close();
        
            // — Single‐frame grab — 
            // AGC behavior is controlled by setAgc()
            cv::Mat captureImage();

            // — Continuous video stream — 
            void startStream(std::function<void(const cv::Mat&)> frameCb);
            void stopStream();
        
            // — Temperature statistics (min/max) — 
            TempStats getTemperatureStats(bool applyAgc = true);
        
            // — Calibration & settings — 
            bool doCalibration();            // runs shutter calibration
            void setEmissivity(float e);     // 0.01–1.0
            void setAgc(bool enable);        // enable/disable AGC
        
        private:
            // internal thread func
            void streamLoop();
        
            // low‐level handles (only one is non‐null at a time)
            i3::TE_A* teA_{nullptr};
            i3::TE_B* teB_{nullptr};

            bool agc_{false}; // AGC enabled/disabled
            
            // streaming state
            std::thread            streamThread_;
            std::atomic<bool>      streaming_{false};
            std::function<void(const cv::Mat&)> frameCallback_;

            // hotplug callback
            static HotplugFn       hotplugCallback_;
            static void            hotplugProxy(i3::TE_STATE state);
        };

    } // namespace thermal
