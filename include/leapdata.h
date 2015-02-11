typedef struct {
  float height;
  float pitch;
  float yaw;
  float roll;
} LeapData_t;


extern "C" int jakopter_connect_leap();
extern "C" int jakopter_disconnect_leap();
