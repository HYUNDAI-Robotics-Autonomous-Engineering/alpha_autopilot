#ifndef ALPHA_AUTOPILOT_H
#define ALPHA_AUTOPILOT_H
#include <alpha_autopilot/PID.h>
#include <alpha_autopilot/Types.h>
#include <alpha_msgs/FilteredState.h>
#include <alpha_msgs/RC.h>
#include <alpha_autopilot/AlphaMode.h>
#include <std_msgs/Empty.h>
#include <ros/ros.h>
#include <vector>
#include <nodelet/nodelet.h>
namespace alpha_autopilot{
  class AutoPilot : public nodelet::Nodelet{
  private:
    PID pid_roll;
    PID pid_pitch;
    PID pid_z;
  
    std::vector<int> trim;
    std::vector<int> rc_in;
  
    ros::Subscriber rc_sub;
    ros::Publisher rc_pub;
    ros::Publisher calibrate_pub;
    ros::Subscriber state_sub;
  
    AlphaState state;

  public:
    AutoPilot();
    ~AutoPilot();  
    virtual void onInit();
    AlphaMode* current_mode;
    void update();
  private:
    void rcInputCB(alpha_msgs::RCConstPtr msg);
    void publishRC(std::vector<int> &rc_out);
    void stateCB(alpha_msgs::FilteredStateConstPtr msg);
    std::vector<int> compute_auto_rc_out(float roll_effort, float pitch_effort, float throttle);
    std::vector<int> compute_manual_rc_out(std::vector<int> rc_in);
    void send_calibrate_request();

  };
}
#endif //ALPHA_AUTOPILOT_H
