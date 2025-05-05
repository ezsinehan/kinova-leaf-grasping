// MoveIt’s MoveGroupInterface: high level api to control robo arms
#include <moveit/move_group_interface/move_group_interface.h>

// C++ header for smart pointers
#include <memory>

// C++ ROS 2 client library
#include <rclcpp/rclcpp.hpp>

// add chrono for sleep?? lol idk why
#include <chrono>
#include <thread>

// include geometry_msgs for Pose
#include <geometry_msgs/msg/pose.hpp>


// imports for camera subscriber node
#include <sensor_msgs/msg/image.hpp> //access to the ROS 2 Image message type.
#include <cv_bridge/cv_bridge.h> //bridge between ROS and OpenCV
#include <opencv2/opencv.hpp> // OpenCV — the computer vision library.

// function to check if pose is valid
bool isPoseNonZero(const geometry_msgs::msg::Pose& pose) {
  const double epsilon = 1e-6; //small threshold to check for "almost zero"

  return (std::abs(pose.position.x) > epsilon || 
          std::abs(pose.position.y) > epsilon || 
          std::abs(pose.position.z) > epsilon ||
          std::abs(pose.orientation.x) > epsilon ||
          std::abs(pose.orientation.y) > epsilon ||
          std::abs(pose.orientation.z) > epsilon ||
          std::abs(pose.orientation.w - 1.0) > epsilon);
}


int main(int argc, char* argv[]) {
  // init ROS and create the node
  rclcpp::init(argc, argv);
  // creating node(as shared ptr) w/ option indicating how we will handled parameters when passed with overides
  // when loading a parameter from cli or launch file that wasn't explictly declared, your node will crash or ignore it
  // basically allows you to take in parameters from cli or launch files
  auto const node = std::make_shared<rclcpp::Node>("simple_vision_controller", rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true));

  // create a ROS logger
  auto const logger = rclcpp::get_logger("simple_vision_controller");
  RCLCPP_INFO(logger, "starting simple vision controller");

  // critfix! -> created a dedicated thread to handle ros2 callbacks
  // 1. creating ros2 event processor
  rclcpp::executors::SingleThreadedExecutor executor;
  // 2. connect node to this processor
  executor.add_node(node);
  // 3. launch background worker
  std::thread executor_thread([&executor]() {
    executor.spin(); // this keeps node alive
    // continuously processing -> joint state updates, service responses, other ros2 msgs
  });


  // now creating the moveit movegroup interface
  using moveit::planning_interface::MoveGroupInterface;
  auto move_group_interface = MoveGroupInterface(node, "manipulator"); // manipulator is the planning group name used in your SRDF(Semantic Robot Description Format)
  // planning group is named collection of joints in your robot

  // sudo apt-get install -y ros-humble-vision-opencv
  auto image_callback = [&](const sensor_msgs::msg::Image::ConstSharedPtr msg) {
    try {
      cv::Mat frame = cv_bridge::toCvCopy(msg, "bgr8")->image; // blue green red 9 unsigned ints per channel
      // green is mixed with blue/red so no clean threshold for color iso bgr is nono
      RCLCPP_INFO(logger, "recieved %dx%d image", frame.cols, frame.rows);

      // basic green detection
      cv::Mat hsv; // hsv is a color space transformation
      cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
      cv::Mat mask;
      // for every pixel in hsv, if h, s, v values in that range means green so make it white, otherwise black

      // hue from 35 to 85 = green tones
      // saturation from 50 to 255 = moderately strong to full color
      // value from 50 to 255 = moderately bright to full brightness
      // avoids low sat(greys) n low brightness(shadows)
      cv::inRange(hsv, cv::Scalar(35, 50, 50), cv::Scalar(85, 255, 255), mask); 

      // add debug visualization
      cv::imshow("Camera Feed", frame);
      cv::imshow("Green Mask", mask);
      cv::waitKey(1);

      // finding centroid of green areas (region analysis)
      cv::Moments m = cv::moments(mask, true); // statistical properity of shapes in images
      // used to describe position, area, orientation, etc. of white blobs in a binary image


      // m00 area (sum of all white pixels)
      // m10 weighted sum of x-coordinates
      // m01 weighted sum of y-coordinates
      if (m.m00 > 0) { // making sure there is green
        cv::Point center(m.m10/m.m00, m.m01/m.m00);
        RCLCPP_INFO(logger, "green detected at: %d,%d", center.x, center.y);
      }
    } catch (const cv_bridge::Exception& e) {
      RCLCPP_ERROR(logger, "CV processing error: %s", e.what());
    }
  };

  auto image_sub = node->create_subscription<sensor_msgs::msg::Image>("/wrist_mounted_camera/image", 10, image_callback);


  // get the current pose (useful for reference)
  try {
    auto current_pose = move_group_interface.getCurrentPose().pose; 

    if (isPoseNonZero(current_pose)) {
      RCLCPP_INFO(logger, "current position: x=%.2f, y=%.2f, z=%.2f", current_pose.position.x, current_pose.position.y, current_pose.position.z);
    } else {
      RCLCPP_ERROR(logger, "recieved invalid pose (all zeros)");
      RCLCPP_INFO(logger, "make sure the robot or simulation is running and publishing joint states");
      RCLCPP_INFO(logger, "try running 'make gazebo' and 'make moveit' in separate terminals first");
    }
  } catch (const std::exception& e) {
    RCLCPP_ERROR(logger, "failed to get current pose: %s", e.what());
    RCLCPP_INFO(logger, "make sure the robot or simulation is running and publishing joint states");
  }

  // shutdown ROS
  rclcpp::shutdown();
  executor_thread.join(); // important wait for thread to finish
  return 0;
}