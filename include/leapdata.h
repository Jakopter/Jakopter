typedef struct {
  float height;
  float pitch;
  float yaw;
  float roll;
} LeapData_t;


#ifdef __cplusplus
extern "C" {
#endif
	int jakopter_connect_leap();
	int jakopter_disconnect_leap(); 
#ifdef __cplusplus
}
#endif