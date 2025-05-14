// ThermalCamera.cpp
#include "ThermalCamera.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>

namespace thermal {

// Static initialization
template<typename Fn>
static Fn* s_hotplugCallback = nullptr;

// Proxy for the C‐style callback
// Must match C‐API exactly: void(*)(i3::TE_STATE)
void ThermalCamera::hotplugProxy(i3::TE_STATE state) {
    if (hotplugCallback_)
        hotplugCallback_(state);
}

ThermalCamera::ThermalCamera(const ThermalConfig& cfg)
    : agc_{cfg.agc}, emissivity_{cfg.emissivity} {
    switch (cfg.model) {
        case CameraModel::Q1:
            teB_ = i3::OpenTE_B(I3_TE_Q1, cfg.deviceNumber);
            break;
        case CameraModel::V1:
            teB_ = i3::OpenTE_B(I3_TE_V1, cfg.deviceNumber);
            break;
        case CameraModel::Engine:
            teA_ = i3::OpenTE_A(cfg.deviceNumber);
            break;
    }
    if (!teA_ && !teB_) {
        throw std::runtime_error("Failed to open thermal camera");
    }
    // Apply emissivity
    if (teA_) teA_->SetEmissivity(emissivity_);
    if (teB_) teB_->SetEmissivity(emissivity_);
}

ThermalCamera::~ThermalCamera() {
    stopRecording();
    stopStream();
    doClose();
}

bool ThermalCamera::isOpen() const noexcept {
    return teA_ != nullptr || teB_ != nullptr;
}

cv::Mat ThermalCamera::captureGreyscale() {
    cv::Mat raw = acquireRaw(agc_);
    if (raw.empty()) return {};
    cv::Mat gray8;
    if (agc_) {
        raw.convertTo(gray8, CV_8U, 1.0/256.0);
    } else {
        double mn, mx;
        cv::minMaxLoc(raw, &mn, &mx);
        raw.convertTo(gray8, CV_8U,
                      255.0/(mx - mn),
                      -mn * 255.0/(mx - mn));
    }
    return gray8;
}

cv::Mat ThermalCamera::captureRaw() {
    return acquireRaw(false);
}

cv::Mat ThermalCamera::acquireRaw(bool applyAgc) {
    const int MAX_RETRIES = 3;
    int retry = 0, ret = 0;
    if (teA_) {
        int w = teA_->GetImageWidth(), h = teA_->GetImageHeight();
        std::vector<unsigned short> buf(w * h);
        do {
            ret = teA_->RecvImage(buf.data(), applyAgc);
            if (ret == 1) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (++retry < MAX_RETRIES);
        if (ret != 1) {
            return {};
        }
        cv::Mat raw16(h, w, CV_16U, buf.data());
        return raw16.clone();
    } else if (teB_) {
        int w = teB_->GetImageWidth(), h = teB_->GetImageHeight();
        std::vector<unsigned short> buf(w * h);
        if (teB_->RecvImage(buf.data()) != 1) {
            return {};
        }
        cv::Mat raw16(h, w, CV_16U, buf.data());
        return raw16.clone();
    }
    return {};
}

void ThermalCamera::startStream(std::function<void(const cv::Mat&)> cb, double fps) {
    if (streaming_ || !cb) return;
    frameCallback_ = std::move(cb);
    streamFps_ = fps > 0 ? fps : streamFps_;
    streaming_ = true;
    streamThread_ = std::thread(&ThermalCamera::streamLoop, this);
}

void ThermalCamera::stopStream() {
    // Stop streaming thread
    streaming_ = false;
    if (streamThread_.joinable())
        streamThread_.join();
    // Also stop recording since no more frames will be produced
    if (recording_)
        stopRecording();
}

void ThermalCamera::startRecording(const std::string& filename,
                                   int fourcc,
                                   double fps) {
    if (recording_) return;
    int w = teA_ ? teA_->GetImageWidth() : teB_->GetImageWidth();
    int h = teA_ ? teA_->GetImageHeight() : teB_->GetImageHeight();
    videoWriter_.open(filename, fourcc, fps, cv::Size(w, h), false);
    if (!videoWriter_.isOpened()) {
        throw std::runtime_error("Failed to open video file");
    }
    recording_ = true;
}

void ThermalCamera::stopRecording() {
    if (!recording_) return;
    videoWriter_.release();
    recording_ = false;
}

void ThermalCamera::streamLoop() {
    const auto frameInterval = std::chrono::microseconds(
        static_cast<long long>(1e6 / streamFps_)
    );
    while (streaming_) {
        auto start = std::chrono::steady_clock::now();
        cv::Mat frame = captureGreyscale();
        if (frame.empty()) break;
        if (recording_ && videoWriter_.isOpened()) {
            videoWriter_.write(frame);
        }
        frameCallback_(frame);
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed < frameInterval) {
            std::this_thread::sleep_for(frameInterval - elapsed);
        }
    }
}

TempStats ThermalCamera::getTemperatureStats() {
    TempStats s{0, 0, {0, 0}, {0, 0}};
    if (teA_) {
        int w = teA_->GetImageWidth(), h = teA_->GetImageHeight();
        std::vector<unsigned short> imgBuf(w * h);
        std::vector<unsigned short> tempBuf(w * h);
        if (teA_->RecvImage(imgBuf.data(), false) == 1) {
            teA_->CalcTemp(tempBuf.data());
            unsigned short mn = std::numeric_limits<unsigned short>::max();
            unsigned short mx = 0;
            size_t minP = 0, maxP = 0;
            for (size_t i = 0, sz = imgBuf.size(); i < sz; ++i) {
                if (tempBuf[i] < mn) { mn = tempBuf[i]; minP = i; }
                if (tempBuf[i] > mx) { mx = tempBuf[i]; maxP = i; }
            }
            s.minTemp = (mn - 5000) / 100.0f;
            s.maxTemp = (mx - 5000) / 100.0f;
            s.minLoc = {static_cast<int>(minP % w), static_cast<int>(minP / w)};
            s.maxLoc = {static_cast<int>(maxP % w), static_cast<int>(maxP / w)};
        }
        return s;
    } else if (teB_) {
        int w = teB_->GetImageWidth(), h = teB_->GetImageHeight();
        std::vector<unsigned short> imgBuf(w * h);
        std::vector<float> tempBuf(w * h);
        if (teB_->RecvImage(imgBuf.data()) == 1) {
            teB_->CalcEntireTemp(tempBuf.data());
            float mn = std::numeric_limits<float>::max();
            float mx = std::numeric_limits<float>::lowest();
            size_t minP = 0, maxP = 0;
            for (size_t i = 0, sz = imgBuf.size(); i < sz; ++i) {
                if (tempBuf[i] < mn) { mn = tempBuf[i]; minP = i; }
                if (tempBuf[i] > mx) { mx = tempBuf[i]; maxP = i; }
            }
            s.minTemp = mn;
            s.maxTemp = mx;
            s.minLoc = {static_cast<int>(minP % w), static_cast<int>(minP / w)};
            s.maxLoc = {static_cast<int>(maxP % w), static_cast<int>(maxP / w)};
        }
        return s;
    }
    return s;
}

float ThermalCamera::temperatureAt(int x, int y) {
    if (!isOpen()) return 0.0f;
    int w = teA_ ? teA_->GetImageWidth() : teB_->GetImageWidth();
    int h = teA_ ? teA_->GetImageHeight() : teB_->GetImageHeight();
    if (x < 0 || x >= w || y < 0 || y >= h) return 0.0f;
    if (teA_) {
        std::vector<unsigned short> imgBuf(w * h);
        std::vector<unsigned short> tempBuf(w * h);
        if (teA_->RecvImage(imgBuf.data(), false) == 1) {
            teA_->CalcTemp(tempBuf.data());
            return (tempBuf[y * w + x] - 5000) / 100.0f;
        }
        return 0.0f;
    } else {
        std::vector<unsigned short> imgBuf(w * h);
        std::vector<float> tempBuf(w * h);
        if (teB_->RecvImage(imgBuf.data()) == 1) {
            teB_->CalcEntireTemp(tempBuf.data());
            return tempBuf[y * w + x];
        }
        return 0.0f;
    }
}

bool ThermalCamera::doCalibration() {
    if (teA_) return teA_->ShutterCalibrationOn() == 1;
    if (teB_) return teB_->ShutterCalibrationOn() == 1;
    return false;
}

void ThermalCamera::setEmissivity(float emissivity) {
    emissivity_ = emissivity;
    if (teA_) teA_->SetEmissivity(emissivity_);
    if (teB_) teB_->SetEmissivity(emissivity_);
}

void ThermalCamera::setAgc(bool enable) {
    agc_ = enable;
}

void ThermalCamera::doClose() noexcept {
    if (teA_) { teA_->CloseTE(); teA_ = nullptr; }
    if (teB_) { teB_->CloseTE(); teB_ = nullptr; }
}

std::vector<DeviceInfo> ThermalCamera::scanDevices() {
    i3::TEScanData devs[MAX_USB_NUM];
    std::vector<DeviceInfo> list;
    if (i3::ScanTE(devs) == 1) {
        for (int i = 0; i < MAX_USB_NUM; ++i) {
            if (devs[i].bDevCon) {
                list.push_back({
                    static_cast<unsigned int>(i),
                    devs[i].nProdVer,
                    devs[i].nCoreID
                });
            }
        }
    }
    return list;
}

void ThermalCamera::setHotplugCallback(HotplugFn cb) {
    hotplugCallback_ = std::move(cb);
    i3::SetHotplugCallback(&ThermalCamera::hotplugProxy);
}

} // namespace thermal
