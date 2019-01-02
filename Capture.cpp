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

//objface.rect = re; (��������Ŀ�)
Rect Capture::checkface(void)                  
{
	if (objface.rect.width != 0 && objface.rect.height != 0)
		return objface.rect;
	return Rect();              
}

//��ȡ��depth_growͼ�е�rect����
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

//�沿����������ֵ����Сֵ�ĵ�λ��
Point Capture::getGrownImage_minPoint(void)
{
	return objface.minPt;
}
Point Capture::getGrownImage_maxPoint(void)
{
	return objface.maxPt;
}

//objface.mask = depth_grow.clone(); ���������Ĥ
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

//--------Ѱ��Ŀ�꣺�����������õ��˲����Զ����Ӻ�����-----------------------
//�Զ����Ӻ���1��cutImage(depth_zip, &depth_cut); --���˳�����----------------
//�Զ����Ӻ���2��shadowDown(depth_cut, &depth_down, 0);----//����ͶӰ---------
//�Զ����Ӻ���3��findTop(depth_cut, maxP.x, 10, 10);��--	//����ߵ�--------
//�Զ����Ӻ���4��cutBody(depth_cut, &depth_head, topY, 100, maxP.x, 60, &minHPt);---//��ȡͷ��-----------
//�Զ����Ӻ���5��RegionGrow����;-----��������------------
bool Capture::findobj(Mat imgdepth)  // use orignal depth data
{
	Mat depth = imgdepth.clone();
	Mat depth_zip;
	depth.convertTo(depth_zip, CV_8U, 1.0 / 6, -170);
	//cout << "\nѹ��������ֵ"<<depth_zip<<endl;
	//imshow(" depth_zip", depth_zip);

	//�ǵ���
	/*Mat cornerStrength;
	cornerHarris(depth_zip, cornerStrength, 2, 3, 0.01);

	//�ԻҶ�ͼ������ֵ����
	Mat harrisCorner;
	threshold(cornerStrength, harrisCorner, 0.00001, 255, THRESH_BINARY);
	imshow("�ǵ����Ķ�ֵ��Ч��", harrisCorner);*/

	//���˳�����
	Mat depth_cut;
	int isAllblack=cutImage(depth_zip, &depth_cut);                              
	//������鿴������Ϣ
	//namedWindow("Display");
	//setMouseCallback("Display", on_mouse, &depth_cut);
	//imshow(" depth_cut", depth_cut);
if (!isAllblack)
 {
	    Mat depth_down;
	    shadowDown(depth_cut, &depth_down, 0);
	    Point maxP;
	   minMaxLoc(depth_down, 0, 0, 0, &maxP);      //�õ����ֵ������  ������maxPΪ(�У���) ��max��0��

	//����ߵ�
	int topY = findTop(depth_cut, maxP.x, 10, 10);    

	if (topY != -1)
	{
		//��ȡͷ��
		Mat depth_head;
		Point minHPt = Point();                                     //minHPt�����������ֵ��С�ĵ�
		cutBody(depth_cut, &depth_head, topY, 120, maxP.x,70, &minHPt);
		circle(depth_cut, minHPt, 3, Scalar(255));            //�������ֵ��С�ĵ㣨��ʵ�ǻ���һ���Դ��Բ��
		//imshow("depth_head", depth_head);
		//imshow(" depth_cut", depth_cut);
		

		//��������
		Rect re;              //���������㷨�õ��沿����ľ���
		Mat depth_grow;       //���������㷨�õ��������ͼ���С��ͬ(���ɳ�����Ϊ��ɫ�沿��������Ϊ��ɫ)
		Point2i pt = Point2i(minHPt);
		Point minPt;           //�������������㷨��õ��沿���������Сֵ��λ��
		Point maxPt;           //�������������㷨��õ��沿����������ֵ��λ��
		int dis_min;           //�����������㷨�и�ֵ�ˣ��沿����������Сֵ
		int dis_max;           //�����������㷨�и�ֵ�ˣ��沿�����������ֵ

		//pt�ĺᡢ������Ϊԭͼ����С���ֵ��pt(x,y)�ȼ���srcImage.at<uchar>(pt.y, pt.x)
		int area_th = 21000;                                  //��������30000���ٴ�ͻ�Ч�����ã�

		int result = RegionGrow(depth_cut, &depth_grow, pt, 30, area_th, 6000, &re, &dis_min, &minPt, &dis_max, &maxPt);
		cout << "��Сֵ�����꣺" << minPt.x << endl;
		//��������֮�������������ж�
		//int isfrontfaceCheck = frontfaceCheck(depth_grow,re,minPt,4200);
		int isfrontfaceCheck =0;
		if (!isfrontfaceCheck)         //�ж�������ʱ��ִ�����³���
		{
			//cout << "����isfrontface" << endl;
			if (!result)      //��if���������������������жϾ����Զ����
			{
				result = secondOrderCheck(depth, depth_grow, re, 5, (float)0.87);   //��δѹ����ԭ���ͼ�������׼��
				if (!result)
					{
						//�������С
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

						//Ѱ�ҽ������
						objface.rect = re;
						objface.mask = depth_grow.clone();
						objface.depthmax = dis_max;
						objface.depthmin = dis_min;

						objface.minPt = Point(minPt);
						objface.maxPt = Point(maxPt);
						//rectangle(depth_zip, re, Scalar(255));
						//imshow("�������С���depth_grow", depth_zip);
						//cout << "�������С���" << endl;
						return true;
						}
			}
			else
			{
				if (result == -1)
					cout << "\t�뱣������������һ��!!����!!����!!!" << endl;
				if (result == -2)
					cout << "\t���뱣����������Զһ��!��Զ!��Զ!!!" << endl;
				return false;
			}
			return false;                        //�ж��������ķ���ֵ
		}
	}
	//Ѱ��ͷ����ʧ��ʱ����ʾ��ͷ����
	else
	{
		cout << "\t�뱣����������ͷ����һ��!!" << endl;
	}
	return false;
   }
   return false;
}

//��ת�����ͼ�������ֵΪ255����0������ԭ���ͼ�о����Զ��ֵ��
//����	src �Ҷ�ͼ��
//����  dst ������
//cutImage(depth_zip, &depth_cut);   
int Capture::cutImage(Mat src, Mat* dst)
{
	int count_black = 0;
	*dst = src.clone();
	for (int i = 0; i < dst->rows; i++)
	{
		for (int j = 0; j < dst->cols; j++)
		{
			if ((j < dst->cols *3/ 10 || j >(dst->cols *7/10))|| (i < dst->rows / 5 || i>(dst->rows / 5* 4)))       //���ҡ���������col/4��0
			{
				dst->at<uchar>(i, j) = 0;
			}
			else if (dst->at<uchar>(i, j) == 255)
			{
				dst->at<uchar>(i, j) = 0;
			}
		}
	}

	//dstΪȫ��ͼ�񣬺��������û�б�Ҫ�����ˣ�
	for (int i = 0; i < dst->rows; i++)
	{
		for (int j = 0; j < dst->cols; j++)
		{
			if (dst->at<uchar>(i, j) == 0)
				count_black++;
		}
	}
	if (count_black == (src.rows*src.cols))               //dstΪȫ��ͼ�񣬺��������û�б�Ҫ�����ˣ�
	{
		//cout << "ȫ���ˣ�����" << endl;
		return -1;
	}	
 return 0;
}

//����ͶӰ
//������	src ����Ҷ�ͼ��
//������	dst ���һάͳ������
//������	image �Ƿ���ʾͳ��ͼ
//���أ�	��
//shadowDown(depth_cut, &depth_down, 0);
void Capture::shadowDown(Mat src, Mat* dst, int image)
{
	cout << "����ͶӰ������......." << endl;
	Mat shadow_down = Mat::zeros(Size(src.cols, 1), CV_16U);          //��Ϊͳ�Ƶ����ֵ���������ܳ���255����ѡ�� CV_16U
	//����ͶӰ
	for (int j = 0; j < src.cols; j++)
	{
		for (int i = 0; i < src.rows; i++)
		{
			if (src.at<uchar>(i, j) != 0)
			{
				shadow_down.at<ushort>(0, j)++;                       //ͳ��ÿ�еķ�0����
			}
		}
	}

	//�Ƿ���ʾͶӰͳ��ͼ������imageΪ1������ʾ��
	if (image)     
	{
		//�����ȴ���һ���ڵ׵�ͼ��Ϊ�˿�����ʾ��ɫ�����Ըû���ͼ����һ��8λ��3ͨ��ͼ��    
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


//ȷ��ͷ��λ�õ���(ʵ��ȷ�����б�ͷ�����Դ��)
//���룺	src �Ҷ�ͼ��
//���룺	col ������
//���룺	scaler ����������Χ
//���룺	th ��ֵ
//���أ�	�к�
//int topY = findTop(depth_cut, maxP.x, 10, 10);
int Capture::findTop(Mat src, int maxcol, int scaler, int th)
{
	cout << "ȷ�����ͼ��ͷ���е�λ�ý�����........" << endl;
	for (int i = 0; i < src.rows*3/4; i++)                 //û�б�Ҫ�ж�ȫ������
	{
		int avaliCounter = 0;
		for (int j = 0; j <= scaler; j++)
		{
			//���ҷ�Χ
			if (src.at<uchar>(i, maxcol + j) != 0)                   //������ֵ�е��ұ�
				avaliCounter++;

			if (src.at<uchar>(i, maxcol -j) != 0)                   //������ֵ�е����
				avaliCounter++;
		}
		if (avaliCounter > th)
		{
			cout << "ͷ����Ϊ�ڣ�" << i << " ��" << endl;
			return i;                                                //�ɸĽ���������������
		}                                                            //���ȫ�����꣬��û���ҵ�����ֵ���ǲ����˾����̫Զ�ˣ�Ҫ��ʾ������
	}                                                                //ǰ����Ҫȷ������վ��ǰ�棡
	cout << "(Ѱ��ͷ���е�λ��ʧ�ܣ�����)" << endl;
	return -1;
}

//��ȡͷ������������
//���룺	src �Ҷ�ͼ��
//���룺	dst ���ͼ����
//���룺	top ͷ������
//���룺	th  ͷ��������ֵ
//���룺	middle �������ֵ�����м���maxP.x
//���룺	width Ѱ�����ĵ�middle���ҿ��
//���룺	pt	  ��������꣨�������ֵ��С��
//cutBody(depth_cut, &depth_head, topY, 200, maxP.x, 100, &minHPt);
void Capture::cutBody(Mat src, Mat *dst, int top, int th, int middle, int width, Point* pt)
{
	cout << "���ڽ�ȡͷ������������..........." << endl;
	*dst = Mat::zeros(Size(src.size().width, src.size().height), CV_8U);        //Size(����)
	uchar min = 255;
	int topRange;

	if (top + th > src.rows)
		topRange = src.rows;
	else
		topRange = top + th;
	 
	for (int i = src.rows/5; i < topRange; i++)        //��Ϊ���֮һ��֮ǰ�Ѿ�����0(��ɫ��)�������ٱ�����
	{                                                  //���Կ�����Сѭ����Χ���д�top��ʼ���з�Χcol/4~col/4*3
		for (int j = src.cols *3/10 ; j < src.cols *7/10; j++)         //��֮ǰ��Щ��֮���ֵ�Ѿ�����0��
		{
			dst->at<uchar>(i, j) = src.at<uchar>(i, j);

			//���if�������ǣ�Ѱ���������ֵ��С�ĵ�(��������¿����Ǳ��ӣ���ñ��ʱ������ñ�ܲ���)
			if (j > middle - width && j < middle + width && src.at<uchar>(i, j) != 0)
			{
				if (min > src.at<uchar>(i, j))
				{
					min = src.at<uchar>(i, j);
					pt->x = j;
					pt->y = i;            //pt�ĺ����걻��ֵΪԭͼ�����ֵ��������Ϊ��ֵ
				}
			}
		}
	}
}

//���������㷨
//������	src �Ҷ�ͼ
//������	dst ������ͼ
//������	pt	���ӵ�
//������	th	��������ֵ����
//������	area_max �������ֵ
//������	area_min ������Сֵ
//������	re	��Χ����
//������	distance_min ��С��������
//������	distanceMinPt ��С��λ��
//������	distance_max ����������
//������	distanceMaxPt ����λ��
//���أ�	�ɹ�0 ʧ��-1
//int result = RegionGrow(depth_cut, &depth_grow, pt, 30, area_th, 5000, &re, &dis_min, &minPt, &dis_max, &maxPt);
int Capture::RegionGrow(Mat src, Mat* dst, Point2i pt, int th, int area_max, int area_min, Rect* re, int* distance_min, Point* distanceMinPt, int* distance_max, Point* distanceMaxPt)
{
	cout << "���ڽ����沿��������.........." << endl;
	Point2i ptGrowing;								//��������λ��
	int nGrowLable = 0;								//����Ƿ�������
	int nSrcValue = 0;								//����������ֵ
	int nCurValue = 0;								//��ǰ�����������ֵ
	Mat matDst = Mat::zeros(src.size(), CV_8UC1);	//����һ���հ��������Ϊ��ɫ
	//��������˳������
	int DIR[8][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 } };

	int left = pt.x;
	int right = pt.x;                               //pt.x��Ӧԭͼ����ֵ�������꣩
	int top = pt.y;                                 //pt.y��Ӧԭͼ����ֵ�������꣩
	int bottom = pt.y;
	int counter = 0;								//��¼�������
	*distance_min = 255;							//Ѱ�������
	*distance_max = 0;                              //��Ҫ������ֵ������ʼ��Ϊ��Сֵ

	queue<Point2i> vcGrowPt;						//�������
	vcGrowPt.push(pt);								//��������������
	matDst.at<uchar>(pt.y, pt.x) = 255;				//���ԭʼ������Ϊ��ɫ
	nSrcValue = src.at<uchar>(pt.y, pt.x);			//��¼����������ֵ
	
	//�˳���������������
	while (!vcGrowPt.empty())						//�����Ӳ�Ϊ�����������ö����������´洢8��Ԫ�أ���8������㶼��Ϊ�����㣩
	{
		pt = vcGrowPt.front();						//ȡ����һ��������
		vcGrowPt.pop();                             //�Ӷ���ȥ���ǰ��Ԫ��

		//�ֱ�԰˸������ϵĵ��������
		for (int i = 0; i < 8; ++i)
		{
			ptGrowing.x = pt.x + DIR[i][0];
			ptGrowing.y = pt.y + DIR[i][1];
			//����Ƿ��Ǳ�Ե��
			if (ptGrowing.x < 0 || ptGrowing.y < 0 || ptGrowing.x >(src.cols - 1) || (ptGrowing.y > src.rows - 1))
				continue;

			nGrowLable = matDst.at<uchar>(ptGrowing.y, ptGrowing.x);		//��ֵ��ǰͼ���ı��(Ϊ0��Ϊδ�����㣬255��Ϊ�Ѿ��������ĵ�)
			if (nGrowLable == 0)					                        //�����ǵ㻹û�б�����
			{
				nCurValue = src.at<uchar>(ptGrowing.y, ptGrowing.x);        //��ǰ��������Ҷ�ֵ
				if (nCurValue != 0)                                         //��ɫ�㲻��������
				{
					if (abs(nSrcValue - nCurValue) < th)					//����ֵ��Χ������������Ⱦ�����С�ĵ���������ĵ��������ֵ�Ƚϣ�
					{
						matDst.at<uchar>(ptGrowing.y, ptGrowing.x) = 255;	//���Ϊ��ɫ
						vcGrowPt.push(ptGrowing);					        //����������������

						if (left > ptGrowing.x)
							left = ptGrowing.x;
						if (right < ptGrowing.x)                    //ÿ�ζ�ʹ�����㿿�󡢿��ҡ����ϡ�����
							right = ptGrowing.x;
						if (top > ptGrowing.y)                      //���õ��������Ͻǵ�(left, top)�����½ǵ�(right, bottom)
							top = ptGrowing.y;
						if (bottom < ptGrowing.y)
							bottom = ptGrowing.y;

						if (*distance_min > nCurValue)             
						{
							*distance_min = nCurValue;
							distanceMinPt->x = ptGrowing.x;           //�ҳ������Сֵ��λ��
							distanceMinPt->y = ptGrowing.y;
						}
						else if (*distance_max < nCurValue)
						{
							*distance_max = nCurValue;
							distanceMaxPt->x = ptGrowing.x;           //��¼��������ֵ��λ��
							distanceMaxPt->y = ptGrowing.y;
						}
						     ++counter;                               //��¼�������
					}

					//�˳���������������
					//���������У��������������������������ֵʱ
					if (counter > area_max)
					{
						printf("���������д������������˳��� Big area: %d\n", counter);
						cout << "����̫�����뱣��������Զһ�㣡" << endl;
						return -2;
					}
				}
			}
		}
	}

//���������������������������������ֵ����Сֵ֮��ʱ
	printf("��������������沿������˳���area: %d\n", counter);
	//cout << "�������������Сֵ��" << *distance_min << endl;
	*re = Rect(Point(left, top), Point(right, bottom));
	cout << "re.x: " << (*re).width<< "re.y:" << (*re).height << endl;
	*dst = matDst;                                      //���Ǹ��ƾ�����Ϣͷ
	rectangle(matDst, *re, Scalar(255));                //��matDst��depth_grow���Ͽ����
	imshow("depth_grow", matDst);

//����������������������С�ڶ����������Сֵʱ
	if (counter < area_min)                      
	{
		printf("����������������������С��������С��ֵʱ�˳� Small!\n");
		cout << "�뱣������������һ��" << endl;
		return -1;
	}

//������������п�ߵļ��(��һЩ����������������С��ֵ���������������Բ�������������)
	cout << "���ڽ������������߼��........." << endl;
	int width_lth = 60;
	int height_lth = 70;
	int width_hth = 200;
	int height_hth = 200;
	if ((*re).width < width_lth || (*re).height < height_lth || (*re).width > width_hth || (*re).height > height_hth)
	{
		cout << "��������Ŀ�߹�С�����" << endl;
		return -1;
	}

	return 0;
}


//���׹���+��Ч���ر�������
//���룺	src ԭʼ���ͼ��ushort
//���룺	depthgrow �����������
//���룺	re  ����������
//���룺	orderTh �����ݶ���ֵ
//���룺	th	������ֵ
//���룺	areaTh ��Ч����ٷֱ���ֵ
//���أ�	0 �ɹ� -1����
//result = secondOrderCheck(depth, depth_grow, re, 5, (float)0.9, 0.3f);   //��δѹ����ԭ���ͼ�������׼��
int Capture::secondOrderCheck(Mat src, Mat depthgrow, Rect re, int orderTh, float th)
{
	cout << "���ڽ����沿ƽ����(��ԭ���ͼ�н���)........." << endl;
	long yAsisCounter = 0;
	long xAxisCounter = 0;

	int reLeft = re.x + 1;
	int reRight = re.x + re.width - 1;
	int reTop = re.y + 1;
	int reBottom = re.y + re.height - 1;

	int area = 0;        //ͳ��re����ķ�0���ֵ�������������

    //ֻ��Ҫ��������������Ĳ��ֶ�Ӧ��ԭ���ͼ�Ķ�������
	for (int i = reTop; i < reBottom; i++)
	{
		for (int j = reLeft; j < reRight; j++)
		{
			if (depthgrow.at<uchar>(i, j) != 0)                
			{
				ushort nCurValue = src.at<ushort>(i, j);
				//ͳ�ƶ���
				if (abs(src.at<ushort>(i + 1, j) + src.at<ushort>(i - 1, j) - 2 * nCurValue) <orderTh)   //�Ƚ�y����Ķ��׵���
				{
					yAsisCounter++;
				}
				if (abs(src.at<ushort>(i, j + 1) + src.at<ushort>(i, j - 1) - 2 * nCurValue) < orderTh)  //�Ƚ�x����Ķ��׵���
				{
					xAxisCounter++;
				}
				area++;
			}
		}
	}
	cout << "(ԭ���ͼ�о��ο������Ϊ: " << re.width * re.height << endl;
	cout << "(percent of XAxis_secondOrder:" << (double)xAxisCounter / area << endl;
	cout << "(percent of YAxis_secondOrder:" << (double)yAsisCounter / area << endl;

	//�ж��Ƿ�ƽ��
	if (std::abs((double)xAxisCounter / area) > th)
	{
		cout << "(������ƽ�����X�Ķ��׵�����(��ֵΪ0.87) return -1)" << endl;
		return -1;
	}
	if (std::abs((double)yAsisCounter / area) > th)
	{
		cout << "(������ƽ�����Y�Ķ��׵�����(��ֵΪ0.87) return -1)" << endl;
		return -1;
	}

	//���50%����Ч��������
	cout << "50%����Ч����ͳ�ƽ�����(depth_grow image)......" << endl;
	int fiftyArea = 0;			//50%����Ч����ͳ��
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
	cout << "50%�����������Ϊ��" << Totalfifty << "    ��Ч���Ϊ��" << fiftyArea << endl;
	cout <<"50%����Ч�����������(��ֵΪ0.91)��"<< PerfiftyArea << endl;
	if (PerfiftyArea < 0.91)
	{
		cout << "50%����Ч�������ͳ�Ʋ����� return -1" << endl;
		return -1;
	}
	return 0;
}

//���������
//���룺	depthgrow �����������
//���룺	re  ����������(δ������)
//���룺	min ���������е���Сֵ��
//���룺	th	������ֵ(�Զ�������������������ֵ����)
//int isfrontfaceCheck = frontfaceCheck(depth_grow,re,minPt,30);
//int Capture::frontfaceCheck(Mat depthgrow, Rect re, Point min, int th)
int Capture::frontfaceCheck(Mat depthgrow, Rect re, Point min,int th)
{
	cout << "�ж����������ڽ���........." << endl;
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
		for (int j = re.y+1; j < re.y + re.height; j++)              //ͳ��������ұߵ����ֵ
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
			if (depthgrow.at<uchar>(i, j) == 0)                    //ͳ���������ߵ����ֵ
				Lcount0++;
		   tLcount++;
		}
	}
	cout << "re.width:" << re.width << "   re.height:" <<re.height<< endl;
	cout << "areas: " << re.width*re.height << endl;
	cout << "tRcount:" << tRcount << "  tLcount:" << tLcount << " ��Ϊ��" << tRcount + tLcount << endl;
	cout << "Rcount0:" <<Rcount0<<"  Lcount0:"<<Lcount0<<endl;
	cout << "right 255 is:" <<Rcount255 << endl;
	Sub = abs(Rcount0 - Lcount0);
	cout << "�����������Ĳ���Ϊ��" << Sub << endl;
	if (Sub> th)
		return -1;

	return 0;
}

//��������ֱ��ͼ
//���룺	srcimg     ��������������
//���룺	mindepth   �����������������С���ֵ
//���룺	maxdepth   ���������������������ֵ
//���룺	phhist	   ��ֱͶӰͳ��
//���룺	pvhist	   ˮƽͶӰͳ��
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
	float par = (float)255 / (maxdepth - mindepth);     //�黯ϵ�����黯��(0~255)

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

	//hist��һ������;
	for (int i = 0; i < 256; i++)
	{
		if (tempvaluemax < hist[i])
			tempvaluemax = hist[i];               //�ҵ����ֵ�����Ǹ�С���ӣ�ͬʱҲ��֪�����������Ƕ�����
		//	cout << "hist" << hist[i] << endl;
	}
	float histpara = 255.0f / tempvaluemax;

	//�����ȴ���һ���ڵ׵�ͼ��Ϊ�˿�����ʾ��ɫ�����Ըû���ͼ����һ��8λ��3ͨ��ͼ��    
	Mat drawImage = Mat::zeros(Size(257, 257), CV_8UC3);

	//cout << "average:";
	for (int i = 0; i < 25; i++)
	{
		for (int j = 1; j < 11; j++)
		{
			allvalue[i] += hist[i * 10 + j];
		}
		avgvalue[i] = allvalue[i] / 10;         //����10�����ֵ��ƽ������

		file << int(avgvalue[i]) << ",";
		//cout << allvalue[i] << ",";              //������ļ���
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

	//����ֱ��ͼ
	for (int i = 0; i < 25; i++)
	{
		for (int j = 1; j < 11; j++)
		{
			hist[i * 10 + j] = avgvalue[i];    //����ʮ��С���ӵ�ֱ��ͼ�����ǵ�ƽ��ֵ��ʾ
		}
	}
	for (int i = 1; i < 256; i++)
	{
		int value = hist[i];
		value = (int)(value * histpara);
		//cout << "value" << value << endl;
		line(drawImage, Point(i, drawImage.rows - 1), Point(i, drawImage.rows - 1 - value), Scalar(0, 0, 255));  //����֮��ľ���Ϊvalue
	}
	line(drawImage, Point(0, drawImage.rows - 1), Point(0, drawImage.rows - 1 - 0), Scalar(0, 0, 255));
	namedWindow("hist_zip");
	moveWindow("hist_zip", 700, 50);
	imshow("hist_zip", drawImage);
	return;
}
 

//�Ƚ�mImageDepthͼ�е�rect�����depth_growͼ�е�rect����
//����depth_growͼ�е�rect����Ϊ0�ĵ�Ҳ��mImageDepthͼ�е�rect������0
//imageROI�е���Ҳ������������������re�����Ӧ��ԭ���ͼ����
//imagedepthROI = capture.get3dface(imagedepthROI);
Mat Capture::get3dface(Mat imageroi)
{
	Mat mask;
	mask = getGrownImage();              //getGrownImage()��������ȡ��depth_growͼ�е�re��������������rect

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


//-------------------------һЩ��δʹ�õĺ���--------------------------
//����ֱ��ͼ
//������	imageGray �Ҷ�ͼ��
//������	isShow	-0 ������
//					-1 ����
//����ֵ��	�Ҷ�ֱ��ͼ����
/*
MatND Capture::myCalcHist(Mat imageGray, int isShow)
{
//����ֱ��ͼ
int channels = 0;
MatND dstHist;
int histSize[] = { 256 };
float midRanges[] = { 0, 256 };
const float *ranges[] = { midRanges };
calcHist(&imageGray, 1, &channels, Mat(), dstHist, 1, histSize, ranges, true, false);

if (isShow)
{
//����ֱ��ͼ,�����ȴ���һ���ڵ׵�ͼ��Ϊ�˿�����ʾ��ɫ�����Ըû���ͼ����һ��8λ��3ͨ��ͼ��
Mat drawImage = Mat::zeros(Size(257, 257), CV_8UC3);
//�κ�һ��ͼ���ĳ�����ص��ܸ����п��ܻ�ܶ࣬���������������ͼ��ĳߴ磬
//������Ҫ�ȶԸ������з�Χ�����ƣ���minMaxLoc�������õ�����ֱ��ͼ������ص�������
double g_dHistMaxValue;
Point maxLoc;
dstHist.at<float>(0) = 0;
minMaxLoc(dstHist, 0, &g_dHistMaxValue, &maxLoc, 0);
normalize(dstHist, dstHist, 0, 256, NORM_MINMAX, -1, Mat());
//�����صĸ������ϵ�ͼ������Χ��
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




//����ͶӰ��δ�ã�
//������	src ����Ҷ�ͼ��
//������	dst ���һάͳ������
//������	image �Ƿ���ʾͳ��ͼ
//���أ�	��
/*
Mat Capture::shadowLeft(Mat src, Mat* dst, int image)
{

Mat shadow_left = Mat::zeros(Size(src.rows, 1), CV_16U);
//����ͶӰ
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
//�����ȴ���һ���ڵ׵�ͼ��Ϊ�˿�����ʾ��ɫ�����Ըû���ͼ����һ��8λ��3ͨ��ͼ��
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
