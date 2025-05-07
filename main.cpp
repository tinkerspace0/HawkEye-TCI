#include "ThermalCamera.h"
#include <opencv2/highgui.hpp>
#include <iostream>
#include <thread>
#include <chrono>


int main(){
    // 1) Scan for connected TE devices
    auto devices = thermal::ThermalCamera::scanDevices();
    if (devices.empty()) {
        std::cerr << "No Thermal-Expert devices found!\n";
        return -1;
    }
    std::cout << "Found " << devices.size() << " device(s):\n";
    for (const auto &d : devices) {
        std::cout << "  Device #" << d.deviceNumber
                  << "  ProdVer=" << d.productVersion
                  << "  Serial=" << d.serialNumber << "\n";
    }
  

    // 2) Open the first device as an "Engine" camera (EQ1/EV1 etc.)
    const int ENGINE_MODEL = 3;  
    auto devNum = devices[0].deviceNumber;
    thermal::ThermalCamera cam;
    if (!cam.open(ENGINE_MODEL, devNum)) {
        std::cerr << "Failed to open camera #" << devNum << "\n";
        return -1;
    }
    std::cout << "Camera opened successfully.\n";


    // 3) Capture and show a single frame (with hardware AGC)
    {
      auto img = cam.captureImage(true);
      if (img.empty()) {
          std::cerr << "captureImage() returned empty frame\n";
      } else {
          cv::imshow("Single Frame", img);
          cv::waitKey(0);  // press any key to continue
      }
    }


    // 4) Print min/max temperature stats
    {
      auto stats = cam.getTemperatureStats(true);
      std::cout << "Min Temp: " << stats.minTemp << " °C at "
                << "(" << stats.minLoc.x << "," << stats.minLoc.y << ")\n";
      std::cout << "Max Temp: " << stats.maxTemp << " °C at "
                << "(" << stats.maxLoc.x << "," << stats.maxLoc.y << ")\n";
    }


    // 5) Perform shutter calibration
    if (cam.doCalibration()) {
      std::cout << "Shutter calibration succeeded.\n";
    } else {
        std::cout << "Shutter calibration failed or not supported.\n";
    }


    // 6) Adjust emissivity (example: human skin ≈ 0.98)
    cam.setEmissivity(0.98f);
    std::cout << "Emissivity set to 0.98\n";


    // 7) Start live streaming for 10 seconds
    std::cout << "Starting live stream for 10 seconds...\n";
    cam.startStream(
        [](const cv::Mat &frame) {
            cv::imshow("Live Stream", frame);
            cv::waitKey(1);
        },
        /*applyAgc=*/true
    );
    std::this_thread::sleep_for(std::chrono::seconds(10));
    cam.stopStream();
    std::cout << "Live stream stopped.\n";

    
    // 8) Close camera
    cam.close();
    std::cout << "Camera closed. Exiting.\n";
    return 0;
}
