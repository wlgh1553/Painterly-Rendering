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

float getDiff(CvScalar a, CvScalar b)
{
	float sum = 0;
	for (int k = 0; k < 3; k++)
	{
		sum += a.val[k] - b.val[k];
	}
	return sum;
}

void makeSplineStroke(int x0, int y0, int R, IplImage* ref, IplImage* canvas)
{
	CvScalar strokeColor = cvGet2D(ref, y0, x0);

	int maxStrokeLength = 4 * R;
	int minStrokeLength = 0.2 * R;

	//a new stroke with radius R and color strokeColor
	CvSize size = cvGetSize(ref);
	CvPoint* strokes = (CvPoint*)malloc(sizeof(CvPoint) * maxStrokeLength);
	int back = -1;

	//add point (x0,y0) to strokes
	back++;
	strokes[back] = cvPoint(x0, y0);

	int x = x0, y = y0;
	float lastDx = 0, lastDy = 0;

	for (int i = 1; i < maxStrokeLength; i++)
	{
		if (i > minStrokeLength &&
			getError(cvGet2D(ref, y, x), cvGet2D(canvas, y, x))
			< getError(cvGet2D(ref, y, x), strokeColor))
			break;

		//gradient
		if (x - 1 < 0 || y - 1 < 0 || x + 1 >= size.width || y + 1 >= size.height)
			break;
		float gx = getDiff(cvGet2D(ref, y, x + 1), cvGet2D(ref, y, x - 1));
		float gy = getDiff(cvGet2D(ref, y + 1, x), cvGet2D(ref, y - 1, x));

		//detect vanishing gradient
		if (sqrt(gx * gx + gy * gy) == 0)
			break;
		
		//get unit vector of gradient
		float dx = -gy;
		float dy = gx;

		//if necessary, reverse direction
		if (lastDx * dx + lastDy * dy < 0)
		{
			dx *= -1; dy *= -1;
		}

		//filter the stroke direction
		float fc = 0.8; //이게 뭐노
		dx = fc * dx + (1 - fc) * lastDx;
		dy = fc * dy + (1 - fc) * lastDy;
		float norm = sqrt(dx * dx + dy * dy);
		dx /= norm;
		dy /= norm;

		//next pos
		x += R * dx;
		y += R * dy;
		if (x < 0 || y < 0 || x >= size.width || y >= size.height)
			break;

		//update last direction
		lastDx = dx;
		lastDy = dy;

		//add the point (x,y) to K
		back++;
		strokes[back] = cvPoint(x, y);
	}

	//draw strokes
	for (int i = 0; i < back; i++)
	{
		cvDrawLine(canvas, strokes[i], strokes[i + 1], strokeColor, R);
	}
	cvShowImage("canvas", canvas);
}

void paintLayer(IplImage* canvas, IplImage* ref, int R, int drawingMode)
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

	//shuffle strokes' order
	shuffle(strokes, back + 1);

	//paint circle
	if (drawingMode == 0)
	{
		for (int i = 0; i <= back; i++)
		{
			CvPoint point = strokes[i];
			CvScalar color = cvGet2D(ref, point.y, point.x);
			cvDrawCircle(canvas, point, R, color, -1);
		}
	}
	else
	{
		//paint spline
		for (int i = 0; i <= back; i++)
		{
			CvPoint point = strokes[i];
			makeSplineStroke(point.x, point.y, R, ref, canvas);
		}
	}
}

IplImage* paint(IplImage* src, int R[5], int drawingMode)
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
		paintLayer(canvas, ref, R[i], drawingMode);
		cvShowImage("canvas", canvas);

		cvWaitKey();
	}

	return canvas;
}

int main()
{
	printf("=============================================\n");
	printf("Software Department, Sejong University\n");
	printf("Multimedia Programming Homework #4\n");
	printf("Painterly Rendering\n");
	printf("22011824 이지호\n");
	printf("=============================================\n");
	
	char filePath[50]; //"c:\\TempImg\\giraffe.jpg"
	printf("Input File Path:");
	scanf("%s", filePath);
	IplImage* src = cvLoadImage(filePath);
	while (src == nullptr)
	{
		printf("File Not Found!\n");
		printf("Input File Path:");
		scanf("%s", filePath);
		src = cvLoadImage(filePath);
	}
	int drawingMode;
	printf("Select Drawing Mode (0=circle, 1=stroke):");
	scanf("%d", &drawingMode);
	while (!(drawingMode == 0 || drawingMode == 1))
	{
		printf("Wrong Drawing Mode!\n");
		printf("Select Drawing Mode (0=circle, 1=stroke):");
		scanf("%d", &drawingMode);
	}

	int R[5] = { 9,7,5,3,1 };
	for (int i = 0; i < 5; i++)
		R[i] *= 3;

	IplImage* canvas = paint(src, R, drawingMode);
	cvShowImage("canvas", canvas);
	cvWaitKey();

	return 0;
}