// The "Square Detector" program.
// It loads several images sequentially and tries to find squares in
// each image

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <math.h>
#include <string.h>

using namespace cv;
using namespace std;

static void help()
{
    cout <<
    "\nA program using pyramid scaling, Canny, contours, contour simpification and\n"
    "memory storage (it's got it all folks) to find\n"
    "squares in a list of images pic1-6.png\n"
    "Returns sequence of squares detected on the image.\n"
    "the sequence is stored in the specified memory storage\n"
    "Call:\n"
    "./squares\n"
    "Using OpenCV version " << CV_VERSION << "\n" << endl;
}


int thresh = 50, N = 11;
const char* wndname = "Square Detection Demo";
const char* fltname = "Square Detection Demo - Gray";

static void applyFilters(const Mat& src, vector<Mat>& filters) {
    assert(src.type() == CV_8UC3);

    filters.clear();
    
    Mat hsv;
    cvtColor(src,hsv,CV_BGR2HSV);

    Mat filters0;
    Mat filters1;
    Mat filters2;
    inRange(hsv, Scalar( 90,  32,  0), Scalar(100, 192, 255), filters0); // Detects Cyan
    inRange(hsv, Scalar( 30,  64,  0), Scalar( 50, 192, 255), filters1); // Detects Yellow
    inRange(hsv, Scalar(160, 127,  0), Scalar(180, 255, 255), filters2); // Detects Pink
    filters.push_back(filters0);
    filters.push_back(filters1);
    filters.push_back(filters2);
   
}

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
static double angle( Point pt1, Point pt2, Point pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
static void findSquares( const Mat& image, vector<vector<Point> >& squares )
{
    squares.clear();

    Mat pyr, timg, gray0(image.size(), CV_8U), gray;

    // down-scale and upscale the image to filter out the noise
    pyrDown(image, pyr, Size(image.cols/2, image.rows/2));
    pyrUp(pyr, timg, image.size());
    vector<vector<Point>> contours;


    // find squares in every color plane of the image
    for( int c = 0; c < 3; c++ )
    {
//        int ch[] = {c, 0};
//        mixChannels(&timg, 1, &gray0, 1, ch, 1);
//
//        // try several threshold levels
//        for( int l = 0; l < N; l++ )
//        {
//            // hack: use Canny instead of zero threshold level.
//            // Canny helps to catch squares with gradient shading
//            if( l == 0 )
//            {
//                // apply Canny. Take the upper threshold from slider
//                // and set the lower to 0 (which forces edges merging)
//                Canny(gray0, gray, 0, thresh, 5);
//                // dilate canny output to remove potential
//                // holes between edge segments
//                dilate(gray, gray, Mat(), Point(-1,-1));
//            }
//            else
//            {
//                // apply threshold if l!=0:
//                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
//                gray = gray0 >= (l+1)*255/N;
//            }

            Mat gray;
            dilate(image, gray, Mat(), Point(-1,-1));

            // find contours and store them all as a list
            findContours(gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

            vector<Point> approx;

            // test each contour
            for( size_t i = 0; i < contours.size(); i++ )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( approx.size() == 4 &&
                    fabs(contourArea(Mat(approx))) > 1000 &&
                    isContourConvex(Mat(approx)) )
                {
                    double maxCosine = 0;

                    for( int j = 2; j < 5; j++ )
                    {
                        // find the maximum cosine of the angle between joint edges
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if( maxCosine < 0.3 )
                        squares.push_back(approx);
                }
            }
        //}
    }
}


// the function draws all the squares in the image
static void drawSquares( Mat& image, const vector<vector<Point>>& squares, Scalar color)
{
    for( size_t i = 0; i < squares.size(); i++ )
    {
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        polylines(image, &p, &n, 1, true, color, 3, LINE_AA);
    }

}

static void findCenters( const vector<vector<Point>>& squares, vector<Point>& centers) {
  centers.clear();

  for( size_t i = 0; i < squares.size(); i++) {
    Point c = (squares[i][0] + squares[i][2])/ 2;
    centers.push_back(c);
  }
}

static void drawCenters( Mat& image, const vector<Point>& centers, Scalar color) {
    for( size_t i = 0; i < centers.size(); i++ )
    {
        Point p = centers[i];
        circle(image, centers[i], 5, color, 3);
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    vector<Scalar> color;
    color.push_back(Scalar(255,0,0));
    color.push_back(Scalar(0,255,0));
    color.push_back(Scalar(0,0,255));
    help();
    vector<Mat> filters;
    vector<vector<Point>> squares;
    vector<Point> centers;
    VideoCapture stream(1);   //0 is the id of video device.0 if you have only one camera.
 
    if (!stream.isOpened()) { //check if video device has been initialised
      cout << "cannot open camera";
    }
    while(true)
    {
        
        Mat camImage;
        stream.read(camImage);
        if( camImage.empty() )
        {
            cout << "Couldn't load image" << endl;
            continue;
        }
        Mat image = camImage.clone();

        applyFilters(image, filters);
        for( size_t c = 0; c < filters.size(); c++ ) {
          findSquares(filters[c], squares);
          findCenters(squares, centers);
          drawSquares(image, squares, color[c]);
          drawCenters(image, centers, color[c]);
        }
        Mat filtered;
        merge(filters, filtered);
        imshow(wndname, image);
        imshow(fltname, filtered);
        
        char key = (char)waitKey(10);
        if( key == 27 ) {
            break;
        } else if(key == 'w') {
            imwrite("camera.png", camImage);
            imwrite("squares.png", image);
            imwrite("filters.png", filtered);
        }
    }

    return 0;
}
