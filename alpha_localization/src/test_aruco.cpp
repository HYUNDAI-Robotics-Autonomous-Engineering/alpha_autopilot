#include <ros/ros.h>

#include <opencv2/highgui.hpp>
#include <opencv2/aruco/charuco.hpp>

using namespace cv;
using namespace std;
int main(int argc, char *argv[]){

  ros::init(argc,argv,"aruco_test");
  ros::NodeHandle nh;
  int squares_x = 3;
  int squares_y = 3;

  float square_length = 2.0;
  float marker_length = 1.5;
  int dictionaryId = 1;

  namedWindow("detection_result");
  startWindowThread();
  namedWindow("board");
  startWindowThread();

  Ptr<aruco::Dictionary> dictionary = 
    aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME(dictionaryId));

  VideoCapture input;
  input.open(0);
  
  Ptr<aruco::CharucoBoard> charucoboard = aruco::CharucoBoard::create(squares_x,squares_y,square_length, marker_length, dictionary);
  Mat boardImg;
  charucoboard->draw(cv::Size(1000,1000),boardImg,10,1);
  imshow("board",boardImg);
  Ptr<aruco::Board> board = charucoboard.staticCast<aruco::Board>();

  Mat distCoeffs = (Mat_<float>(1,5) << 0.182276,-0.533582,0.000520,-0.001682,0.000000);
  Mat camMatrix  = (Mat_<float>(3,3) << 
		    743.023418,0.000000,329.117496,
		    0.000000,745.126083,235.748102,
		    0.000000,0.000000,1.000000);

  while(input.grab() && ros::ok()){
    Mat image;
    input.retrieve(image);
  
    vector<int> markerIds,charucoIds;
    vector<vector<Point2f> > markerCorners, rejectedMarkers;
    vector<Point2f> charucoCorners;
    Vec3d rvec, tvec;

    Ptr<aruco::DetectorParameters> detectorParams = aruco::DetectorParameters::create();

    aruco::detectMarkers(image, dictionary, markerCorners, markerIds, detectorParams,rejectedMarkers);

    int interpolatedCorners = 0;
    if(markerIds.size() > 0)
      interpolatedCorners =
	aruco::interpolateCornersCharuco(markerCorners, markerIds, image, 
					 charucoboard, charucoCorners, charucoIds, camMatrix, distCoeffs);

    bool validPose = false;

    if(camMatrix.total()!=0)
      validPose = aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds,charucoboard, camMatrix, distCoeffs, rvec, tvec);

    Mat result;

    image.copyTo(result);
  
    if(markerIds.size() > 0)
      aruco::drawDetectedMarkers(result, markerCorners);

    if(interpolatedCorners > 0){
      Scalar color(255,0,0);
      aruco::drawDetectedCornersCharuco(result, charucoCorners, charucoIds, color);
    }

    if(validPose)
      aruco::drawAxis(result, camMatrix, distCoeffs, rvec, tvec, 1.0f);
  
    imshow("detection_result",result);
  } 
  return 0;
}
