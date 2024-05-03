#include <opencv2/opencv.hpp>

float getError(CvScalar c1, CvScalar c2)
{
	float sum = 0;
	for (int k = 0; k < 3; k++)
	{
		sum += (c1.val[k] - c2.val[k]) * (c1.val[k] - c2.val[k]);
	}
	return sqrt(sum);
}

float getAreaError(int x, int y, int grid, IplImage* canvas, IplImage* ref)
{
	float areaError = 0.f;
	int cnt = 0;
	for (int i = y - grid / 2; i < y + grid / 2; i++)
	{
		for (int j = x - grid / 2; j < x + grid / 2; j++)
		{
			if (i < 0 || j < 0 || i >= cvGetSize(canvas).height || j >= cvGetSize(canvas).width)
				continue;
			cnt++;
			areaError += getError(cvGet2D(canvas, i, j), cvGet2D(ref, i, j));
		}
	}
	if (cnt == 0)
		return 0;
	areaError /= cnt;
	return areaError;
}

CvPoint getLargestErrorPoint(int x, int y, int grid, IplImage* canvas, IplImage* ref)
{
	float maxError = 0;
	CvPoint maxErrPoint = cvPoint(x, y);
	for (int i = y - grid / 2; i < y + grid / 2; i++)
	{
		for (int j = x - grid / 2; j < x + grid / 2; j++)
		{
			if (i < 0 || j < 0 || i >= cvGetSize(canvas).height || j >= cvGetSize(canvas).width)
				continue;

			float err = getError(cvGet2D(canvas, i, j), cvGet2D(ref, i, j));
			if (err > maxError)
			{
				maxError = err;
				maxErrPoint = cvPoint(j, i);
			}
		}
	}
	return maxErrPoint;
}

void shuffle(CvPoint* arr, int size)
{
	srand(time(NULL));
	for (int i = 0; i < size - 1; i++)
	{
		int rn = rand() % (size - i) + i;
		CvPoint temp = arr[i];
		arr[i] = arr[rn];
		arr[rn] = temp;
	}
}

void paintLayer(IplImage* canvas, IplImage* ref, int R)
{
	CvSize size = cvGetSize(canvas);

	//create a new set of strokes, initially empty
	CvPoint* strokes = (CvPoint*)malloc(sizeof(CvPoint) * size.height * size.width);
	int back = -1;
	
	int fg = 1; //임시
	int grid = R * fg;

	for (int y = 0; y < size.height; y += grid)
	{
		for (int x = 0; x < size.width; x += grid)
		{
			//평균 에러 구하기
			float areaError = getAreaError(x, y, grid, canvas, ref);

			//붓질이 필요할 때만 칠하기
			int T = 50;
			if (areaError > T)
			{
				//find the largest error point
				CvPoint point = getLargestErrorPoint(x, y, grid, canvas, ref);
				
				//add strokes
				back++;
				strokes[back] = point;
			}
		}
	}

	//paint circle
	shuffle(strokes, back + 1);
	for (int i = 0; i <= back; i++)
	{
		CvPoint point = strokes[i];
		CvScalar color = cvGet2D(ref, point.y, point.x);
		cvDrawCircle(canvas, point, R, color, -1);
	}
}

IplImage* paint(IplImage* src, int R[5])
{
	//create a new constant color image
	IplImage* canvas = cvCreateImage(cvGetSize(src), 8, 3);
	cvSet(canvas, cvScalar(255, 255, 255));

	//paint the canvas
	for (int i = 0; i < 5; i++)
	{
		//apply Gaussian blur
		IplImage* ref = cvCreateImage(cvGetSize(src), 8, 3);
		int k = 1; //임시 (홀수일것)
		int G = R[i] * k;
		cvSmooth(src, ref, CV_GAUSSIAN, G);
		cvShowImage("ref", ref);

		//paint a layer
		paintLayer(canvas, ref, R[i]);
		cvShowImage("canvas", canvas);

		cvWaitKey();
	}

	return canvas;
}

int main()
{
	IplImage* src = cvLoadImage("c:\\TempImg\\giraffe.jpg");
	int R[5] = { 9,7,5,3,1 };
	
	//temp
	for (int i = 0; i < 5; i++)
		R[i] *= 3;

	IplImage* canvas = paint(src, R);
	cvShowImage("canvas", canvas);
	cvWaitKey();

	return 0;
}