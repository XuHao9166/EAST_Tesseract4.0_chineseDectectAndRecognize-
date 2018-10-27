#include <baseapi.h>
// #include <allheaders.h>
//#include "sys/time.h"
#include <string>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/core/core.hpp>
#include "putText.h"


using namespace cv;
using namespace std;
using namespace cv::dnn;



void decode(const Mat& scores, const Mat& geometry, float scoreThresh,
	std::vector<RotatedRect>& detections, std::vector<float>& confidences);

string UTF8ToGB(const char* str);
wchar_t * Utf_8ToUnicode(const char* szU8);
char* UnicodeToAnsi(const wchar_t* szStr);

int main(int argc, char** argv)
{


	float confThreshold = 0.5;    ///���Ŷ���ֵ
	float nmsThreshold = 0.4;  ///�����������ֵ
	int inpWidth = 320;   ///ͨ��������С���ض������Ԥ��������ͼ����Ӧ����32�ı���
	int inpHeight = 320;  ///ͨ��������С���ض��߶���Ԥ��������ͼ����Ӧ����32�ı���
	String model = "frozen_east_text_detection.pb";
	const char *text = NULL;
	CV_Assert(!model.empty());

	// ��������ģ��
	Net net = readNet(model);

/////////////////OCR
// ��ʼ�� tesseract OCR 
	tesseract::TessBaseAPI *myOCR =
		new tesseract::TessBaseAPI();

	printf("Tesseract-ocr version: %s\n",
		myOCR->Version());  //���Tesseract-ocr �汾

	const char* datapath = "D:\\opencvʵ��\\tesseract_4.0\\tessdata";  //�����ֿ�ģ�͵ĵ��õ�ַ
	if (myOCR->Init(datapath, "chi_sim", tesseract::OEM_TESSERACT_ONLY))  //�������ļ��ģ��
	{
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}

	tesseract::PageSegMode pagesegmode = static_cast<tesseract::PageSegMode>(7); // treat the image as a single text line
	myOCR->SetPageSegMode(pagesegmode);


	//������ʾ����
	int fontFace = FONT_HERSHEY_PLAIN;
	double fontScale = 2;
	int thickness = 2;
/////////////////////////////////////////

	VideoCapture cap;

	cap.open(0);

	static const std::string kWinName = "EAST�ı����";
	//namedWindow(kWinName, WINDOW_NORMAL);

	std::vector<Mat> outs;
	std::vector<String> outNames(2);
	outNames[0] = "feature_fusion/Conv_7/Sigmoid";
	outNames[1] = "feature_fusion/concat_3";

	Mat frame, blob,frameclone;
	
	while (waitKey(1) < 0)
	{
		
		cap >> frame;
		if (frame.empty())
		{
			waitKey();
			break;
		}
		frameclone = frame.clone();
		blobFromImage(frame, blob, 1.0, Size(inpWidth, inpHeight), Scalar(123.68, 116.78, 103.94), true, false);
		net.setInput(blob);
		net.forward(outs, outNames);

		Mat scores = outs[0];
		Mat geometry = outs[1];

		// ����Ԥ��������Ŷ�
		std::vector<RotatedRect> boxes;
		std::vector<float> confidences;
		decode(scores, geometry, confThreshold, boxes, confidences);

		// Ӧ�÷�������Ƴ���
		std::vector<int> indices;
		NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

		Mat image;
		cvtColor(frameclone, image, CV_BGR2GRAY);

		// ��Ⱦ���
		//ratio�ǻ�ȡԭͼ���ı����ͼ�ڳ��Ϳ��ϵ����ű���
		Point2f ratio((float)frame.cols / inpWidth, (float)frame.rows / inpHeight);
		for (size_t i = 0; i < indices.size(); ++i)
		{
			RotatedRect& box = boxes[indices[i]];

			Point2f vertices[4];
			box.points(vertices);
			for (int j = 0; j < 4; ++j)
			{
				vertices[j].x *= ratio.x;
				vertices[j].y *= ratio.y;
			}
			//��ԭͼ�ϻ��Ƽ���,
			//�����ŵ�˳�� 1 ------ 2
			//                   |        |
			//                   0 ------ 3
			for (int j = 0; j < 4; ++j)
				line(frame, vertices[j], vertices[(j + 1) % 4], Scalar(0, 255, 0), 1);

	///////////////�Լ����е�������Tesseract-ocr ����ʶ��

		   // ʶ���ı�
			int Width = (vertices[2].x - vertices[1].x)*1.1;
			int Height = (vertices[0].y - vertices[1].y)*1.1;
			myOCR->TesseractRect(image.data, 1, image.step1(), vertices[1].x, vertices[1].y, Width, Height);
			text = myOCR->GetUTF8Text();
		
			wchar_t *tempchar;
			const char * resulttemp;
			tempchar = Utf_8ToUnicode(text);
			resulttemp = UnicodeToAnsi(tempchar);
			


			/*string ss;
			ss = UTF8ToGB(text);

			printf("text: \n");
			printf(ss.c_str());
			printf("\n");
*/

			
		    //putText ֻ����ͼƬ��ʾӢ��
			//putText(frame, ss, Point(vertices[1].x, vertices[1].y), fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
		
		 //��ͼƬ��ʾ����
			putTextZH(frame, resulttemp, Point(vertices[1].x-10, vertices[1].y-10), Scalar(0, 255, 0), 10, "΢���ź�", true, true);
	
			delete[] tempchar;
			delete[] resulttemp;
		}

		// ��ʾ֡��.
		std::vector<double> layersTimes;
		double freq = getTickFrequency() / 1000;
		double t = net.getPerfProfile(layersTimes) / freq;
		/*std::string label = format("Inference time: %.2f ms", t);
		putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));*/

		Mat frameclone;
		resize(frame, frameclone, Size(frame.cols * 2, frame.rows * 2));
		imshow(kWinName, frameclone);
	}

	delete[] text;
	
	myOCR->Clear();
	myOCR->End();
	return 0;
}


////Ԥ��������Ŷ�
void decode(const Mat& scores, const Mat& geometry, float scoreThresh,
	std::vector<RotatedRect>& detections, std::vector<float>& confidences)
{
	detections.clear();
	CV_Assert(scores.dims == 4); CV_Assert(geometry.dims == 4); CV_Assert(scores.size[0] == 1);
	CV_Assert(geometry.size[0] == 1); CV_Assert(scores.size[1] == 1); CV_Assert(geometry.size[1] == 5);
	CV_Assert(scores.size[2] == geometry.size[2]); CV_Assert(scores.size[3] == geometry.size[3]);

	const int height = scores.size[2];
	const int width = scores.size[3];
	for (int y = 0; y < height; ++y)
	{
		const float* scoresData = scores.ptr<float>(0, 0, y);
		const float* x0_data = geometry.ptr<float>(0, 0, y);
		const float* x1_data = geometry.ptr<float>(0, 1, y);
		const float* x2_data = geometry.ptr<float>(0, 2, y);
		const float* x3_data = geometry.ptr<float>(0, 3, y);
		const float* anglesData = geometry.ptr<float>(0, 4, y);
		for (int x = 0; x < width; ++x)
		{
			float score = scoresData[x];
			if (score < scoreThresh)
				continue;

			// ����Ԥ��.
			// ���4����ΪҪ��ͼ������ͼ����4��.
			float offsetX = x * 4.0f, offsetY = y * 4.0f;
			float angle = anglesData[x];
			float cosA = std::cos(angle);
			float sinA = std::sin(angle);
			float h = x0_data[x] + x2_data[x];
			float w = x1_data[x] + x3_data[x];

			Point2f offset(offsetX + cosA * x1_data[x] + sinA * x2_data[x],
				offsetY - sinA * x1_data[x] + cosA * x2_data[x]);
			Point2f p1 = Point2f(-sinA * h, -cosA * h) + offset;
			Point2f p3 = Point2f(-cosA * w, sinA * w) + offset;
			RotatedRect r(0.5f * (p1 + p3), Size2f(w, h), -angle * 180.0f / (float)CV_PI);
			detections.push_back(r);
			confidences.push_back(score);
		}
	}
}


string UTF8ToGB(const char* str)
{
	string result;
	WCHAR *strSrc;
	//TCHAR *szRes;
	LPSTR szRes;

	//�����ʱ�����Ĵ�С
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//�����ʱ�����Ĵ�С
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
}

//utf-8תunicode
wchar_t * Utf_8ToUnicode(const char* szU8)
{
	//UTF8 to Unicode
	//��������ֱ�Ӹ��ƹ���������룬��������ʱ�ᱨ���ʲ���16������ʽ

	//Ԥת�����õ�����ռ�Ĵ�С
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);
	//����ռ�Ҫ��'\0'�����ռ䣬MultiByteToWideChar�����'\0'�ռ�
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	//ת��
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);
	//������'\0'
	wszString[wcsLen] = '\0';
	return wszString;
}

//�����ֽ�wchar_t*ת��Ϊ���ֽ�char*  
char* UnicodeToAnsi(const wchar_t* szStr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];

	WideCharToMultiByte(CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL);

	return pResult;

}




//int main(int argc, char* argv[]) {
//
//	// ��ʼ�� tesseract OCR 
//	tesseract::TessBaseAPI *myOCR =
//		new tesseract::TessBaseAPI();
//
//	printf("Tesseract-ocr version: %s\n",
//		myOCR->Version());  //���Tesseract-ocr �汾
//	// printf("Leptonica version: %s\n",
//	//        getLeptonicaVersion());
//
//	const char* datapath = "D:\\opencvʵ��\\tesseract_4.0\\tessdata";  //�����ֿ�ģ�͵ĵ��õ�ַ
//	if (myOCR->Init(datapath, "eng")) {
//		fprintf(stderr, "Could not initialize tesseract.\n");
//		exit(1);
//	}
//
//	tesseract::PageSegMode pagesegmode = static_cast<tesseract::PageSegMode>(7); // treat the image as a single text line
//	myOCR->SetPageSegMode(pagesegmode);
//
//	// read iamge
//	namedWindow("tesseract-opencv", 0);
//	Mat image = imread("testCarID.jpg", 0);
//
//	// set region of interest (ROI), i.e. regions that contain text
//	Rect text1ROI(80, 50, 800, 110);
//	Rect text2ROI(190, 200, 550, 50);
//
//	// recognize text
//	myOCR->TesseractRect(image.data, 1, image.step1(), text1ROI.x, text1ROI.y, text1ROI.width, text1ROI.height);
//	const char *text1 = myOCR->GetUTF8Text();
//
//	myOCR->TesseractRect(image.data, 1, image.step1(), text2ROI.x, text2ROI.y, text2ROI.width, text2ROI.height);
//	const char *text2 = myOCR->GetUTF8Text();
//
//	// remove "newline"
//	string t1(text1);
//	t1.erase(std::remove(t1.begin(), t1.end(), '\n'), t1.end());
//
//	string t2(text2);
//	t2.erase(std::remove(t2.begin(), t2.end(), '\n'), t2.end());
//
//	// print found text
//	printf("found text1: \n");
//	printf(t1.c_str());
//	printf("\n");
//
//	printf("found text2: \n");
//	printf(t2.c_str());
//	printf("\n");
//
//	// draw text on original image
//	Mat scratch = imread("sample.png");
//
//	int fontFace = FONT_HERSHEY_PLAIN;
//	double fontScale = 2;
//	int thickness = 2;
//	putText(scratch, t1, Point(text1ROI.x, text1ROI.y), fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
//	putText(scratch, t2, Point(text2ROI.x, text2ROI.y), fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
//
//	rectangle(scratch, text1ROI, Scalar(0, 0, 255), 2, 8, 0);
//	rectangle(scratch, text2ROI, Scalar(0, 0, 255), 2, 8, 0);
//
//	imshow("tesseract-opencv", scratch);
//	waitKey(0);
//
//	delete[] text1;
//	delete[] text2;
//
//	// destroy tesseract OCR engine
//	myOCR->Clear();
//	myOCR->End();
//
//	return 0;
//}