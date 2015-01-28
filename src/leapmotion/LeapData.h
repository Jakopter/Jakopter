typedef struct {
  float height;
  float pitch;
  float yaw;
  float roll;
} LeapData_t;

extern "C" void StartLeapData();
extern "C" void StopLeapData();
extern "C" LeapData_t GetLeapData();
