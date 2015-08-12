-- package.cpath = package.cpath .. ";?.dylib"
j = require("libjakopter")
j.connect()
j.land()
print(j.battery())
j.disconnect()
