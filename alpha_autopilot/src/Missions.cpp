
#include <alpha_autopilot/AlphaMode.h>
#include <cmath>
//TODO implement missions

AlphaState HorizontalTurn::get_setpoint(AlphaState state){
  AlphaState setpoint;
  setpoint.pos.z = initial_state.pos.z;
  setpoint.rot.x =0.2;
  is_initial = false;
  return setpoint;
}

AlphaState EightTurn::get_setpoint(AlphaState state){
  AlphaState setpoint;
  setpoint.pos.z = initial_state.pos.z;
  setpoint.rot.x = 0.1;
  is_initial = false;
  return setpoint;
}

AlphaState RiseTurn::get_setpoint(AlphaState state){
 AlphaState setpoint;
  setpoint.pos.z = initial_state.pos.z;
  setpoint.rot.x = 0;
  is_initial = false;
  return setpoint;
}

AlphaState Glide::get_setpoint(AlphaState state){
  AlphaState setpoint;
  setpoint.pos.z = initial_state.pos.z;
  setpoint.rot.x = -0.1;
  is_initial = false;
  return setpoint;
}

AlphaState Land::get_setpoint(AlphaState state){
  AlphaState setpoint;
  setpoint.pos.z = initial_state.pos.z;
  setpoint.rot.x = -0.2;
  is_initial = false;
  return setpoint;
}



