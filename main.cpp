#include <stdio.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <system>
//#include <math.h>
//#include <complex>

using namespace std;
using namespace cv;

int x_prev = 0; int y_prev = 0; int A_prev = 0; int delta_a = 0;

///Click function
void mouse_click(int Area)
{
    delta_a = A_prev - Area;
    A_prev = Area;
    cout<<delta_a<<"\n";
    if(delta_a < -500)
    {
        system("xdotool click 1");
    }
}

///New mousmove function
void movemouse(Point &Centroid)
{

}

///Function that accepts the coordinates of the centroid and
///moves the mouse to a corresponding position
void mousemove(int x_pos, int y_pos)
{
    ///Strings that will contain the conversions
    string xcord; string ycord;

    ///These are buffers or something? I don't really know... lol.
    stringstream sstr; stringstream sstr2;

    ///Conversion to regular string happens here
    sstr<<5*x_pos;
    xcord = sstr.str();
    sstr2<<5*y_pos;
    ycord = sstr2.str();

    ///Getting the command string
    string command = "xdotool mousemove " + xcord + " " + ycord;

    ///Converting command string to a form that system() accepts.
    const char *com = command.c_str();
    system(com);
}

///This is the functions that gets the centroid of the thresholded image
void getCentroid(Mat &thresholded_image, Point &Centroid, int &Area)
{
    ///The object that holds all the centroids.
    ///Pass in the image. The boolean true tells the function that the image is binary
    Moments m = moments(thresholded_image, true);
    ///Moment along x axis
    double M10 = m.m10;
    ///Moment along y-axis;
    double M01 = m.m01;
    ///Area
    double M00 = m.m00;
    Centroid.x  = int(M10/M00);
    Centroid.y  = int(M01/M00);
    Area        = int(M00);
}

///HSV for ball: 81-105, 53-74,
void HSV_threshold(Mat &image, Mat &output_image_gray, int H_upper, int H_lower, int S_upper, int S_lower, int V_upper, int V_lower)
{
    Mat HSV;///Temporary Mat to store HSV


    ///Converting input image to HSV
    cvtColor(image, HSV, CV_RGB2HSV);
    //cvtColor(output_image_gray, output_image_gray, CV_RGB2GRAY);

    ///Processing each pixel and thresholding the image.
    int i, j;
        for(i=0; i<image.rows; i++)
        {
            for(j=0; j<image.cols; j++)
            {
                if((HSV.at<Vec3b>(i,j)[0] > H_lower)&&(HSV.at<Vec3b>(i,j)[0] < H_upper)&&(HSV.at<Vec3b>(i,j)[1]>S_lower)&&(HSV.at<Vec3b>(i,j)[1]<S_upper)&&(HSV.at<Vec3b>(i,j)[2]<V_upper)&&(HSV.at<Vec3b>(i,j)[2]>V_lower)) output_image_gray.at<uchar>(i,j) = 255;
                else output_image_gray.at<uchar>(i,j) = 0;
            }
        }
}

Mat Canny_Filter(Mat &gray_image, int lower_thresh,int upper_thresh)
{
    ///Mat object with edges that will be returned
    Mat edges;

    ///Reducing image noise with blur function
    blur(gray_image, edges, Size(3,3));

    ///Canny edge detection function
    Canny(edges, edges, lower_thresh, upper_thresh, 3);

    return edges;
}

///Cartoonify function
Mat cartoonify(Mat &gray_image)
{
    ///Declaration of Mat objects.
    Mat result; Mat edges;
    ///This variable defines the size of the kernel used for blur.
    const int BLUR_SIZE = 7;

    ///The blur function
    medianBlur(gray_image,result,BLUR_SIZE);

    ///Size of kernel matrix for the Laplacian
    const int LAPLACIAN_SIZE = 5;

    ///Function that carries out the laplacian
    Laplacian(result, edges, CV_8U, LAPLACIAN_SIZE);

    ///Final thresholded image
    Mat mask;
    const int THRESH = 80;

    ///Thresholding happens here.
    threshold(edges, mask, THRESH, 255, THRESH_BINARY_INV);

    ///Output the thresholded image
    return mask;
}

int main(int argc, char** argv)
{
    ///variable declarations
    int camera_number    = 0;   int Hue_lower_thresh = 61;
    int max_thresh       = 255; int Hue_upper_thresh = 91;
    int Sat_lower_thresh = 88;  int Sat_upper_thresh = 204;
    int Val_lower_thresh = 179; int Val_upper_thresh = 255;
    Mat camera_frame; Mat displayed_frame; Mat gray_frame; //Mat thresh_frame;
    Mat darkfield = imread("~/Documents/OpenCV/eyetrack/darkfield.png");
    //vector<Mat> bgr_planes;

    ///creating the output window and trackbars
    namedWindow("camfeed");
    //namedWindow("RED"); namedWindow("BLUE"); namedWindow("GREEN");
    createTrackbar("Hue lower", "camfeed", &Hue_lower_thresh, max_thresh, NULL);
    createTrackbar("Hue upper", "camfeed", &Hue_upper_thresh, max_thresh, NULL);
    createTrackbar("Sat upper", "camfeed", &Sat_upper_thresh, max_thresh, NULL);
    createTrackbar("Sat lower", "camfeed", &Sat_lower_thresh, max_thresh, NULL);
    createTrackbar("Val upper", "camfeed", &Val_upper_thresh, max_thresh, NULL);
    createTrackbar("Val lower", "camfeed", &Val_lower_thresh, max_thresh, NULL);

    ///Camera setup
    VideoCapture camera;
    camera.open(camera_number);
    if(! camera.isOpened())
    {
        cerr<<"ERROR: COULD NOT ACCESS THE CAMERA!"<<endl;
        exit(1);
    }

    ///Setting the camera resolution.
    ///Lower resuolution for easier processing.
    camera.set(CV_CAP_PROP_FRAME_WIDTH, 396);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, 216);

    ///Infinite loop which gets each frame from webcam and processes
    ///it. Shows the processing output as a video.
    while(true)
    {
        ///Getting the next frame from the camera
        camera >> camera_frame;
        if(camera_frame.empty())
        {
            cerr<<"ERROR: COULD NOT GRAB A FRAME!"<<endl;
            exit(1);
        }
        flip(camera_frame, camera_frame, 1);
        ///Declaring a thresholded image of same size as the camera frame but grayscale.
        Mat thresh_frame(Size(camera_frame.cols, camera_frame.rows), CV_8U);
        ///converting the image to grayscale
        ///Or any other colorspace conversions
        //cvtColor(camera_frame, gray_frame, CV_RGB2GRAY);
        //split(camera_frame, bgr_planes);

        ///Applying filters
        //displayed_frame = cartoonify(gray_frame);
        //displayed_frame = Canny_Filter(gray_frame, lower_thresh, upper_thresh);

        HSV_threshold(camera_frame, thresh_frame, Hue_upper_thresh, Hue_lower_thresh, Sat_upper_thresh, Sat_lower_thresh, Val_upper_thresh, Val_lower_thresh);
        medianBlur(thresh_frame, thresh_frame, 5); ///Low Pass filter to remove noise
        //
        //Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(2,2));
        //morphologyEx(thresh_frame, thresh_frame, MORPH_CLOSE, kernel);
        //vector<vector<Point> > contours;
        //findContours(thresh_frame, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
        //drawContours(camera_frame, contours, 2, Scalar(255), CV_FILLED);
        //*/
        Point Centroid; int Area;
        getCentroid(thresh_frame, Centroid, Area);

        ///Sending final processed image to display
        imshow("camfeed", camera_frame);
        //cout<<Area;
        ///I need a new mouse move function.
        if((Centroid.x<thresh_frame.cols)&&(Centroid.x>0)&&(Centroid.y>0)&&(Centroid.y<thresh_frame.rows))
        {
            if(Area > 100)
            {
                ///Comment out this function, then compile and run program for calibration
                ///For more details see README file.
                mousemove(Centroid.x, Centroid.y);
            }
        }
        mouse_click(Area);
        //imshow("BLUE", bgr_planes[0]);
        //imshow("GREEN", bgr_planes[1]);
        //imshow("RED", bgr_planes[2]);

        ///Listening for the user to press a key on the keyboard
        char keypress = waitKey(10);

        ///If user pressed 'c' stop program and save current frame to file
        if(keypress == 'c')
        {
            imwrite("capture.png", camera_frame);
            cout<<"Image captured"<<"Now exiting program";
            break;
        }
        if(keypress == 'a') cout<<Area<<"\n";
        ///If user pressed escape key stop program.
        if(keypress == 27)
        {
            break;
        }
        ///Getting darkfield images
        if(keypress == 'd')
        {
            imwrite("darkfield1.png", camera_frame);
            cout<<"Darkfield Image captured"<<"\n";
        }
    }

    return 0;
}
