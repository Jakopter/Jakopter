-- package.cpath = package.cpath .. ";?.dylib"
j = require("libjakopter")
j.connect()
j.takeoff()
j.stay()
print(j.log_command())
j.usleep(2000000)
j.stay()
j.land()
j.disconnect()