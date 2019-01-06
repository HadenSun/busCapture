/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/

#include <OpenNI.h>
#include "Viewer.h"
#include "Capture.h"
#include "AXonLink.h"

#include <iostream>
#include <opencv2/opencv.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <direct.h>


using namespace std;
using namespace openni;
using namespace cv;


bool depth_img_isvalid = false;
bool color_img_isvalid = false;
bool ir_img_isvalid = false;
bool flag = 0;


bool get_true() { return true; }

Capture capture = Capture();

int main(int argc, char** argv)
{
	openni::Status rc = openni::STATUS_OK;

	openni::Device device;
	openni::VideoStream depth, color, ir;

	int nResolutionColor = 0;
	int nResolutionDepth = 0;
	int lastResolutionX = 0;
	int lastResolutionY = 0;
	const char* deviceURI = openni::ANY_DEVICE;

	if (argc > 1)
	{
		deviceURI = argv[1];
	}

	rc = openni::OpenNI::initialize();
	printf("After initialization:\n%s\n", openni::OpenNI::getExtendedError());

	rc = device.open(deviceURI);                /*打开任意可识别*/
	if (rc != openni::STATUS_OK)
	{
		printf("SimpleViewer: Device open failed:\n%s\n", openni::OpenNI::getExtendedError());
		openni::OpenNI::shutdown();            /*关闭openni*/
		return 1;
	}

	//--获取驱动版本号---------------------------------------
	OniVersion drver;
	int nsize;
	nsize = sizeof(drver);
	device.getProperty(ONI_DEVICE_PROPERTY_DRIVER_VERSION, &drver, &nsize);    /*获取驱动版本号*/
	printf("AXon driver version V%d.%d.%d.%d\n", drver.major, drver.minor,
		drver.maintenance, drver.build);

	// get color sensor
	const openni::SensorInfo* info = device.getSensorInfo(openni::SENSOR_COLOR);
	if (info)
	{
		for (int i = 0; i < info->getSupportedVideoModes().getSize(); i++)
		{
			printf("Color info : videomode %d %dx%d FPS %d f %d\n", i,
				info->getSupportedVideoModes()[i].getResolutionX(),
				info->getSupportedVideoModes()[i].getResolutionY(),
				info->getSupportedVideoModes()[i].getFps(),
				info->getSupportedVideoModes()[i].getPixelFormat());
			if ((info->getSupportedVideoModes()[i].getResolutionX() != lastResolutionX) || (info->getSupportedVideoModes()[i].getResolutionY() != lastResolutionY))
			{
				nResolutionColor++;
				lastResolutionX = info->getSupportedVideoModes()[i].getResolutionX();
				lastResolutionY = info->getSupportedVideoModes()[i].getResolutionY();
			}
		}
	}
	lastResolutionX = 0;
	lastResolutionY = 0;
	// get depth sensor
	const openni::SensorInfo* depthinfo = device.getSensorInfo(openni::SENSOR_DEPTH);
	if (depthinfo)
	{

		for (int i = 0; i < depthinfo->getSupportedVideoModes().getSize(); i++)
		{
			printf("\nDepth info: videomode %d %dx%d Fps %d f %d\n", i,                          //会被执行输出
				depthinfo->getSupportedVideoModes()[i].getResolutionX(),
				depthinfo->getSupportedVideoModes()[i].getResolutionY(),
				depthinfo->getSupportedVideoModes()[i].getFps(),
				depthinfo->getSupportedVideoModes()[i].getPixelFormat());
			if ((depthinfo->getSupportedVideoModes()[i].getResolutionX() != lastResolutionX) || (depthinfo->getSupportedVideoModes()[i].getResolutionY() != lastResolutionY))
			{
				nResolutionDepth++;
				lastResolutionX = depthinfo->getSupportedVideoModes()[i].getResolutionX();
				lastResolutionY = depthinfo->getSupportedVideoModes()[i].getResolutionY();
			}
		}
	}

	//------------ Creat depth stream ---------------------------------------------------
	rc = depth.create(device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		rc = depth.start();                       // start depth 
		if (rc != openni::STATUS_OK)
		{
			printf("SimpleViewer: Couldn't start depth stream:\n%s\n", openni::OpenNI::getExtendedError());
			depth.destroy();
		}
	}
	else
	{
		printf("SimpleViewer: Couldn't find depth stream:\n%s\n", openni::OpenNI::getExtendedError());
	}

	//-------------- Creat Color stream ----------------------------------------------------
	/*
	rc = color.create(device, openni::SENSOR_COLOR);
	if (rc == openni::STATUS_OK)
	{
		openni::VideoMode vm;
		openni::VideoMode dvm;
		dvm = depth.getVideoMode();
		vm = color.getVideoMode();

		vm.setResolution(dvm.getResolutionX(), dvm.getResolutionY());
		color.setVideoMode(vm);
		rc = color.start();  // start color
		if (rc != openni::STATUS_OK)
		{
			printf("SimpleViewer: Couldn't start color stream:\n%s\n", openni::OpenNI::getExtendedError());
			color.destroy();
		}
	}
	else
	{
		printf("SimpleViewer: Couldn't find color stream:\n%s\n", openni::OpenNI::getExtendedError());
	}


	AXonLinkCamParam camParam;
	int dataSize = sizeof(AXonLinkCamParam);
	device.getProperty(AXONLINK_DEVICE_PROPERTY_GET_CAMERA_PARAMETERS, &camParam, &dataSize);


	/*for (int i = 0; i < nResolutionColor; i++)
	{
	printf("astColorParam x =%d\n", camParam.astColorParam[i].ResolutionX);
	printf("astColorParam y =%d\n", camParam.astColorParam[i].ResolutionY);
	printf("astColorParam fx =%.5f\n", camParam.astColorParam[i].fx);
	printf("astColorParam fy =%.5f\n", camParam.astColorParam[i].fy);
	printf("astColorParam cx =%.5f\n", camParam.astColorParam[i].cx);
	printf("astColorParam cy =%.5f\n", camParam.astColorParam[i].cy);
	printf("astColorParam k1 =%.5f\n", camParam.astColorParam[i].k1);
	printf("astColorParam k2 =%.5f\n", camParam.astColorParam[i].k2);
	printf("astColorParam p1 =%.5f\n", camParam.astColorParam[i].p1);
	printf("astColorParam p2 =%.5f\n", camParam.astColorParam[i].p2);
	printf("astColorParam k3 =%.5f\n", camParam.astColorParam[i].k3);
	printf("astColorParam k4 =%.5f\n", camParam.astColorParam[i].k4);
	printf("astColorParam k5 =%.5f\n", camParam.astColorParam[i].k5);
	printf("astColorParam k6 =%.5f\n", camParam.astColorParam[i].k6);
	}
	for (int i = 0; i < nResolutionDepth; i++)
	{
	printf("astDepthParam x =%d\n", camParam.astDepthParam[i].ResolutionX);
	printf("astDepthParam y =%d\n", camParam.astDepthParam[i].ResolutionY);
	printf("astDepthParam fx =%.5f\n", camParam.astDepthParam[i].fx);
	printf("astDepthParam fy =%.5f\n", camParam.astDepthParam[i].fy);
	printf("astDepthParam cx =%.5f\n", camParam.astDepthParam[i].cx);
	printf("astDepthParam cy =%.5f\n", camParam.astDepthParam[i].cy);
	printf("astDepthParam k1 =%.5f\n", camParam.astDepthParam[i].k1);
	printf("astDepthParam k2 =%.5f\n", camParam.astDepthParam[i].k2);
	printf("astDepthParam p1 =%.5f\n", camParam.astDepthParam[i].p1);
	printf("astDepthParam p2 =%.5f\n", camParam.astDepthParam[i].p2);
	printf("astDepthParam k3 =%.5f\n", camParam.astDepthParam[i].k3);
	printf("astDepthParam k4 =%.5f\n", camParam.astDepthParam[i].k4);
	printf("astDepthParam k5 =%.5f\n", camParam.astDepthParam[i].k5);
	printf("astDepthParam k6 =%.5f\n", camParam.astDepthParam[i].k6);
	}
	printf("R = %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f \n", camParam.stExtParam.R_Param[0], camParam.stExtParam.R_Param[1], camParam.stExtParam.R_Param[2], camParam.stExtParam.R_Param[3], camParam.stExtParam.R_Param[4], camParam.stExtParam.R_Param[5], camParam.stExtParam.R_Param[6], camParam.stExtParam.R_Param[7], camParam.stExtParam.R_Param[8]);
	printf("T = %.5f %.5f %.5f \n", camParam.stExtParam.T_Param[0], camParam.stExtParam.T_Param[1], camParam.stExtParam.T_Param[2]);*/

	//--------------Creat Ir stream--------------------------------------------------
	/*
	rc = ir.create(device, openni::SENSOR_IR);
	if (rc == openni::STATUS_OK)
	{
		rc = ir.start();   // start IR 
		if (rc != openni::STATUS_OK)
		{
			printf("SimpleViewer: Couldn't start ir stream:\n%s\n", openni::OpenNI::getExtendedError());
			ir.destroy();
		}
	}
	else
	{
		printf("SimpleViewer: Couldn't find ir stream:\n%s\n", openni::OpenNI::getExtendedError());
	}
	AXonLinkGetExposureLevel value;
	int nSize = sizeof(value);
	ir.getProperty(AXONLINK_STREAM_PROPERTY_EXPOSURE_LEVEL, &value, &nSize);

	printf("\nGet level:custId=%d,max=%d,current=%d\n\n", value.customID, value.maxLevel, value.curLevel);     //会被执行输出


	if (!depth.isValid() || !color.isValid() || !ir.isValid())
	{
		printf("SimpleViewer: No valid streams. Exiting\n");
		openni::OpenNI::shutdown();
		return 2;
	}
	rc = device.setDepthColorSyncEnabled(true);
	if (rc != openni::STATUS_OK)
	{
		printf("start sync failed1\n");
		openni::OpenNI::shutdown();
		return 4;
	}

	rc = device.setImageRegistrationMode(IMAGE_REGISTRATION_DEPTH_TO_COLOR);				//对齐
	if (rc != openni::STATUS_OK)
	{
		printf("start registration failed\n");
		openni::OpenNI::shutdown();
		return 5;
	}
	*/

	//--------------------------------------------
	int changedIndex;
	openni::VideoStream**	m_streams;

	m_streams = new openni::VideoStream*[3];
	m_streams[0] = &depth;
	m_streams[1] = &color;
	m_streams[2] = &ir;
	VideoFrameRef  frameDepth;
	VideoFrameRef  frameColor;
	VideoFrameRef  frameIr;

	cv::Mat mScaledDepth;
	cv::Mat mRgbDepth;
	cv::Mat mScaledIr;
	cv::Mat mImageDepth;
	cv::Mat mImageRGB;
	cv::Mat mImageIr;

	cv::Mat cImageBGR;
	cv::Mat cImageIr;

	cv::Mat mFaceImg;


	Vec3b colorpix = Vec3b(255, 255, 0);
	Vec3b color_value;


	int readIndex = 0;
	int count = 0;		//保存文件计数



	//-------------------------
	// main loop 
	//-------------------------
	while (get_true())
	{
		rc = openni::OpenNI::waitForAnyStream(m_streams, 3, &changedIndex);
		if (rc != openni::STATUS_OK)
		{
			printf("Wait failed\n");
			return 1;
		}

		switch (changedIndex)
		{
		case 0:
			depth.readFrame(&frameDepth);                    //读深度流帧数据
			readIndex++;
			depth_img_isvalid = true;

			mImageDepth = cv::Mat(frameDepth.getHeight(), frameDepth.getWidth(), CV_16UC1, (void*)frameDepth.getData());
			//mImageDepth.convertTo(mRgbDepth, CV_8U, 1.0 / 16, 0);
			//applyColorMap(mRgbDepth, mRgbDepth, COLORMAP_JET);	  //将mImageDepth变成看似的深度图(伪彩色图)

			//imshow("Depth", mRgbDepth);
			//waitKey(1);

			break;
		case 1:
			//color.readFrame(&frameColor);
			color_img_isvalid = true;
			//mImageRGB = cv::Mat(frameColor.getHeight(), frameColor.getWidth(), CV_8UC3, (void*)frameColor.getData());
			//cv::cvtColor(mImageRGB, cImageBGR, CV_RGB2BGR);

			//imshow("Color", cImageBGR);
			//cv::waitKey(1);


			break;
		case 2:
			//ir.readFrame(&frameIr);
			ir_img_isvalid = true;
			//mImageIr = cv::Mat(frameIr.getHeight(), frameIr.getWidth(), CV_8UC1, (void *)frameIr.getData());

			//imshow("Ir", mImageIr);
			//cv::waitKey(1);

			break;
		default:
			printf("Error in wait\n");
		}


		
		//获取当前时间  HHMM
		time_t t = time(0);
		char tmp[64];
		strftime(tmp, sizeof(tmp), "%H%M", localtime(&t));
		if (tmp[3] == 0)
		{
			tmp[3] = tmp[2];
			tmp[2] = '0';
			tmp[4] = 0;
		}


		//存储路径 C:/HHMM（当前时间）
		String dir = "C:/" + String(tmp);
		if (_access(dir.c_str(), 0) == -1)
		{
			//文件夹不存在 创建文件夹
			int flag = _mkdir(dir.c_str());
			count = 1;
			if (flag == 0)
			{
				cout << "make successfully" << endl;
			}
			else {
				cout << "make fsiled" << endl;
			}
		}

		//保存图片
		if (depth_img_isvalid)
		{
			depth_img_isvalid = false;

			//图片命名：xxxx.png
			char str[256];
			sprintf(str, "%04d.png", count);

			vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);  //选择jpeg
			compression_params.push_back(9); //在这个填入你要的图片质量

			String depth_dir = dir + "/" + String(str);
			imwrite(depth_dir, mImageDepth, compression_params);

			count++;
		}
		




		//-----------------------------------------------------
		// 终止快捷键
		if (cv::waitKey(10) == 'q')
			break;
	}
	// 关闭数据流
	depth.destroy();
	color.destroy();
	ir.destroy();
	// 关闭设备
	device.close();
	// 最后关闭OpenNI
	openni::OpenNI::shutdown();
	return 0;
}

