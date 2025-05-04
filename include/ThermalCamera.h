// ThermalCamera.h
#pragma once

#include <opencv2/opencv.hpp>
#include "i3system/i3system_TE.h"
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * ThermalCamera provides a simple C++ interface
 * to open, configure, capture images and continuous
 * video from the TE-EQ1 thermal camera.
 */
class ThermalCamera {
public:
    ThermalCamera();
    ~ThermalCamera();

    bool open(int index = 0);

    void close();

    cv::Mat captureDisplayImage();

    void startStream(int fps = 10);

    bool getNextFrame(cv::Mat &frame);

    void stopStream();

    bool recordVideo(const std::string &filename, int fps, int duration_seconds);

    // radiometric configuration
    void setEmissivity(float e);
    void setTempOffset(float o);
    void setRotation(int degrees);

private:
    i3::TE_Cam cam_;
    bool isOpen_ = false;

    // streaming
    std::thread streamThread_;
    std::atomic<bool> streaming_{false};
    std::queue<cv::Mat> frameQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCond_;
    int streamFps_ = 10;

    void streamLoop();
};
