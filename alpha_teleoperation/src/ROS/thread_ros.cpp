#include "thread_ros.h"
#include <alpha_msgs/RC.h>

Thread_ROS::Thread_ROS(Shared_Memory* share_memory)
{
    this->share_memory = share_memory;

    ros::NodeHandle n;
    rc_override_pub = n.advertise< alpha_msgs::RC >("/rc", 10);

    std::vector<int> rc_maxlimits;
    std::vector<int> rc_minlimits;


    for(int i = 0; i < 4; i++){
//        std::cout << RC_Param("_MAX", i) << std::endl;
      rc_maxlimits.push_back(1900);
      //        rc_maxlimtis.push_back(RC_Param(std::string("_MAX"), i));
    }
    for(int i = 0; i < 4; i++){
//        std::cout << RC_Param("_MIN", i) << std::endl;
//        rc_minlimtis.push_back(RC_Param(std::string("_MIN"), i));
      rc_minlimits.push_back(1100);
    }
    share_memory->setRC_maxlimits(rc_maxlimits);
    share_memory->setRC_minlimits(rc_minlimits);
//    for(int i = 0; i < 4; i++){
//        std::cout << RC_Param("_TRIM", i) << std::endl;
//    }
    stop = false;
}

Thread_ROS::~Thread_ROS()
{
    stop = true;
    alpha_msgs::RC msg;

    for(int i = 0; i < 8; i++){
      msg.Channel.push_back(i);
    }
    rc_override_pub.publish(msg);

    rc_override_pub.shutdown();

    ros::shutdown();

    std::cout << "~Thread_ROS" << std::endl;
}

int Thread_ROS::RC_Param(std::string s, int i)
{
  /*    mavros_msgs::ParamGet srv;
    std::stringstream ss;
    ss << (i+1);
    std::string param = std::string("RC") + ss.str() + s;
    srv.request.param_id = param;
    if (cl_param.call(srv)) {
        ROS_INFO("Send OK %d Value: %ld", srv.response.success, srv.response.value.integer);
        if (srv.response.success)
            return srv.response.value.integer;
        return -1;
    }

    ROS_ERROR("Failed RC_PARAM");
    return -1;
  */
}

void Thread_ROS::updateMode()
{
  /*    if(share_memory->getMode()!=share_memory->getModeChange() && !share_memory->getModeChange().empty()){
        mavros_msgs::SetMode srv;
        srv.request.base_mode = 0;
        srv.request.custom_mode = share_memory->getModeChange();
        if (cl_mode.call(srv)) {
            ROS_INFO("Send OK %d Value:", srv.response.success);
        } else {
            ROS_ERROR("Failed SetMode");
            return;
        }
	}*/
}

void Thread_ROS::run()
{
    struct timeval a, b;
    long totalb, totala;
    long diff;

    int cycle_control = 20;

    while(!stop){

        gettimeofday(&a, NULL);
        totala = a.tv_sec * 1000000 + a.tv_usec;

	//        updateMode();

        alpha_msgs::RC msg_override;

        if(share_memory->getOverride()){
	  msg_override.Channel.push_back(share_memory->getRoll());
	  msg_override.Channel.push_back(share_memory->getPitch());
	  msg_override.Channel.push_back(share_memory->getThrottle());
	  msg_override.Channel.push_back(share_memory->getYaw());
	  msg_override.Channel.push_back(1100);
	  msg_override.Channel.push_back(1100);
	  msg_override.Channel.push_back(1100);
	  msg_override.Channel.push_back(1100);
        }else{
            for(int i = 0; i < 8; i++){
	      msg_override.Channel.push_back(1100);
            }
        }
	for(int i = 0; i < 8; i++){
	  msg_override.Channel.push_back(1100);
	}

        if(!stop){
            rc_override_pub.publish(msg_override);
        }
        ros::spinOnce();

        gettimeofday(&b, NULL);
        totalb = b.tv_sec * 1000000 + b.tv_usec;
        diff = (totalb - totala) / 1000;

        if (diff < 0 || diff > cycle_control)
            diff = cycle_control;
        else
            diff = cycle_control - diff;

        usleep(diff * 1000);
    }
}
