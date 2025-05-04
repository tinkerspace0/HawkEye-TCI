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

    /**
     * Open the TE-EQ1 device.
     * @param index USB index (default 0 selects first camera)
     * @return true on success
     */
    bool open(int index = 0);

    /**
     * Close the camera and stop any running stream
     */
    void close();

    /**
     * Capture a single display-ready image (8-bit BGR).
     * @return cv::Mat containing the image, or empty if failed
     */
    cv::Mat captureDisplayImage();

    /**
     * Start continuous capture in a background thread.
     * Frames will be pushed to an internal queue.
     * @param fps Desired frames per second
     */
    void startStream(int fps = 10);

    /**
     * Retrieve the next frame from the stream queue.
     * Blocks until a frame is available or stream stops.
     * @param frame Output cv::Mat frame
     * @return true if a frame was returned, false if stream ended
     */
    bool getNextFrame(cv::Mat &frame);

    /**
     * Stop the background stream thread.
     */
    void stopStream();

    /**
     * Record video to file using OpenCV VideoWriter.
     * This will pull frames from the internal queue.
     * @param filename Path to output video (e.g. .avi or .mp4)
     * @param fps Frames per second
     * @param duration_seconds Maximum duration to record
     * @return true on successful recording
     */
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
