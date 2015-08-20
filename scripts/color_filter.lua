--package.cpath = package.cpath .. ";?.dylib"
d = require("libjakopter")
d.connect()
d.connect_video()
d.set_callback(1)

d.cc_write_int(2, 28, 100) -- brightness (0 to 256)
d.cc_write_int(2, 32, 200) -- darkness (0 to 256)
d.cc_write_int(2, 36, 124) -- U chrominance : hot to cold colors (0 to 256)
d.cc_write_int(2, 40, 132) -- V chrominance : deep to pale colors (0 to 256)

while true do
    -- get the percentage of blue
    blue = d.cc_read_float(6, 0)
    if (blue > 0.8) then
        print("There is so much blue here "..blue.."%")
        d.cc_write_float(2, 24, 1) -- Take a screenshot
        break
    end
end
d.usleep(1000*1000)