#include <iostream>
#include <stdio.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <unistd.h>
#include "i3system_TE.h"

using namespace cv;
using namespace std;
using namespace i3;

int getcharUntilReturn();
void ScanDevice();
void ConnectDeviceMenu();
void ConnectDevice(int _model, int _devNum);
void DevOperationMenu();
void ReceiveImage(bool _err);
void ReceiveImageCont();
void *RecvThread(void *ptr);
void GetTemperature();
void DoCalibration();
void DisconnectDevice();
void SettingMenu();
void AgcSetting();
void EmissivitySetting();
void DeviceChangedCallback(TE_STATE _in);

TE_A *gTE_A = NULL;
TE_B *gTE_B = NULL;

hotplug_callback_func gCallback;
bool gStopThread = true;
bool gApplyAgc = true;

char gMainMenu[] = {
    "\r\n "
    "\r\n ======================"
    "\r\n ==== Select Menu ====="
    "\r\n ======================"
    "\r\n "
    "\r\n 1 : Scan Connected Devices"
    "\r\n 2 : Connect Device"
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
};

char gConnectDevNumMenu[] = {
    "\r\n "
    "\r\n Type Device Number (0 ~ 31, x : Exit): "
};

char gScanResultMenu[] = {
    "\r\n "
    "\r\n ======================"
    "\r\n ==== Scan Result ====="
    "\r\n ======================"
    "\r\n "
    "\r\n "
};

char gConnectDevTypeMenu[] = {
    "\r\n "
    "\r\n ========================"
    "\r\n ==== Select Device ====="
    "\r\n ========================"
    "\r\n "
    "\r\n 1 : TE-Q1"
    "\r\n 2 : TE-V1"
    "\r\n 3 : Engine (TE-EQ1/TE-EV1/TE-EQ2/TE-EV2)"
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
    "\r\n "
};

char gDevOpMenu[] = {
    "\r\n "
    "\r\n ==========================="
    "\r\n ==== Select Operation ====="
    "\r\n ==========================="
    "\r\n "
    "\r\n 1 : Receive Image"
    "\r\n 2 : Receive Image continuously"
    "\r\n 3 : Get Temperature (Min, Max)"
    "\r\n 4 : Do Calibration"
    "\r\n 5 : Setting"
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
    "\r\n "
};

char gThreadExit[] = {
    "\r\n "
    "\r\n ========================="
    "\r\n ==== Running Thread ====="
    "\r\n ========================="
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
    "\r\n "
};

char gSettingMenu[] = {
    "\r\n "
    "\r\n ========================="
    "\r\n ==== Select Setting ====="
    "\r\n ========================="
    "\r\n "
    "\r\n 1 : AGC"
    "\r\n 2 : Emissivity"
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
};

char gAgcSettingMenu[] = {
    "\r\n "
    "\r\n ============================="
    "\r\n ==== Select AGC Setting ====="
    "\r\n ============================="
    "\r\n "
    "\r\n 1 : AGC On"
    "\r\n 2 : AGC Off"
    "\r\n "
    "\r\n x : Exit"
    "\r\n "
};

char gEmsSettingMenu[] = {
    "\r\n "
    "\r\n =========================="
    "\r\n ==== Type Emissivity ====="
    "\r\n =========================="
    "\r\n "
    "\r\n Emissivity (0 ~ 100, x : exit) : "
};

int main(){
    gCallback = &DeviceChangedCallback;
    SetHotplugCallback(gCallback);

 main_start:
    cout << gMainMenu << endl;

    int ch = getcharUntilReturn();
    cout << "\r\n";

    switch(ch){
    case '1':
        ScanDevice();
        goto main_start;
    case '2':
        ConnectDeviceMenu();
        goto main_start;
    case 'x':
    case 'X':
        return 1;
    default:
        cout << (char)ch << " is not supported" << endl;
        goto main_start;
    }
}


/**
   @brief Scans connected TE Devices
 */
void ScanDevice(){

    cout << gScanResultMenu;
    TEScanData devs[MAX_USB_NUM];
    if(ScanTE(devs) == 1){
        for(int i = 0; i < MAX_USB_NUM; ++i){
            if(devs[i].bDevCon == 1){

                cout << "Device Number : " << i << ", ";

                switch(devs[i].nProdVer){
                case I3_TE_EQ1:
                    cout << "Device : EQ1, ";
                    break;
                case I3_TE_EV1:
                    cout << "Device : EV1, ";
                    break;
                case I3_TE_EQ2:
                    cout << "Device : EQ2, ";
                    break;
                case I3_TE_EV2:
                    cout << "Device : EV2, ";
                    break;
                case 0:
                    cout << "Device : Q1 or V1, ";
                    break;
                default:
                    cout << "Unknown device, ";
                    break;
                }

                cout << "Serial Number : " << devs[i].nCoreID << endl;
            }
        }
    }
    else{
        cout << "Failed to scan devices" << endl;
    }
}

/**
   @brief Connect Thermal Expert to do USB communication
*/
void ConnectDeviceMenu(){
 connect_start:
    cout << gConnectDevNumMenu;
    char ch1, ch2;
    int num;
    ch1 = getchar();

    if(ch1 == 'x'){
        getcharUntilReturn();
        return;
    }
    else if(ch1 == '\n'){
        goto connect_start;
    }
    else if(ch1 >= '0' && ch1 <= '9'){
        ch2 = getcharUntilReturn();
        if(ch2 >= '0' && ch2 <= '9'){
            num = (int)(ch1 - '0') * 10 + (int)(ch2 - '0');
            if(num < 0 || num >= MAX_USB_NUM){
                cout << "Device number out of range" << endl;
                goto connect_start;
            }
        }
        else if(ch2 == '\n'){
            num = (int)(ch1 - '0');
            if(num < 0 || num >= MAX_USB_NUM){
                cout << "Device number out of range" << endl;
                goto connect_start;
            }
        }
        else{
            cout << "unsupported value" << endl;
            goto connect_start;
        }
    }
    else{
        getcharUntilReturn();
        goto connect_start;
    }

    cout << "\r\n";

    cout << gConnectDevTypeMenu;

    char ch = getcharUntilReturn();
    cout << "\r\n";

    switch(ch){
    case '1':
    case '2':
    case '3':
        ConnectDevice((int)(ch - '0'), num);
        break;
    case 'x':
    case 'X':
        return;
    default:
        cout << ch << " is unsupported vaule\r\n" << endl;
        goto connect_start;
    }
}

/**
   @brief Connect Device. Read Flash Data needs to be called in Q1 & V1 to initialize.
 */
void ConnectDevice(int _model, int _devNum){
    TEScanData devs[MAX_USB_NUM];
    if(ScanTE(devs) == 1){
        if((devs[_devNum].bDevCon == 0) || ((_model == 1 || _model == 2) && devs[_devNum].nProdVer != 0) || (_model == 3 && (devs[_devNum].nProdVer != I3_TE_EQ1 && devs[_devNum].nProdVer != I3_TE_EV1 && devs[_devNum].nProdVer != I3_TE_EQ2 && devs[_devNum].nProdVer != I3_TE_EV2))){
            cout << "Cannot connect device" << endl;
            return;
        }
    }
    else{
        cout << "Cannot connect device" << endl;
        return;
    }

    switch(_model){
    case 1:
        gTE_B = OpenTE_B(I3_TE_Q1, _devNum);
        break;
    case 2:
        gTE_B = OpenTE_B(I3_TE_V1, _devNum);
        break;
    case 3:
        gTE_A = OpenTE_A(_devNum);
        break;
    default:
        return;
    }

    if(gTE_B || gTE_A){
        DevOperationMenu();
    }
    else{
        cout << "Cannot connect to device " << _devNum << endl;
    }
}

void DevOperationMenu(){
    destroyAllWindows();
    namedWindow("Image");
    waitKey(1);
 dev_op_start:

    cout << gDevOpMenu;
    char ch = (char)getcharUntilReturn();
    switch(ch){
    case '1':
        ReceiveImage(true);
        goto dev_op_start;
    case '2':
        ReceiveImageCont();
        goto dev_op_start;
    case '3':
        GetTemperature();
        goto dev_op_start;
    case '4':
        DoCalibration();
        goto dev_op_start;
    case '5':
        SettingMenu();
        goto dev_op_start;
    case 'x':
    case 'X':
        DisconnectDevice();
        break;
    default:
        goto dev_op_start;
    }
}

/**
   @brief Receive image data and display image.
 */
void ReceiveImage(bool _err){

    if(gTE_A){
        int width = gTE_A->GetImageWidth();
        int height = gTE_A->GetImageHeight();
        int size = width * height;
        unsigned short *buf = new unsigned short[size];
        if(gTE_A->RecvImage(buf, gApplyAgc) == 1){
            if(gApplyAgc){
                Mat img(height, width, CV_16U, buf);
                imshow("Image", img);
                waitKey(1);
            }
            else{
                Mat imgOrig(height, width, CV_16U, buf);
                float gain = 0.2f, offset = -1500.f;
                Mat img;
                imgOrig.convertTo(img, CV_8UC1, gain, offset);
                imshow("Image", img);
                waitKey(1);
            }
        }
        else if(_err){
            cout << "Failed to receive image" << endl;
        }
        delete buf;
    }
    else if(gTE_B){
        int width = gTE_B->GetImageWidth();
        int height = gTE_B->GetImageHeight();
        if(gApplyAgc){
            unsigned short *buf = new unsigned short[width * height];
            if(gTE_B->RecvImage(buf) == 1){
                Mat img(height, width, CV_16U, buf);
                imshow("Image", img);
                waitKey(1);
            }
            else if(_err){
                cout << "Failed to receive image" << endl;
            }
            delete buf;
        }
        else{
            float *buf = new float[width * height];
            if(gTE_B->RecvImage(buf) == 1){
                Mat img32(height, width, CV_32F, buf);
                Mat img;
                img32.convertTo(img, CV_32F, 1./256.);
                imshow("Image", img);
                waitKey(1);
            }
            else if(_err){
                cout << "Failed to receive image" << endl;
            }
            delete buf;
        }
    }
}

/**
   @brief Receive image until type x
 */
void ReceiveImageCont(){
    pthread_t thrd;
    gStopThread = false;
    pthread_create(&thrd, NULL, RecvThread, 0);

 thread_msg:
    cout << gThreadExit;

    char ch = (char)getcharUntilReturn();
    if(ch == 'x' || ch == 'X'){
        gStopThread = true;
        pthread_join(thrd, NULL);
    }
    else{
        goto thread_msg;
    }
}

void *RecvThread(void *ptr){
    while(!gStopThread){
        ReceiveImage(false);
        usleep(33000);
    }
    return NULL;
}

/**
   @brief Get temperature data. RecvImage function needs to be done before CalcTemp.
*/
void GetTemperature(){
    if(gTE_A){
        int width = gTE_A->GetImageWidth();
        int height = gTE_A->GetImageHeight();
        int size = width * height;
        unsigned short *buf = new unsigned short[size];
        unsigned short *temp = new unsigned short[size];
        unsigned short minTemp = 65535, maxTemp = 0;
        int minX = 0, minY = 0, maxX = 0, maxY = 0;
        if(gTE_A->RecvImage(buf, true) == 1){
            gTE_A->CalcTemp(temp);

            for(int y = 0, pos = 0; y < height; ++y){
                for(int x = 0; x < width; ++x, ++pos){
                    unsigned short t = temp[pos];
                    if(maxTemp < t){
                        maxTemp = t;
                        maxX = x;
                        maxY = y;
                    }

                    if(minTemp > t){
                        minTemp = t;
                        minX = x;
                        minY = y;
                    }
                }
            }
            Mat img16(height, width, CV_16U, buf);
            Mat img8;
            img16.convertTo(img8, CV_8UC1, 1./256.);
            Mat in[] = {img8, img8, img8};
            Mat img;
            merge(in, 3, img);

            line(img, Point(minX - 5, minY), Point(minX + 5, minY), Scalar(255, 0, 0));
            line(img, Point(minX, minY - 5), Point(minX, minY + 5), Scalar(255, 0, 0));

            line(img, Point(maxX - 5, maxY), Point(maxX + 5, maxY), Scalar(0, 0, 255));
            line(img, Point(maxX, maxY - 5), Point(maxX, maxY + 5), Scalar(0, 0, 255));

            cout << "Min Temp : " << ((float)minTemp - 5000.f) / 100.f << endl;
            cout << "Max Temp : " << ((float)maxTemp - 5000.f) / 100.f << endl;

            imshow("Image", img);
            waitKey(1);
        }
        else{
            cout << "Failed to receive data" << endl;
        }
        delete buf;
        delete temp;
    }
    else if(gTE_B){
        int width = gTE_B->GetImageWidth();
        int height = gTE_B->GetImageHeight();
        int size = width * height;
        unsigned short *buf = new unsigned short[size];
        float *temp = new float[size];
        float minTemp = FLT_MAX, maxTemp = -FLT_MAX;
        int minX = 0, minY = 0, maxX = 0, maxY = 0;
        if(gTE_B->RecvImage(buf) == 1){
            gTE_B->CalcEntireTemp(temp);

            for(int y = 0, pos = 0; y < height; ++y){
                for(int x = 0; x < width; ++x, ++pos){
                    float t = temp[pos];
                    if(maxTemp < t){
                        maxTemp = t;
                        maxX = x;
                        maxY = y;
                    }

                    if(minTemp > t){
                        minTemp = t;
                        minX = x;
                        minY = y;
                    }
                }
            }
            Mat img16(height, width, CV_16U, buf);
            Mat img8;
            img16.convertTo(img8, CV_8UC1, 1./256.);
            Mat in[] = {img8, img8, img8};
            Mat img;
            merge(in, 3, img);

            line(img, Point(minX - 5, minY), Point(minX + 5, minY), Scalar(255, 0, 0));
            line(img, Point(minX, minY - 5), Point(minX, minY + 5), Scalar(255, 0, 0));

            line(img, Point(maxX - 5, maxY), Point(maxX + 5, maxY), Scalar(0, 0, 255));
            line(img, Point(maxX, maxY - 5), Point(maxX, maxY + 5), Scalar(0, 0, 255));

            cout << "Min Temp : " << minTemp << endl;
            cout << "Max Temp : " << maxTemp << endl;

            imshow("Image", img);
            waitKey(1);
        }
        else{
            cout << "Failed to receive data" << endl;
        }
        delete buf;
        delete temp;
    }
}

/**
   @brief calibration
 */
void DoCalibration(){
    if(gTE_A){
        if(gTE_A->ShutterCalibrationOn() != 1){
            cout << "Failed to calibrate" << endl;
        }
        else{
            cout << "Calibration success" << endl;
        }
    }
    else if(gTE_B){
        if(gTE_B->ShutterCalibrationOn() != 1){
            cout << "Failed to calibrate" << endl;
        }
        else{
            cout << "Calibration success" << endl;
        }
    }
}

/**
   @brief Disconnect Device.
 */
void DisconnectDevice(){
    if(gTE_A){
        gTE_A->CloseTE();
        gTE_A = NULL;
    }
    else if(gTE_B){
        gTE_B->CloseTE();
        gTE_B = NULL;
    }
}

/**
   @brief Change setting
 */
void SettingMenu(){
 setting_start:
    cout << gSettingMenu;
    char ch = getcharUntilReturn();
    switch(ch){
    case '1':
        AgcSetting();
        goto setting_start;
    case '2':
        EmissivitySetting();
        goto setting_start;
    case 'x':
    case 'X':
        return;
    default:
        goto setting_start;
    }
}

/**
   @brief Set AGC On/Off
 */
void AgcSetting(){
 agc_setting_start:
    cout << gAgcSettingMenu;
    char ch = (char)getcharUntilReturn();

    switch(ch){
    case '1':
        gApplyAgc = true;
        return;
    case '2':
        gApplyAgc = false;
        break;
    case 'x':
    case 'X':
        return;
    default:
        goto agc_setting_start;
    }
}

/**
   @brief Change emissivity value
 */
void EmissivitySetting(){
 ems_setting_start:
    cout << gEmsSettingMenu;
    int num;
    cin >> num;
    getcharUntilReturn();
    cout << "\r\n";

    if(num < 0 || num > 100){
        cout << "Emissivity value out of range " << num << endl;
        goto ems_setting_start;
    }

    if(gTE_A){
        gTE_A->SetEmissivity((float)num / 100.f);
    }
    else if(gTE_B){
        gTE_B->SetEmissivity((float)num / 100.f);
    }
}

/**
   @brief get first char and remove other char before returnn
*/
int getcharUntilReturn(){
    int ch = getchar();
    if(ch == '\n'){
        return ch;
    }
    else{
        int temp;
        while((temp = getchar()) != '\n');
        return ch;
    }
}

/**
   @brief Check if device is attached/detached.
 */
void DeviceChangedCallback(TE_STATE _in){
    if(_in.nUsbState == TE_ARRIVAL){
        cout << "Device Attached" << endl;
    }
    else if(_in.nUsbState == TE_REMOVAL){
        cout << "Device Detached" << endl;
    }
}
