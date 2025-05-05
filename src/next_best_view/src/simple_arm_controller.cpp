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


// imports for depth sub node
#include <geometry_msgs/msg/point_stamped.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

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
  auto const node = std::make_shared<rclcpp::Node>("simple_arm_controller", rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true).parameter_overrides({{"use_sim_time", true}}));

  // create a ROS logger
  auto const logger = rclcpp::get_logger("simple_arm_controller");
  RCLCPP_INFO(logger, "starting simple arm controller");

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
      cv::imwrite("frame.jpg", frame);
      cv::imwrite("mask.jpg", mask);

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

  
  // Add shared state variables
  double latest_depth_value = 0.0;
  std::mutex depth_mutex;
  
  // Modified depth callback
  auto depth_callback = [&](const sensor_msgs::msg::Image::ConstSharedPtr msg) {
    try {
      cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::TYPE_32FC1);
      
      {
        std::lock_guard<std::mutex> lock(depth_mutex);
        latest_depth_value = cv_ptr->image.at<float>(cv_ptr->image.rows/2, cv_ptr->image.cols/2);
      }
      
      RCLCPP_INFO(logger, "Updated depth: %.3f meters", latest_depth_value);
        cv::Mat latest_depth = cv_ptr->image;
        
        // Calculate center coordinates
        int center_x = latest_depth.cols / 2;
        int center_y = latest_depth.rows / 2;
        
        // Check for valid image dimensions
        if (latest_depth.empty()) {
            RCLCPP_ERROR(logger, "Empty depth image received");
            return;
        }
        
        // Get depth value at center (32FC1 format uses .at<float>)
        float depth_value = latest_depth.at<float>(center_y, center_x);
        RCLCPP_INFO(logger, "Center depth: %.3f meters", depth_value);
        latest_depth_value = depth_value;
        cv::imwrite("depth.jpg", latest_depth);

    } catch (const cv_bridge::Exception& e) {
        RCLCPP_ERROR(logger, "Depth CV error: %s", e.what());
    }
  };

  auto image_sub = node->create_subscription<sensor_msgs::msg::Image>("/camera/color/image_raw", 10, image_callback);
  auto depth_sub = node->create_subscription<sensor_msgs::msg::Image>("/camera/depth/image_raw", 10, depth_callback);

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

    // Create gripper interface
    auto gripper_interface = MoveGroupInterface(node, "gripper");

    // Store initial position
    geometry_msgs::msg::Pose initial_pose;
    try {
      initial_pose = move_group_interface.getCurrentPose().pose;
      
      // Get latest depth value from callback
      float depth_value = 20;
      {
        std::lock_guard<std::mutex> lock(depth_mutex);
        depth_value = latest_depth_value;
      }
      
      // Calculate forward movement with safety margin
      float forward_distance = depth_value * 0.9;
      auto target_pose = initial_pose;
      target_pose.position.z -= forward_distance;
  
      // Execute forward movement
      move_group_interface.setPoseTarget(target_pose);
      move_group_interface.move();
  
      // Close gripper
      gripper_interface.setJointValueTarget("robotiq_85_left_knuckle_joint", 0.8);
      gripper_interface.move();
  
      // Return to initial position
      move_group_interface.setPoseTarget(initial_pose);
      move_group_interface.move();
  
    } catch (const std::exception& e) {
      RCLCPP_ERROR(logger, "Movement failed: %s", e.what());
    }

  // shutdown ROS
  rclcpp::shutdown();
  executor_thread.join(); // important wait for thread to finish
  return 0;
}

  