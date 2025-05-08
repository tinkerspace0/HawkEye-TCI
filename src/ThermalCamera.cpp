#include "ThermalCamera.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <unistd.h>
#include <iostream>

namespace thermal {

    // static member
    ThermalCamera::HotplugFn ThermalCamera::hotplugCallback_ = nullptr;

    // Proxy for the C‐style callback
    // Proxy must match C‐API exactly: void(*)(i3::TE_STATE)
    void ThermalCamera::hotplugProxy(i3::TE_STATE state) {
        if (hotplugCallback_) hotplugCallback_(state);
    }

    // — Static methods — 
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

    // — Construction / Destruction — 
    ThermalCamera::ThermalCamera() = default;
    ThermalCamera::~ThermalCamera() {
        close();
    }


    // — Open / Close — 
    bool ThermalCamera::open(int model, unsigned int devNum) {
        close();
        if (model == 1) {
            teB_ = i3::OpenTE_B(I3_TE_Q1, devNum);
        }
        else if (model == 2) {
            teB_ = i3::OpenTE_B(I3_TE_V1, devNum);
        }
        else if (model == 3) {
            teA_ = i3::OpenTE_A(devNum);
        } else {
            return false;
        }
        return (teA_ != nullptr) || (teB_ != nullptr);
    }

    void ThermalCamera::close() {
        if (teA_) { teA_->CloseTE(); teA_ = nullptr; }
        if (teB_) { teB_->CloseTE(); teB_ = nullptr; }
        stopStream();
    }


    // — Single‐frame capture — 
    cv::Mat ThermalCamera::captureImage(bool applyAgc) {
        const int MAX_RETRIES = 3;
        int retry = 0, ret = 0;
        cv::Mat gray8;

        if (teA_) {
            // 1) Get image size
            int w = teA_->GetImageWidth(), h = teA_->GetImageHeight();
            // 2) Allocate buffer
            auto buf = new unsigned short[w*h];
            
            // Retry loop for transient data‐read failures:
            do {
                ret = teA_->RecvImage(buf, applyAgc);
                if (ret == 1) break;
                std::cerr << "[WARN] RecvImage failed (code=" << ret
                        << "), retrying " << (retry+1) << "/" << MAX_RETRIES << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } while (++retry < MAX_RETRIES);

            if (ret != 1) {
                std::cerr << "[ERROR] captureImage: giving up after " 
                          << MAX_RETRIES << " retries (last code=" << ret << ")\n";
                delete[] buf;
                return {};  // still no image
            }
            {

                // Success: convert exactly as before…
                cv::Mat raw16(h, w, CV_16U, buf);
                if (!applyAgc)      raw16.convertTo(gray8, CV_8U, 0.2, -1500);
                else                raw16.convertTo(gray8, CV_8U);
                
                delete[] buf;
                
                // Expand to BGR (or apply a colormap if you prefer):
                cv::Mat color;
                cv::cvtColor(gray8, color, cv::COLOR_GRAY2BGR);
                // cv::applyColorMap(gray8, color, cv::COLORMAP_JET);
                
                return color;
            }
        }
        else if (teB_) {
            int w = teB_->GetImageWidth(), h = teB_->GetImageHeight();
            if (applyAgc) {
                auto buf = new unsigned short[w*h];
                if (teB_->RecvImage(buf) == 1) {
                    cv::Mat img(h, w, CV_16U, buf);
                    delete[] buf;
                    return img;
                }
                delete[] buf;
            } else {
                auto buf = new float[w*h];
                if (teB_->RecvImage(buf) == 1) {
                    cv::Mat img32(h, w, CV_32F, buf);
                    cv::Mat out; img32.convertTo(out, CV_8U, 1./256.);
                    delete[] buf;
                    return out;
                }
                delete[] buf;
            }
        }
        return {};
    }

    // — Streaming — 
    void ThermalCamera::startStream(std::function<void(const cv::Mat&)> cb,
                                    bool applyAgc) {
        if (streaming_ || !cb) return;
        frameCallback_ = std::move(cb);
        streaming_ = true;
        streamThread_ = std::thread(&ThermalCamera::streamLoop, this, applyAgc);
    }

    void ThermalCamera::stopStream() {
        streaming_ = false;
        if (streamThread_.joinable())
            streamThread_.join();
    }

    void ThermalCamera::streamLoop(bool applyAgc) {
        while (streaming_) {
            cv::Mat f = captureImage(applyAgc);
            if (f.empty()) break;
            frameCallback_(f);
            usleep(33000);
        }
        streaming_ = false;
    }

    // — Temperature statistics — 
    TempStats ThermalCamera::getTemperatureStats(bool applyAgc) {
        TempStats s{0,0,{0,0},{0,0}};
        if (teA_) {
            int w = teA_->GetImageWidth(), h = teA_->GetImageHeight();
            auto imgBuf  = new unsigned short[w*h];
            auto tempBuf = new unsigned short[w*h];
            if (teA_->RecvImage(imgBuf, applyAgc) == 1) {
                teA_->CalcTemp(tempBuf);
                unsigned short mn = USHRT_MAX, mx = 0;
                int minP = 0, maxP = 0;
                for (int i = 0, sz = w*h; i < sz; ++i) {
                    if (tempBuf[i] < mn) { mn = tempBuf[i]; minP = i; }
                    if (tempBuf[i] > mx) { mx = tempBuf[i]; maxP = i; }
                }
                s.minTemp = (mn - 5000) / 100.0f;
                s.maxTemp = (mx - 5000) / 100.0f;
                s.minLoc  = {minP % w, minP / w};
                s.maxLoc  = {maxP % w, maxP / w};
            }
            delete[] imgBuf; delete[] tempBuf;
        }
        else if (teB_) {
            int w = teB_->GetImageWidth(), h = teB_->GetImageHeight();
            auto imgBuf  = new unsigned short[w*h];
            auto tempBuf = new float[w*h];
            if (teB_->RecvImage(imgBuf) == 1) {
                teB_->CalcEntireTemp(tempBuf);
                float mn = FLT_MAX, mx = -FLT_MAX; int minP = 0, maxP = 0;
                for (int i = 0, sz = w*h; i < sz; ++i) {
                    if (tempBuf[i] < mn) { mn = tempBuf[i]; minP = i; }
                    if (tempBuf[i] > mx) { mx = tempBuf[i]; maxP = i; }
                }
                s.minTemp = mn; s.maxTemp = mx;
                s.minLoc  = {minP % w, minP / w};
                s.maxLoc  = {maxP % w, maxP / w};
            }
            delete[] imgBuf; delete[] tempBuf;
        }
        return s;
    }


    // — Calibration & settings — 
    bool ThermalCamera::doCalibration() {
        if (teA_) return teA_->ShutterCalibrationOn() == 1;
        if (teB_) return teB_->ShutterCalibrationOn() == 1;
        return false;
    }

    void ThermalCamera::setEmissivity(float e) {
        if (teA_) teA_->SetEmissivity(e);
        if (teB_) teB_->SetEmissivity(e);
    }


    void ThermalCamera::setAgc(bool enable) {
        // AGC is applied per-frame via captureImage/streamLoop
        agc_ = enable;
    }

} // namespace thermal




  