#include <alpha_autopilot/AutoPilot.h>
#include <wiringPi.h>
#include <alpha_drivers/PCA9685.h>

int computeDropPulse(int drop_state){
  int drop_pulse;

  if(drop_state == 0)
    drop_pulse = 2094;
  else if(drop_state == 1 || drop_state == 5)
    drop_pulse = 1873;
  else if(drop_state == 2 || drop_state == 4)
    drop_pulse = 1577;
  else if(drop_state == 3)
    drop_pulse = 1200;

  return drop_pulse;
}
AutoPilot::AutoPilot() : pid_roll("roll"),pid_pitch("pitch"),pid_yaw("yaw"),pid_z("z"),pid_throttle("throttle"),trim(8,0),rc_in(8,0){
  current_mode = new ManualMode;
  drop_state = 0;
  trim[DROP_CH] = 2094;
  trim[ELEVATOR_CH] = 1500;
  trim[THROTTLE_CH] = 1100;
  trim[RUDDER_CH] = 1500;

  wiringPiSetupGpio();

  pinMode(27,OUTPUT);
  digitalWrite(27,0);
  
  pwm.init();
  pwm.reset();
  usleep(100000);
  pwm.setPWMFreq(60);
  
}
AutoPilot::~AutoPilot(){
  delete current_mode;
}

void AutoPilot::update(){

  std::vector<int> rc_out;

  if(current_mode->isAuto()){
    AutoMode* automode = static_cast<AutoMode*>(current_mode);
    AlphaState setpoint;
    if(automode->isInitial()){
      pid_roll.initialize();
      pid_pitch.initialize();
      pid_z.initialize();
      pid_yaw.initialize();
      pid_throttle.initialize();
    }

  AlphaState &state = imu_state;
  if(current_mode->getAlphaCommand() == AlphaCommand::AUTO_LANDING_CMD()){
    state.pos = marker_state.pos;
    state.rot.z = marker_state.rot.z;
  }
  
  setpoint = automode->get_setpoint(state);

    float rudder_effort = 0;
    if(automode->pid_roll){
      pid_roll.set_setpoint(setpoint.rot.x);
      rudder_effort = pid_roll.update(state.rot.x);
    }
    else if(automode->pid_yaw){
      pid_yaw.set_setpoint(setpoint.rot.z);
      rudder_effort = pid_yaw.update(state.rot.z);
    }
    
    //    if(current_mode->getAlphaCommand() == AlphaCommand::AUTO_LANDING_CMD()){
      /*      if(setpoint.pos.z != -1){
	pid_z.set_setpoint(setpoint.pos.z);
	setpoint.rot.y = pid_z.update(state.pos.z);

	}*/
    float elevator_effort = 0;

    if(automode->pid_z){
      pid_z.set_setpoint(setpoint.pos.z);
      setpoint.rot.y = pid_z.update(state.pos.z);
    }

    if(automode->pid_pitch){
      pid_pitch.set_setpoint(setpoint.rot.y);
      elevator_effort = pid_pitch.update(state.rot.y);
    }

    float throttle;
    if(automode->pid_throttle){
      pid_throttle.set_setpoint(setpoint.pos.z);
      throttle = NEUTRAL_THROTTLE + pid_throttle.update(state.pos.z)*10;
      if(throttle >2000)
	throttle = 2000;
      if(throttle < trim[THROTTLE_CH])
	throttle = trim[THROTTLE_CH];
    }
    else{
      throttle = automode->get_throttle();
    }
    
    rc_out = compute_auto_rc_out(rudder_effort,elevator_effort,throttle);//use trim 

  }
  else{//manual mode

    //turn off the LED on ch5,and ch2,ch3,ch4,ch5 is only available
    if(current_mode->getAlphaCommand() == AlphaCommand::DROP_CMD()){
      Drop* dropmode = static_cast<Drop*>(current_mode);
      if(dropmode->isInitial()){
	drop_state += 1;
	if(drop_state > 5)
	  drop_state = 0;
      }
    }
    else if(current_mode->getAlphaCommand() == AlphaCommand::CALIBRATE_CMD()){
      send_calibrate_request();
      trim = rc_in;
    }
    rc_out = compute_manual_rc_out(rc_in);	    
  }

  setRCOut(rc_out);
  
}
std::vector<int> AutoPilot::compute_auto_rc_out(float roll_effort,float pitch_effort,float throttle){
  std::vector<int> rc_out = trim;

  rc_out[DROP_CH]= computeDropPulse(drop_state);
   rc_out[AUTOPILOT_LED_CH] = 4095;
   if(throttle!=0)//when throttle is zero, turn off throttle
     rc_out[THROTTLE_CH] = throttle;
   rc_out[ELEVATOR_CH] += pitch_effort*100;
   rc_out[RUDDER_CH] += roll_effort*100;
  return rc_out;
}
std::vector<int> AutoPilot::compute_manual_rc_out(std::vector<int> rc_in){
  std::vector<int> rc_out= rc_in;

  rc_out[DROP_CH]= computeDropPulse(drop_state);
  rc_out[AUTOPILOT_LED_CH] = 0;
  return rc_out;
}

void AutoPilot::init(){
  state_sub = nh.subscribe("/pose",10,&AutoPilot::stateCB,this);
  marker_state_sub = nh.subscribe("/marker_pose",10,&AutoPilot::marker_stateCB,this);
  calibrate_pub = nh.advertise<std_msgs::Empty>("/calibrate",10);
  rcout_pub = nh.advertise<alpha_msgs::RC>("/rc_out",10);
}
void AutoPilot::stateCB(alpha_msgs::FilteredState::ConstPtr msg){
  imu_state.pos.x = msg->x;
  imu_state.pos.y = msg->y;
  imu_state.pos.z = msg->z;
  imu_state.rot.x = msg->roll;
  imu_state.rot.y = msg->pitch;
  imu_state.rot.z = msg->yaw;
}

void AutoPilot::marker_stateCB(alpha_msgs::FilteredState::ConstPtr msg){

  marker_state.pos.x = msg->x;
  marker_state.pos.y = msg->y;
  marker_state.pos.z = msg->z;
  marker_state.rot.x = msg->roll;
  marker_state.rot.y = msg->pitch;
  marker_state.rot.z = msg->yaw;
}
void AutoPilot::setRCIn(std::vector<int> &rc_in){
  for(int i = 0; i < 8; i ++)
    this->rc_in[i] = rc_in[i];
  AlphaCommand cmd(rc_in);
  current_mode = cmd.getMode(current_mode,imu_state,marker_state);

}
void AutoPilot::setRCOut(std::vector<int> &rc_out){
  alpha_msgs::RC msg;
  for(int i = 0; i < 8; i++){
    msg.Channel.push_back(rc_out[i]);
    if(i == AUTOPILOT_LED_CH)
      pwm.setPWM(i+3,0,rc_out[i]);
    else
      pwm.setServoPulse(i+3,rc_out[i]);
  }
  rcout_pub.publish(msg);
  //  nh.setParam("/rc_out",rc_out);
}
void AutoPilot::send_calibrate_request(){
  std_msgs::Empty msg;
  calibrate_pub.publish(msg);
}
