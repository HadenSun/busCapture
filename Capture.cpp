/*****************************************************************************
*                                                                            *
*  Copyright (C) 2018 QSVision Ltd.                                          *
*                                                                            *
*  This file is used for real human face capture                             *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/


#include <iostream>
#include <fstream>

#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Capture.h"
#include <OpenNI.h>
#include "Viewer.h"
#include "AXonLink.h"

ofstream file("depthface_Hist_average.txt", ios::out);

Capture::Capture()
{}
Capture::~Capture()
{}

//objface.rect = re; (调整过后的框)
Rect Capture::checkface(void)                  
{
	if (objface.rect.width != 0 && objface.rect.height != 0)
		return objface.rect;
	return Rect();              
}

//提取出depth_grow图中的rect区域
Mat Capture::getGrownImage(void)               
{
	Mat dst = Mat(objface.mask, objface.rect);      //objface.mask = depth_grow.clone();
	return dst;
}                                

//objface.depthmin = dis_min; 
int Capture::getGrownImage_minval(void)           
{
	return objface.depthmin;                    
}
//objface.depthmax = dis_max;
int Capture::getGrownImage_maxval(void)       
{
	return objface.depthmax;
}  

//面部区域深度最大值与最小值的点位置
Point Capture::getGrownImage_minPoint(void)
{
	return objface.minPt;
}
Point Capture::getGrownImage_maxPoint(void)
{
	return objface.maxPt;
}

//objface.mask = depth_grow.clone(); 生长点的掩膜
Mat Capture::getMask()
{
	//return objface.mask.clone();
	return objface.mask;

}

Mat Capture::cropir(Mat imgir)
{
	Mat out_ir_img = imgir.clone();
	return out_ir_img;
}
void Capture::resetFaceSignal(void)
{
	Capture::isface = false;
}
void Capture::setFaceSignal(void)
{
	Capture::isface = true;
}

//--------寻找目标：人脸（其中用到了不少自定义子函数）-----------------------
//自定义子函数1：cutImage(depth_zip, &depth_cut); --过滤超出点----------------
//自定义子函数2：shadowDown(depth_cut, &depth_down, 0);----//向下投影---------
//自定义子函数3：findTop(depth_cut, maxP.x, 10, 10);）--	//找最高点--------
//自定义子函数4：cutBody(depth_cut, &depth_head, topY, 100, maxP.x, 60, &minHPt);---//提取头部-----------
//自定义子函数5：RegionGrow（）;-----区域生长------------
bool Capture::findobj(Mat imgdepth)  // use orignal depth data
{
	Mat depth = imgdepth.clone();
	Mat depth_zip;
	depth.convertTo(depth_zip, CV_8U, 1.0 / 6, -170);
	//cout << "\n压缩后的深度值"<<depth_zip<<endl;
	//imshow(" depth_zip", depth_zip);

	//角点检测
	/*Mat cornerStrength;
	cornerHarris(depth_zip, cornerStrength, 2, 3, 0.01);

	//对灰度图进行阈值操作
	Mat harrisCorner;
	threshold(cornerStrength, harrisCorner, 0.00001, 255, THRESH_BINARY);
	imshow("角点检测后的二值化效果", harrisCorner);*/

	//过滤超出点
	Mat depth_cut;
	int isAllblack=cutImage(depth_zip, &depth_cut);                              
	//鼠标点击查看像素信息
	//namedWindow("Display");
	//setMouseCallback("Display", on_mouse, &depth_cut);
	//imshow(" depth_cut", depth_cut);
if (!isAllblack)
 {
	    Mat depth_down;
	    shadowDown(depth_cut, &depth_down, 0);
	    Point maxP;
	   minMaxLoc(depth_down, 0, 0, 0, &maxP);      //得到最大值的索引  该索引maxP为(列，行) （max，0）

	//找最高点
	int topY = findTop(depth_cut, maxP.x, 10, 10);    

	if (topY != -1)
	{
		//截取头部
		Mat depth_head;
		Point minHPt = Point();                                     //minHPt代表脸部深度值最小的点
		cutBody(depth_cut, &depth_head, topY, 120, maxP.x,70, &minHPt);
		circle(depth_cut, minHPt, 3, Scalar(255));            //画出深度值最小的点（其实是画了一个略大的圆）
		//imshow("depth_head", depth_head);
		//imshow(" depth_cut", depth_cut);
		

		//区域生长
		Rect re;              //区域生长算法得到面部区域的矩形
		Mat depth_grow;       //区域生长算法得到的与深度图像大小相同(生成出来的为白色面部区域，其余为黑色)
		Point2i pt = Point2i(minHPt);
		Point minPt;           //可由区域生长算法求得的面部区域深度最小值的位置
		Point maxPt;           //可由区域生长算法求得的面部区域深度最大值的位置
		int dis_min;           //在区域生长算法中赋值了，面部区域的深度最小值
		int dis_max;           //在区域生长算法中赋值了，面部区域的深度最大值

		//pt的横、纵坐标为原图像的列、行值，pt(x,y)等价于srcImage.at<uchar>(pt.y, pt.x)
		int area_th = 21000;                                  //最大可设置30000（再大就会效果不好）

		int result = RegionGrow(depth_cut, &depth_grow, pt, 30, area_th, 6000, &re, &dis_min, &minPt, &dis_max, &maxPt);
		cout << "最小值点坐标：" << minPt.x << endl;
		//区域生长之后做正侧脸的判断
		//int isfrontfaceCheck = frontfaceCheck(depth_grow,re,minPt,4200);
		int isfrontfaceCheck =0;
		if (!isfrontfaceCheck)         //判断是正脸时，执行以下程序
		{
			//cout << "测试isfrontface" << endl;
			if (!result)      //该if语句的用于区域生长过后，判断距离的远近用
			{
				result = secondOrderCheck(depth, depth_grow, re, 5, (float)0.87);   //在未压缩的原深度图像做二阶检测
				if (!result)
					{
						//调整框大小
						int standardWidth = (int)(dis_min * (-0.37) + 180);
						re.x -= (standardWidth - re.width) / 2;
						int standardHeight = (int)(standardWidth * 1.3);
						re.y -= (standardHeight - re.height) / 2;
						re.height = standardHeight;
						re.width = standardWidth;

						if (re.x < depth_grow.cols / 4)
						re.x = depth_grow.cols / 4;
						if (re.x > depth_grow.cols / 4.0 * 3)
						re.x = (int)(depth_grow.cols / 4.0 * 3);

						if (re.y < 0)
						re.y = 0;
						if (re.y > depth_grow.rows - 1)
						re.y = depth_grow.rows - 1;

						if (re.x + re.width > depth_grow.cols - 1)
						re.width = depth_grow.cols - 1 - re.x;
						if (re.y + re.height > depth_grow.rows - 1)
						re.height = depth_grow.rows - 1 - re.y;

						//寻找结果保存
						objface.rect = re;
						objface.mask = depth_grow.clone();
						objface.depthmax = dis_max;
						objface.depthmin = dis_min;

						objface.minPt = Point(minPt);
						objface.maxPt = Point(maxPt);
						//rectangle(depth_zip, re, Scalar(255));
						//imshow("调整框大小后的depth_grow", depth_zip);
						//cout << "调整框大小完成" << endl;
						return true;
						}
			}
			else
			{
				if (result == -1)
					cout << "\t请保持正脸，靠近一点!!靠近!!靠近!!!" << endl;
				if (result == -2)
					cout << "\t请请保持正脸，靠远一点!靠远!靠远!!!" << endl;
				return false;
			}
			return false;                        //判断正侧脸的返回值
		}
	}
	//寻找头顶行失败时，提示将头靠近
	else
	{
		cout << "\t请保持正脸，把头靠近一点!!" << endl;
	}
	return false;
   }
   return false;
}

//将转换后的图像中深度值为255的置0（即是原深度图中距离过远的值）
//输入	src 灰度图像
//输入  dst 输出结果
//cutImage(depth_zip, &depth_cut);   
int Capture::cutImage(Mat src, Mat* dst)
{
	int count_black = 0;
	*dst = src.clone();
	for (int i = 0; i < dst->rows; i++)
	{
		for (int j = 0; j < dst->cols; j++)
		{
			if ((j < dst->cols *3/ 10 || j >(dst->cols *7/10))|| (i < dst->rows / 5 || i>(dst->rows / 5* 4)))       //左右、上下两边col/4置0
			{
				dst->at<uchar>(i, j) = 0;
			}
			else if (dst->at<uchar>(i, j) == 255)
			{
				dst->at<uchar>(i, j) = 0;
			}
		}
	}

	//dst为全黑图像，后续步骤就没有必要进行了！
	for (int i = 0; i < dst->rows; i++)
	{
		for (int j = 0; j < dst->cols; j++)
		{
			if (dst->at<uchar>(i, j) == 0)
				count_black++;
		}
	}
	if (count_black == (src.rows*src.cols))               //dst为全黑图像，后续步骤就没有必要进行了！
	{
		//cout << "全黑了！！！" << endl;
		return -1;
	}	
 return 0;
}

//向下投影
//参数：	src 输入灰度图像
//参数：	dst 输出一维统计数据
//参数：	image 是否显示统计图
//返回：	无
//shadowDown(depth_cut, &depth_down, 0);
void Capture::shadowDown(Mat src, Mat* dst, int image)
{
	cout << "向下投影进行中......." << endl;
	Mat shadow_down = Mat::zeros(Size(src.cols, 1), CV_16U);          //因为统计的深度值的数量可能超过255，故选用 CV_16U
	//向下投影
	for (int j = 0; j < src.cols; j++)
	{
		for (int i = 0; i < src.rows; i++)
		{
			if (src.at<uchar>(i, j) != 0)
			{
				shadow_down.at<ushort>(0, j)++;                       //统计每列的非0数量
			}
		}
	}

	//是否显示投影统计图，参数image为1，则显示。
	if (image)     
	{
		//首先先创建一个黑底的图像，为了可以显示彩色，所以该绘制图像是一个8位的3通道图像    
		Mat drawImage = Mat::zeros(Size(src.size().width, src.size().height), CV_8UC3);
		for (int i = 1; i < src.cols; i++)
		{
			int value = shadow_down.at<ushort>(0, i - 1);
			line(drawImage, Point(i, drawImage.rows - 1), Point(i, drawImage.rows - 1 - value), Scalar(0, 0, 255));
		}
		line(drawImage, Point(0, drawImage.rows - 1), Point(0, drawImage.rows - 1 - 0), Scalar(0, 0, 255));
		imshow("down_shadow", drawImage);
	}
	*dst = shadow_down;
}


//确定头顶位置的行(实际确定的行比头顶行略大点)
//输入：	src 灰度图像
//输入：	col 过滤列
//输入：	scaler 左右搜索范围
//输入：	th 阈值
//返回：	行号
//int topY = findTop(depth_cut, maxP.x, 10, 10);
int Capture::findTop(Mat src, int maxcol, int scaler, int th)
{
	cout << "确定深度图中头顶行的位置进行中........" << endl;
	for (int i = 0; i < src.rows*3/4; i++)                 //没有必要判断全部的行
	{
		int avaliCounter = 0;
		for (int j = 0; j <= scaler; j++)
		{
			//左右范围
			if (src.at<uchar>(i, maxcol + j) != 0)                   //最多深度值列的右边
				avaliCounter++;

			if (src.at<uchar>(i, maxcol -j) != 0)                   //最多深度值列的左边
				avaliCounter++;
		}
		if (avaliCounter > th)
		{
			cout << "头顶行为第：" << i << " 行" << endl;
			return i;                                                //可改进？？？？？？？
		}                                                            //如果全部找完，还没有找到符合值，是不是人距离的太远了？要提示靠近点
	}                                                                //前提是要确定有人站在前面！
	cout << "(寻找头顶行的位置失败！！！)" << endl;
	return -1;
}

//截取头顶部以下区域
//输入：	src 灰度图像
//输入：	dst 输出图像结果
//输入：	top 头顶的行
//输入：	th  头顶往下阈值
//输入：	middle 非零深度值最多的列即是maxP.x
//输入：	width 寻找中心点middle左右宽度
//输入：	pt	  最近点坐标（脸部深度值最小）
//cutBody(depth_cut, &depth_head, topY, 200, maxP.x, 100, &minHPt);
void Capture::cutBody(Mat src, Mat *dst, int top, int th, int middle, int width, Point* pt)
{
	cout << "正在截取头顶部以下区域..........." << endl;
	*dst = Mat::zeros(Size(src.size().width, src.size().height), CV_8U);        //Size(宽，高)
	uchar min = 255;
	int topRange;

	if (top + th > src.rows)
		topRange = src.rows;
	else
		topRange = top + th;
	 
	for (int i = src.rows/5; i < topRange; i++)        //因为五分之一行之前已经被置0(黑色了)，无需再遍历了
	{                                                  //可以考虑缩小循环范围，行从top开始，列范围col/4~col/4*3
		for (int j = src.cols *3/10 ; j < src.cols *7/10; j++)         //在之前这些列之外的值已经被置0了
		{
			dst->at<uchar>(i, j) = src.at<uchar>(i, j);

			//这个if的作用是：寻找脸部深度值最小的点(正常情况下可能是鼻子，戴帽子时可能是帽檐部分)
			if (j > middle - width && j < middle + width && src.at<uchar>(i, j) != 0)
			{
				if (min > src.at<uchar>(i, j))
				{
					min = src.at<uchar>(i, j);
					pt->x = j;
					pt->y = i;            //pt的横坐标被赋值为原图像的列值，纵坐标为行值
				}
			}
		}
	}
}

//区域生长算法
//参数：	src 灰度图
//参数：	dst 输出结果图
//参数：	pt	种子点
//参数：	th	生长的阈值条件
//参数：	area_max 区域最大值
//参数：	area_min 区域最小值
//参数：	re	范围矩形
//参数：	distance_min 最小距离点距离
//参数：	distanceMinPt 最小点位置
//参数：	distance_max 最大距离点距离
//参数：	distanceMaxPt 最大点位置
//返回：	成功0 失败-1
//int result = RegionGrow(depth_cut, &depth_grow, pt, 30, area_th, 5000, &re, &dis_min, &minPt, &dis_max, &maxPt);
int Capture::RegionGrow(Mat src, Mat* dst, Point2i pt, int th, int area_max, int area_min, Rect* re, int* distance_min, Point* distanceMinPt, int* distance_max, Point* distanceMaxPt)
{
	cout << "正在进行面部区域生长.........." << endl;
	Point2i ptGrowing;								//待生长点位置
	int nGrowLable = 0;								//标记是否生长过
	int nSrcValue = 0;								//生长起点深度值
	int nCurValue = 0;								//当前待生长点深度值
	Mat matDst = Mat::zeros(src.size(), CV_8UC1);	//创建一个空白区域，填充为黑色
	//生长方向顺序数据
	int DIR[8][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 } };

	int left = pt.x;
	int right = pt.x;                               //pt.x对应原图像列值（横坐标）
	int top = pt.y;                                 //pt.y对应原图像行值（列坐标）
	int bottom = pt.y;
	int counter = 0;								//记录生长面积
	*distance_min = 255;							//寻找最近点
	*distance_max = 0;                              //把要存放最大值的量初始化为最小值

	queue<Point2i> vcGrowPt;						//生长点队
	vcGrowPt.push(pt);								//将生长点加入队中
	matDst.at<uchar>(pt.y, pt.x) = 255;				//标记原始生长点为白色
	nSrcValue = src.at<uchar>(pt.y, pt.x);			//记录生长点的深度值
	
	//退出区域生长的条件
	while (!vcGrowPt.empty())						//生长队不为空则生长（该队中最多情况下存储8个元素，即8个方向点都成为生长点）
	{
		pt = vcGrowPt.front();						//取出第一个生长点
		vcGrowPt.pop();                             //从队中去除最靠前的元素

		//分别对八个方向上的点进行生长
		for (int i = 0; i < 8; ++i)
		{
			ptGrowing.x = pt.x + DIR[i][0];
			ptGrowing.y = pt.y + DIR[i][1];
			//检查是否是边缘点
			if (ptGrowing.x < 0 || ptGrowing.y < 0 || ptGrowing.x >(src.cols - 1) || (ptGrowing.y > src.rows - 1))
				continue;

			nGrowLable = matDst.at<uchar>(ptGrowing.y, ptGrowing.x);		//赋值当前图像点的标记(为0则为未生长点，255则为已经生长过的点)
			if (nGrowLable == 0)					                        //如果标记点还没有被生长
			{
				nCurValue = src.at<uchar>(ptGrowing.y, ptGrowing.x);        //当前待生长点灰度值
				if (nCurValue != 0)                                         //黑色点不参与生长
				{
					if (abs(nSrcValue - nCurValue) < th)					//在阈值范围内则生长（深度距离最小的点与待生长的点做差和阈值比较）
					{
						matDst.at<uchar>(ptGrowing.y, ptGrowing.x) = 255;	//标记为白色
						vcGrowPt.push(ptGrowing);					        //将该生长点加入队中

						if (left > ptGrowing.x)
							left = ptGrowing.x;
						if (right < ptGrowing.x)                    //每次都使生长点靠左、靠右、靠上、靠下
							right = ptGrowing.x;
						if (top > ptGrowing.y)                      //最后得到的是左上角点(left, top)和右下角点(right, bottom)
							top = ptGrowing.y;
						if (bottom < ptGrowing.y)
							bottom = ptGrowing.y;

						if (*distance_min > nCurValue)             
						{
							*distance_min = nCurValue;
							distanceMinPt->x = ptGrowing.x;           //找出深度最小值的位置
							distanceMinPt->y = ptGrowing.y;
						}
						else if (*distance_max < nCurValue)
						{
							*distance_max = nCurValue;
							distanceMaxPt->x = ptGrowing.x;           //记录下深度最大值的位置
							distanceMaxPt->y = ptGrowing.y;
						}
						     ++counter;                               //记录生长面积
					}

					//退出区域生长的条件
					//生长过程中，生长区域面积大于区域面积最大值时
					if (counter > area_max)
					{
						printf("生长过程中大于最大区域的退出！ Big area: %d\n", counter);
						cout << "距离太近，请保持正脸靠远一点！" << endl;
						return -2;
					}
				}
			}
		}
	}

//生长结束后，生长区域的面积处于区域最大值与最小值之间时
	printf("生长结束后输出面部区域的退出！area: %d\n", counter);
	//cout << "生长区域深度最小值：" << *distance_min << endl;
	*re = Rect(Point(left, top), Point(right, bottom));
	cout << "re.x: " << (*re).width<< "re.y:" << (*re).height << endl;
	*dst = matDst;                                      //这是复制矩阵信息头
	rectangle(matDst, *re, Scalar(255));                //在matDst（depth_grow）上框矩形
	imshow("depth_grow", matDst);

//生长结束后，生长区域的面积小于定义的区域最小值时
	if (counter < area_min)                      
	{
		printf("生长结束后，生长区域的面积小于区域最小阈值时退出 Small!\n");
		cout << "请保持正脸，靠近一点" << endl;
		return -1;
	}

//对生长区域进行宽高的检测(对一些满足了生长区域最小阈值的条件，但是明显不是人脸的区域)
	cout << "正在进行生长区域宽高检测........." << endl;
	int width_lth = 60;
	int height_lth = 70;
	int width_hth = 200;
	int height_hth = 200;
	if ((*re).width < width_lth || (*re).height < height_lth || (*re).width > width_hth || (*re).height > height_hth)
	{
		cout << "生长区域的宽高过小或过大" << endl;
		return -1;
	}

	return 0;
}


//二阶过滤+有效像素比例过滤
//输入：	src 原始深度图像ushort
//输入：	depthgrow 区域生长结果
//输入：	re  区域生长框
//输入：	orderTh 二阶梯度阈值
//输入：	th	过滤阈值
//输入：	areaTh 有效面积百分比阈值
//返回：	0 成功 -1错误
//result = secondOrderCheck(depth, depth_grow, re, 5, (float)0.9, 0.3f);   //在未压缩的原深度图像做二阶检测
int Capture::secondOrderCheck(Mat src, Mat depthgrow, Rect re, int orderTh, float th)
{
	cout << "正在进行面部平面检测(在原深度图中进行)........." << endl;
	long yAsisCounter = 0;
	long xAxisCounter = 0;

	int reLeft = re.x + 1;
	int reRight = re.x + re.width - 1;
	int reTop = re.y + 1;
	int reBottom = re.y + re.height - 1;

	int area = 0;        //统计re区域的非0深度值的面积（数量）

    //只需要检测区域生长出的部分对应的原深度图的二阶特性
	for (int i = reTop; i < reBottom; i++)
	{
		for (int j = reLeft; j < reRight; j++)
		{
			if (depthgrow.at<uchar>(i, j) != 0)                
			{
				ushort nCurValue = src.at<ushort>(i, j);
				//统计二阶
				if (abs(src.at<ushort>(i + 1, j) + src.at<ushort>(i - 1, j) - 2 * nCurValue) <orderTh)   //比较y方向的二阶导数
				{
					yAsisCounter++;
				}
				if (abs(src.at<ushort>(i, j + 1) + src.at<ushort>(i, j - 1) - 2 * nCurValue) < orderTh)  //比较x方向的二阶导数
				{
					xAxisCounter++;
				}
				area++;
			}
		}
	}
	cout << "(原深度图中矩形框总面积为: " << re.width * re.height << endl;
	cout << "(percent of XAxis_secondOrder:" << (double)xAxisCounter / area << endl;
	cout << "(percent of YAxis_secondOrder:" << (double)yAsisCounter / area << endl;

	//判断是否平面
	if (std::abs((double)xAxisCounter / area) > th)
	{
		cout << "(不满足平面过滤X的二阶导数！(阈值为0.87) return -1)" << endl;
		return -1;
	}
	if (std::abs((double)yAsisCounter / area) > th)
	{
		cout << "(不满足平面过滤Y的二阶导数！(阈值为0.87) return -1)" << endl;
		return -1;
	}

	//检查50%框有效像素区域
	cout << "50%框有效像素统计进行中(depth_grow image)......" << endl;
	int fiftyArea = 0;			//50%框有效像素统计
	reLeft = re.x + re.width / 4 + 5;
	reRight = re.x + re.width / 4 * 3 - 5;
	reTop = re.y + re.height / 4 + 5;
	reBottom = re.y + re.height / 4 * 3 - 5;
	for (int i = reTop; i < reBottom; i++)
	{
		for (int j = reLeft; j < reRight; j++)
		{
			if (depthgrow.at<uchar>(i, j) != 0)
			{
				fiftyArea++;
			}
		}
	}
	double Totalfifty = (re.height - 20) * (re.width - 20) / 4;
	double PerfiftyArea = (double)fiftyArea / Totalfifty;
	cout << "50%框总像素面积为：" << Totalfifty << "    有效面积为：" << fiftyArea << endl;
	cout <<"50%框有效像素面积比例(阈值为0.91)："<< PerfiftyArea << endl;
	if (PerfiftyArea < 0.91)
	{
		cout << "50%框有效像素面积统计不满足 return -1" << endl;
		return -1;
	}
	return 0;
}

//正侧脸检测
//输入：	depthgrow 区域生长结果
//输入：	re  区域生长框(未调整的)
//输入：	min 区域生长中的最小值点
//输入：	th	过滤阈值(自定义的左右两边相差的深度值个数)
//int isfrontfaceCheck = frontfaceCheck(depth_grow,re,minPt,30);
//int Capture::frontfaceCheck(Mat depthgrow, Rect re, Point min, int th)
int Capture::frontfaceCheck(Mat depthgrow, Rect re, Point min,int th)
{
	cout << "判断正侧脸正在进行........." << endl;
	int Rcount0 = 0;
	int Rcount255 = 0;
	int tRcount = 0;
	int Lcount0 = 0;
	int tLcount = 0;
	int Sub = 0;
	//rightrange = re.width+re.x-min.x;
	//leftrange = min.x - re.x;

	for (int i = min.x; i <re.width + re.x; i++)
	{
		for (int j = re.y+1; j < re.y + re.height; j++)              //统计最近点右边的深度值
		{
			if (depthgrow.at<uchar>(i, j) == 0)
				Rcount0++;
			if (depthgrow.at<uchar>(i, j) != 0)
				Rcount255++;
			tRcount++;
		}
	}
	for (int i = re.x+1; i <min.x; i++)
	{
		for (int j = re.y+1; j < re.y+re.height; j++)
		{
			if (depthgrow.at<uchar>(i, j) == 0)                    //统计最近点左边的深度值
				Lcount0++;
		   tLcount++;
		}
	}
	cout << "re.width:" << re.width << "   re.height:" <<re.height<< endl;
	cout << "areas: " << re.width*re.height << endl;
	cout << "tRcount:" << tRcount << "  tLcount:" << tLcount << " 和为：" << tRcount + tLcount << endl;
	cout << "Rcount0:" <<Rcount0<<"  Lcount0:"<<Lcount0<<endl;
	cout << "right 255 is:" <<Rcount255 << endl;
	Sub = abs(Rcount0 - Lcount0);
	cout << "左右两侧脸的差异为：" << Sub << endl;
	if (Sub> th)
		return -1;

	return 0;
}

//绘制人脸直方图
//输入：	srcimg     检测出的人脸区域
//输入：	mindepth   检测出的人脸区域的最小深度值
//输入：	maxdepth   检测出的人脸区域的最大深度值
//输入：	phhist	   垂直投影统计
//输入：	pvhist	   水平投影统计
//capture.getdepthHist(imagedepthROI, imagedepthROImin, imagedepthROImax, phhist, pvhist);
void Capture::getdepthHist(Mat srcimg, int mindepth, int maxdepth, int* phhist, int *pvhist)
{
	unsigned short grayValue;
	float tempvalue;
	int hist[256] = { 0 };
	int tempvaluemax = 0;
	int shiftvalue;
	int allvalue[26] = { 0 };
	int avgvalue[26] = { 0 };
	float par = (float)255 / (maxdepth - mindepth);     //归化系数，归化到(0~255)

	for (int i = 0; i < 256; i++)
	{
		phhist[i] = 0;
		pvhist[i] = 0;
	}
	for (int y = 0; y < srcimg.rows; y++)
	{
		for (int x = 0; x < srcimg.cols; x++)
		{
			grayValue = srcimg.at<ushort>(y, x);
			if ((grayValue > 0) && (grayValue < mindepth))
			{
				tempvalue = (float)mindepth;
			}
			else if (grayValue > maxdepth || grayValue == 0)
			{
				tempvalue = 0;
			}
			else
			{
				tempvalue = (float)(grayValue - mindepth);
			}
			tempvalue = tempvalue*par;      
			shiftvalue = (int)tempvalue;

			if (tempvalue >0)
			{
				phhist[y]++;
				pvhist[x]++;
				if (shiftvalue < 255)
				{
					hist[shiftvalue]++;
				}
			}
		}
	}

	//hist归一化处理;
	for (int i = 0; i < 256; i++)
	{
		if (tempvaluemax < hist[i])
			tempvaluemax = hist[i];               //找到深度值最多的那个小盒子，同时也就知道最多的数量是多少了
		//	cout << "hist" << hist[i] << endl;
	}
	float histpara = 255.0f / tempvaluemax;

	//首先先创建一个黑底的图像，为了可以显示彩色，所以该绘制图像是一个8位的3通道图像    
	Mat drawImage = Mat::zeros(Size(257, 257), CV_8UC3);

	//cout << "average:";
	for (int i = 0; i < 25; i++)
	{
		for (int j = 1; j < 11; j++)
		{
			allvalue[i] += hist[i * 10 + j];
		}
		avgvalue[i] = allvalue[i] / 10;         //连续10个深度值的平均数量

		file << int(avgvalue[i]) << ",";
		//cout << allvalue[i] << ",";              //输出到文件中
	}

	// face or fake check: test1
	/*for (int i = 20; i < 25; i++)
	{
		if (avgvalue[i] < 20)
		{
			resetFaceSignal();
			break;
		}
	}
	file << "\n";
	cout << "\n";*/

	//绘制直方图
	for (int i = 0; i < 25; i++)
	{
		for (int j = 1; j < 11; j++)
		{
			hist[i * 10 + j] = avgvalue[i];    //连续十个小盒子的直方图用他们的平均值表示
		}
	}
	for (int i = 1; i < 256; i++)
	{
		int value = hist[i];
		value = (int)(value * histpara);
		//cout << "value" << value << endl;
		line(drawImage, Point(i, drawImage.rows - 1), Point(i, drawImage.rows - 1 - value), Scalar(0, 0, 255));  //两点之间的距离为value
	}
	line(drawImage, Point(0, drawImage.rows - 1), Point(0, drawImage.rows - 1 - 0), Scalar(0, 0, 255));
	namedWindow("hist_zip");
	moveWindow("hist_zip", 700, 50);
	imshow("hist_zip", drawImage);
	return;
}
 

//比较mImageDepth图中的rect区域和depth_grow图中的rect区域
//将在depth_grow图中的rect区域为0的点也在mImageDepth图中的rect区域置0
//imageROI中的脸也就是区域生长出来的re区域对应在原深度图中脸
//imagedepthROI = capture.get3dface(imagedepthROI);
Mat Capture::get3dface(Mat imageroi)
{
	Mat mask;
	mask = getGrownImage();              //getGrownImage()函数是提取出depth_grow图中的re区域调整后的区域rect

	for (int i = 0; i < mask.rows; i++)
	{
		for (int j = 0; j < mask.cols; j++)
		{
			if (0 == mask.at<uchar>(i, j))
			{
				imageroi.at<ushort>(i, j) = 0;
			}
		}
	}
	return imageroi;
}


//-------------------------一些并未使用的函数--------------------------
//计算直方图
//参数：	imageGray 灰度图像
//参数：	isShow	-0 不绘制
//					-1 绘制
//返回值：	灰度直方图数组
/*
MatND Capture::myCalcHist(Mat imageGray, int isShow)
{
//计算直方图
int channels = 0;
MatND dstHist;
int histSize[] = { 256 };
float midRanges[] = { 0, 256 };
const float *ranges[] = { midRanges };
calcHist(&imageGray, 1, &channels, Mat(), dstHist, 1, histSize, ranges, true, false);

if (isShow)
{
//绘制直方图,首先先创建一个黑底的图像，为了可以显示彩色，所以该绘制图像是一个8位的3通道图像
Mat drawImage = Mat::zeros(Size(257, 257), CV_8UC3);
//任何一个图像的某个像素的总个数有可能会很多，甚至超出所定义的图像的尺寸，
//所以需要先对个数进行范围的限制，用minMaxLoc函数来得到计算直方图后的像素的最大个数
double g_dHistMaxValue;
Point maxLoc;
dstHist.at<float>(0) = 0;
minMaxLoc(dstHist, 0, &g_dHistMaxValue, &maxLoc, 0);
normalize(dstHist, dstHist, 0, 256, NORM_MINMAX, -1, Mat());
//将像素的个数整合到图像的最大范围内
for (int i = 1; i < 256; i++)
{
//int value = cvRound(dstHist.at<float>(i) * 256 * 0.9 / g_dHistMaxValue);
int value = (int)dstHist.at<float>(i - 1);
line(drawImage, Point(i, drawImage.rows - 1), Point(i, drawImage.rows - 1 - value), Scalar(0, 0, 255));
}
line(drawImage, Point(0, drawImage.rows - 1), Point(0, drawImage.rows - 1 - 0), Scalar(0, 0, 255));
imshow("histROI", drawImage);
}
return dstHist;
}
*/




//向左投影（未用）
//参数：	src 输入灰度图像
//参数：	dst 输出一维统计数据
//参数：	image 是否显示统计图
//返回：	无
/*
Mat Capture::shadowLeft(Mat src, Mat* dst, int image)
{

Mat shadow_left = Mat::zeros(Size(src.rows, 1), CV_16U);
//向左投影
for (int i = 0; i < src.rows; i++)
{
for (int j = 0; j < src.cols; j++)
{
if (src.at<uchar>(i, j) != 0)
{
shadow_left.at<ushort>(0, i)++;
}

}
}

if (image)
{
//首先先创建一个黑底的图像，为了可以显示彩色，所以该绘制图像是一个8位的3通道图像
Mat drawImage = Mat::zeros(Size(src.size().width, src.size().height), CV_8UC3);

for (int i = 1; i < src.rows; i++)
{
int value = shadow_left.at<ushort>(0, i - 1);
line(drawImage, Point(drawImage.cols - 1, i), Point(drawImage.cols - 1 - value, i), Scalar(0, 0, 255));
}
//line(drawImage, Point(drawImage.cols - 1, 0), Point(drawImage.cols - 1 - 0, 0), Scalar(0, 0, 255));
imshow("left_shadow", drawImage);
}


*dst = shadow_left.clone();

return shadow_left.clone();
}*/
