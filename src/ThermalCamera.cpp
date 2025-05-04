// ThermalCamera.cpp
#include "ThermalCamera.h"
#include <chrono>
#include <iostream>

ThermalCamera::ThermalCamera() {}

ThermalCamera::~ThermalCamera() {
    stopStream();
    if(isOpen_) cam_.close();
}

bool ThermalCamera::open(int idx) {
    if(isOpen_) return true;
    isOpen_ = cam_.open();
    return isOpen_;
}

void ThermalCamera::close() {
    stopStream();
    if(isOpen_) cam_.close();
    isOpen_ = false;
}

cv::Mat ThermalCamera::captureDisplayImage() {
    if(!isOpen_) return {};
    auto frame = cam_.getFrame();
    return cam_.getDisplayImage(frame);
}

void ThermalCamera::startStream(int fps) {
    if(!isOpen_ || streaming_) return;
    streamFps_ = fps;
    streaming_ = true;
    streamThread_ = std::thread(&ThermalCamera::streamLoop, this);
}

void ThermalCamera::streamLoop() {
    using namespace std::chrono;
    milliseconds interval(1000 / streamFps_);
    while(streaming_) {
        auto start = steady_clock::now();
        cv::Mat img = captureDisplayImage();
        {
            std::lock_guard<std::mutex> lk(queueMutex_);
            frameQueue_.push(img);
        }
        queueCond_.notify_one();
        auto elapsed = steady_clock::now() - start;
        if(elapsed < interval) std::this_thread::sleep_for(interval - elapsed);
    }
    // notify any waiting getNextFrame()
    queueCond_.notify_all();
}

bool ThermalCamera::getNextFrame(cv::Mat &frame) {
    std::unique_lock<std::mutex> lk(queueMutex_);
    queueCond_.wait(lk, [&]{ return !frameQueue_.empty() || !streaming_; });
    if(frameQueue_.empty()) return false;
    frame = frameQueue_.front(); frameQueue_.pop();
    return true;
}

void ThermalCamera::stopStream() {
    if(!streaming_) return;
    streaming_ = false;
    if(streamThread_.joinable()) streamThread_.join();
    // clear any leftover frames
    std::lock_guard<std::mutex> lk(queueMutex_);
    std::queue<cv::Mat> empty;
    std::swap(frameQueue_, empty);
}

bool ThermalCamera::recordVideo(const std::string &filename, int fps, int duration_seconds) {
    if(!isOpen_) return false;
    cv::VideoWriter writer;
    cv::Size sz = captureDisplayImage().size();
    writer.open(filename, cv::VideoWriter::fourcc('M','J','P','G'), fps, sz, true);
    if(!writer.isOpened()) return false;

    startStream(fps);
    int totalFrames = fps * duration_seconds;
    cv::Mat frame;
    for(int i = 0; i < totalFrames; ++i) {
        if(!getNextFrame(frame)) break;
        writer.write(frame);
    }
    stopStream();
    writer.release();
    return true;
}

void ThermalCamera::setEmissivity(float e) { if(isOpen_) cam_.setEmissivity(e); }
void ThermalCamera::setTempOffset(float o) { if(isOpen_) cam_.setTempOffset(o); }
void ThermalCamera::setRotation(int d) { if(isOpen_) cam_.setRotation(d); }
