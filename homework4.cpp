#include <opencv2/opencv.hpp>

typedef struct vector
{
	CvPoint* points;
	int back;
	int size;
}Vector;
void initVector(Vector* vec, int size);
int pushVector(Vector* vec, CvPoint elem);

float getError(CvScalar c1, CvScalar c2);
float getMeanError(int x, int y, int grid, IplImage* canvas, IplImage* ref);
CvPoint getLargestErrorPoint(int x, int y, int grid, IplImage* canvas, IplImage* ref);
void shuffle(CvPoint* arr, int size);
float getDiff(CvScalar a, CvScalar b);
Vector makeSplineStroke(int x0, int y0, int R, IplImage* ref, IplImage* canvas);
void drawSplines(IplImage* canvas, Vector* strokes, int R, CvScalar strokeColor);
void paintLayer(IplImage* canvas, IplImage* ref, int R, int drawingMode);
IplImage* paint(IplImage* src, int R[5], int drawingMode);

int main()
{
	printf("=============================================\n");
	printf("Software Department, Sejong University\n");
	printf("Multimedia Programming Homework #4\n");
	printf("Painterly Rendering\n");
	printf("22011824 ¿Ã¡ˆ»£\n");
	printf("=============================================\n");
	
	char filePath[50]; //"c:\\TempImg\\giraffe.jpg"
	printf("Input File Path: ");
	scanf("%s", filePath);
	IplImage* src = cvLoadImage(filePath);
	while (src == nullptr)
	{
		printf("File Not Found!\n");
		printf("Input File Path: ");
		scanf("%s", filePath);
		src = cvLoadImage(filePath);
	}
	int drawingMode;
	printf("Select Drawing Mode (0=circle, 1=stroke): ");
	scanf("%d", &drawingMode);
	while (!(drawingMode == 0 || drawingMode == 1))
	{
		printf("Wrong Drawing Mode!\n");
		printf("Select Drawing Mode (0=circle, 1=stroke): ");
		scanf("%d", &drawingMode);
	}

	int R[5] = { 9,7,5,3,1 };
	for (int i = 0; i < 5; i++)
		R[i] *= 3;

	cvShowImage("src", src);
	IplImage* canvas = paint(src, R, drawingMode);
	cvWaitKey();

	return 0;
}

float getError(CvScalar c1, CvScalar c2)
{
	float sum = 0;
	for (int k = 0; k < 3; k++)
	{
		sum += (c1.val[k] - c2.val[k]) * (c1.val[k] - c2.val[k]);
	}
	return sqrt(sum);
}

float getMeanError(int x, int y, int grid, IplImage* canvas, IplImage* ref)
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

void initVector(Vector* vec, int size)
{
	vec->points = (CvPoint*)malloc(sizeof(CvPoint) * size);
	vec->back = -1;
	vec->size = size;
}

int pushVector(Vector* vec, CvPoint elem)
{
	if (vec->back + 1 == vec->size)
		return 0; //fail

	(vec->back)++;
	vec->points[vec->back] = elem;
	return 1; //success
}

Vector makeSplineStroke(int x0, int y0, int R, IplImage* ref, IplImage* canvas)
{
	CvScalar strokeColor = cvGet2D(ref, y0, x0);

	int maxStrokeLength = 4 * R;
	int minStrokeLength = 0.2 * R;

	//a new stroke with radius R and color strokeColor
	CvSize size = cvGetSize(ref);
	Vector strokes;
	initVector(&strokes, maxStrokeLength);

	//add point (x0,y0) to strokes
	pushVector(&strokes, cvPoint(x0, y0));

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
		float fc = 0.8;
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
		pushVector(&strokes, cvPoint(x, y));
	}

	return strokes;
}

void drawSplines(IplImage* canvas, Vector* strokes, int R, CvScalar strokeColor)
{
	for (int i = 0; i < strokes->back; i++)
	{
		cvDrawLine(canvas, strokes->points[i], strokes->points[i + 1], strokeColor, R);
	}
}

void paintLayer(IplImage* canvas, IplImage* ref, int R, int drawingMode)
{
	CvSize size = cvGetSize(canvas);

	//create a new set of strokes, initially empty
	Vector strokes;
	initVector(&strokes, size.height * size.width);

	int fg = 1;
	int grid = R * fg;

	for (int y = 0; y < size.height; y += grid)
	{
		for (int x = 0; x < size.width; x += grid)
		{
			//get mean error near (x,y)
			float areaError = getMeanError(x, y, grid, canvas, ref);

			//threshold
			int T = 50;
			if (areaError > T)
			{
				//find the largest error point
				CvPoint point = getLargestErrorPoint(x, y, grid, canvas, ref);

				//add strokes
				pushVector(&strokes, point);
			}
		}
	}

	//shuffle strokes' order
	shuffle(strokes.points, strokes.back + 1);

	//paint
	for (int i = 0; i <= strokes.back; i++)
	{
		CvPoint point = strokes.points[i];
		CvScalar color = cvGet2D(ref, point.y, point.x);

		if (drawingMode == 0) //draw circle
			cvDrawCircle(canvas, point, R, color, -1);
		else //draw curve
		{
			Vector controlPoints = makeSplineStroke(point.x, point.y, R, ref, canvas);
			drawSplines(canvas, &controlPoints, R, color);
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
		int k = 1;
		int G = R[i] * k;
		cvSmooth(src, ref, CV_GAUSSIAN, G);

		//paint a layer
		paintLayer(canvas, ref, R[i], drawingMode);
		cvShowImage("canvas", canvas);

		cvWaitKey(1000);
	}

	return canvas;
}