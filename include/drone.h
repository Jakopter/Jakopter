#ifndef JAKOPTER_DRONE_H
#define JAKOPTER_DRONE_H

#include "common.h"
#include "navdata.h"
#include "com_channel.h"
#include "com_master.h"

#define PACKET_SIZE     256
#define PORT_CMD        5556

/* 30 ms in ns */
#define TIMEOUT_CMD      30000000
#define NAVDATA_ATTEMPT  10
#define HEIGHT_THRESHOLD 500

#define HEAD_LEN    10
#define ARG_LEN     30
#define ARGS_MAX     7

/* Headers of AT* messages */
#define HEAD_REF        "REF"
#define HEAD_PCMD       "PCMD"
#define HEAD_CONFIG     "CONFIG"
#define HEAD_CTRL       "CTRL"
#define HEAD_COM_WATCHDOG "COMWDG"
#define HEAD_FTRIM      "FTRIM"
#define HEAD_CALIB      "CALIB"

enum video_channels {
	VID_HORIZONTAL,
	VID_VERTICAL,
	VID_HORIZON_VERTICAL,
	VID_VERTICAL_HORIZON,
	VID_SWITCH,
	VID_CHANNELS
};

/**
 * \brief Creates a socket and starts the command thread. Needs the computer to be connected to the drone wifi network.
 * \param drone_ip used by simulator, if you are with a real drone, set it to NULL. 255.255.255.255 can't be used (see man inet_addr)
 * \returns 0 if success, -1 if error
*/
int jakopter_connect(const char* drone_ip);
/**
  * \brief Command to take off the drone
  * \returns 0 if success, -1 if error.
  */
int jakopter_takeoff();
/**
  * \brief Command to land the drone. If no recent navdata are received, it sends the emergency command.
  * \returns 0 if success, -1 if error.
  */
int jakopter_land();
/**
  * \brief Command to stop drone rotors.
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_emergency();
/**
  * \brief Command to reset the communication watchdog.
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_reinit();
/**
  * \brief Stop main thread (End of drone connection)
  * \return pthread_join value or -1 if the communication is already stopped
  */
int jakopter_disconnect();
/**
  * \brief Command to make the drone rotate to the left with an angular speed.
  * \param speed the angular speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_rotate_left(float speed);
/**
  * \brief Command to make the drone rotate to the right with an angular speed.
  * \param speed the angular speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_rotate_right(float speed);
/**
  * \brief Command to make the drone slide to the left with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_slide_left(float speed);
/**
  * \brief Command to make the drone slide to the right with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_slide_right(float speed);
/**
  * \brief Command to make the drone go forward with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_forward(float speed);
/**
  * \brief Command to make the drone go backward with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_backward(float speed);
/**
  * \brief Command to make the drone go up with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_up(float speed);
/**
  * \brief Command to make the drone go down with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_down(float speed);
/**
  * \brief Command to make the drone stay at its position.
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_stay();
/**
  * \brief define the movement of the drone (-1.0 to 1.0 values).
  * \param l_to_r the speed from left to right
  * \param b_to_f the speed from backward to forward
  * \param vertical_speed the speed from down to up
  * \param angular_speed the angular speed to rotate the drone
  * \return 0 if success, -1 if command couldn't be set.
  */
int jakopter_move(float l_to_r, float b_to_f, float vertical_speed, float angular_speed);
/**
  * \brief Command to switch between front and bottom camera.
  * \param id the id of the camera in video_channels enum:
  * - 0 horizontal
  * - 1 vertical
  * - 2 horizontal with vertical on left corner. Deprecated on ARdrone2
  * - 3 vertical with horizontal on left corner. Deprecated on ARdrone2
  * - 4 switch to the next camera
  * \returns 0 if success, -1 if command couldn't be set or id invalid.
  */
int jakopter_switch_camera(unsigned int id);
/**
  * \brief Return a timestamp and the last command sent separated with a colon.
  * \returns the last logged_command if connected, NULL otherwise.
  */
const char* jakopter_log_command();

/* DEBUG */
/**
  * \brief Set the frame of reference of the drone before taking off
  * \returns 0 if success, -1 if the drone is flying or the command couldn't be set.
  */
int jakopter_flat_trim();
/**
  * \brief Calibration of the drone for smartphone accelerometer
  * \returns 0 if success, -1 if the drone isn't flying or the command couldn't be set.
  */
int jakopter_calib();

/* Used by navdata */
/**
 * \brief Command used by navdata to define the type of navdata to "demo".
 * \returns send return code or -1 if command couldn't be set.
*/
int init_navdata_bootstrap();
/**
 * \brief Command used by navdata to acknowledge navdata settings.
 * \returns send return code or -1 if command couldn't be set.
*/
int config_ack();


#endif