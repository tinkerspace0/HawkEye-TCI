#!/bin/sh

#Install to /usr/local/lib if it exists
if [ -d /usr/local/lib ]
then
# copy so files
	rm /usr/local/lib/libi3system*
	cp -r ./lib/libi3system*.so.2.* /usr/local/lib/
	ln -s /usr/local/lib/libi3system_te_64.so.2.* /usr/local/lib/libi3system_te_64.so.2
	ln -s /usr/local/lib/libi3system_usb_64.so.2.* /usr/local/lib/libi3system_usb_64.so.2
	ln -s /usr/local/lib/libi3system_te_64.so.2 /usr/local/lib/libi3system_te_64.so
	ln -s /usr/local/lib/libi3system_usb_64.so.2 /usr/local/lib/libi3system_usb_64.so
	ln -s /usr/local/lib/libi3system_imgproc_impl_64.so.2.* /usr/local/lib/libi3system_imgproc_impl_64.so.2
	ln -s /usr/local/lib/libi3system_imgproc_impl_64.so.2 /usr/local/lib/libi3system_imgproc_impl_64.so
	
	ln -s /usr/local/lib/libi3system_imgproc_64.so.2.* /usr/local/lib/libi3system_imgproc_64.so.2
	ln -s /usr/local/lib/libi3system_imgproc_64.so.2 /usr/local/lib/libi3system_imgproc_64.so

#	echo $(who | awk '{print $1; exit}')
	chown -R $(who | awk '{print $1; exit}'):$(who | awk '{print $1; exit}') /usr/local/lib/libi3system*

# copy header file
	if [ -d /usr/local/include/i3system ]
	then
		sudo rm -rf /usr/local/include/i3system
	fi
	mkdir /usr/local/include/i3system
	cp -r ./include/i3system* /usr/local/include/i3system/
	cp ./include/i3_types.h /usr/local/include/i3system/
	chown -R $(who | awk '{print $1; exit}'):$(who | awk '{print $1; exit}') /usr/local/include/i3system*

# install udev
	#udevStatus=""	
	udevStatus=$(dpkg -s libudev-dev | grep "installed" | awk '{print $4}')
	#udevStatus = $(dpkg -s libudev-dev)
	#udevStatus=$(sudo find / -name 'libudev-dev')
	if [ "$udevStatus" != "" ]
	then
		echo "Check libudev ... yes"
	else
		echo "Check libudev ... no"
		echo "Do you agree to install libudev?[y, n]"
		read instudev
		if [ $instudev = "y" ]
		then
			sudo apt-get install libudev-dev
		#elif [ $instudev = "n" ]
		#then
		
		fi
	fi

	
# install libusb
	#libusbStatus=""
	libusbStatus=$(dpkg -s libusb-1.0-0 | grep "installed" | awk '{print $4}')
	#libusbStatus=$(sudo find / -name 'libusb-1.0.so.0.1.0')
	if [ "$libusbStatus" = "installed" ]
	then
		echo "Check libusb ... yes"
	else
		echo "Check libusb ... no"
		echo "Do you agree to install libusb?[y, n]"
		read instusb
		if [ $instusb = "y" ]
		then
			cp ./libusb-1.0.20.tar.gz /home/
			tar -xvf /home/libusb-1.0.20.tar.gz -C /home/	
			/home/libusb-1.0.20/configure && make && make install
			rm /home/libusb-1.0.20.tar.gz
			rm -rf /home/libusb-1.0.20		
		#elif [ $instusb = "n" ]
		#then
		
		fi
	fi
	
# move i3Vendor.rules
	mv ./i3Vendor.rules /etc/udev/rules.d/

## install test program
#	echo "Do you want to install test program[y, n]?"
#	read instTP
#	if [ $instTP = "y" ]
#	then 
#		# install opencv
##		libusbStatus = $(dpkg -s libusb-1.0-0 | grep "installed" | awk '{print $4}')
#		cvcore=$(sudo find / -name 'libopencv_core.so.2.4.9')
#		if [ "$cvcore" != "" ]
#		then
#			echo "Check libopencv_core.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_core.so.2.4.9 ... no"
#		fi
#
#		cvcalib3d=$(sudo find / -name 'libopencv_calib3d.so.2.4.9')
#		if [ "$cvcalib3d" != "" ]
#		then
#			echo "Check libopencv_calib3d.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_calib3d.so.2.4.9 ... no"
#		fi
#
#		cvcontrib=$(sudo find / -name 'libopencv_contrib.so.2.4.9')
#
#		if [ "$cvcontrib" != "" ]
#		then
#			echo "Check libopencv_contrib.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_contrib.so.2.4.9 ... no"
#		fi
#
#		cvfeatures2d=$(sudo find / -name 'libopencv_features2d.so.2.4.9')
#
#		if [ "$cvfeatures2d" != "" ]
#		then
#			echo "Check libopencv_features2d.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_features2d.so.2.4.9 ... no"
#		fi
#
#		cvflann=$(sudo find / -name 'libopencv_flann.so.2.4.9')
#		if [ "$cvflann" != "" ]
#		then
#			echo "Check libopencv_flann.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_flann.so.2.4.9 ... no"
#		fi
#
#		cvhighgui=$(sudo find / -name 'libopencv_highgui.so.2.4.9')
#		if [ "$cvhighgui" != "" ]
#		then
#			echo "Check libopencv_highgui.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_highgui.so.2.4.9 ... no"
#		fi
#
#		cvimgproc=$(sudo find / -name 'libopencv_imgproc.so.2.4.9')
#		if [ "$cvimgproc" != "" ]
#		then
#			echo "Check libopencv_imgproc.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_imgproc.so.2.4.9 ... no"
#		fi
#
#		cvml=$(sudo find / -name 'libopencv_ml.so.2.4.9')
#
#		if [ "$cvml" != "" ]
#		then
#			echo "Check libopencv_ml.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_ml.so.2.4.9 ... no"
#		fi
#
#		cvobjdetect=$(sudo find / -name 'libopencv_objdetect.so.2.4.9')
#		if [ "$cvobjdetect" != "" ]
#		then
#			echo "Check libopencv_objdetect.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_objdetect.so.2.4.9 ... no"
#		fi
#
#
#		cvvideo=$(sudo find / -name 'libopencv_video.so.2.4.9')
#		if [ "$cvvideo" != "" ]
#		then
#			echo "Check libopencv_video.so.2.4.9 ... yes"
#		else
#			echo "Check libopencv_video.so.2.4.9 ... no"
#		fi
#
#		if [ "$cvvideo" == "" -o  "$cvobjdetect" == "" -o "$cvml" == "" -o "$cvimgproc" == "" -o "$cvhighgui" == "" -o "$cvflann" == "" -o "$cvfeatures2d" == "" -o "$cvcontrib" == "" -o "$cvcalib3d" == "" -o "$cvcore" == ""]
# 		then
# 			echo "Do you agree to install opencv?[y, n]"
# 			read instOpencv
# 			if [ $instOpencv = "y" ]
# 			then
# 				cp ./opencv-2.4.9.tar.gz /home/
# 				tar -xvf /home/opencv-2.4.9.tar.gz -C /home/
# 				cd /home/opencv-2.4.9
# 				cmake -D CMAKE_INSTALL_DIR=/usr/local/ -D.&&make&&sudo make install
# 				rm /home/opencv-2.4.9.tar.gz
# 				rm -rf /home/opencv-2.4.9
# 			fi
# 		fi

# 		# copy executable file in /usr/local/bin
# 		mv ./Library_Test_Program* /usr/local/bin/

# 		# install qt4
# 		qt4Status=$(dpkg -s qt4-default | grep "installed" | awk '{print $4}')
# 		if [ "$qt4Status" == "installed" ]
# 		then
# 			echo "Check qt4-default ... yes"
# 		else
# 			echo "Check qt4-default ... no"
# 			echo "Do you agree to install qt4-default?[y, n]"
# 			read instQt4
# 			if [ $instQt4 = "y" ]
# 			then
# 				sudo apt-get install qt4-default
# 			fi
# 		fi
# 	fi
fi
