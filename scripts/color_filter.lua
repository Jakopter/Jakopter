-- package.cpath = package.cpath .. ";?.dylib"
d = require("libjakopter")

if l.use_visp() then
	print("ViSP callback doesn't handle yet filtering")
	os.exit()
end
d.connect()
d.connect_video()

d.set_callback(1)

d.cc_write_int(2, 28, 100) -- brightness (0 to 256)
d.cc_write_int(2, 32, 200) -- darkness (0 to 256)
d.cc_write_int(2, 36, 124) -- U chrominance : hot to cold colors (0 to 256)
d.cc_write_int(2, 40, 132) -- V chrominance : deep to pale colors (0 to 256)

tt = 0
tt_new = 0
while true do
	--update if data have changed
	repeat
		--yield to avoid CPU use
		d.yield()
		tt_new = d.cc_get_timestamp(1)
		d.usleep(1000*1000)
	until tt_new > tt

	tt = tt_new
	bat = d.cc_read_int(1, 0)
	alt = d.cc_read_int(1, 4)
	--angles in millidegrees, needs to convert in degrees.
	pitch = d.cc_read_float(1, 8) / 1000
	roll = d.cc_read_float(1, 12) / 1000
	yaw = d.cc_read_float(1, 16) / 1000
	--send data to display
	d.cc_write_int(2, 0, bat)
	d.cc_write_int(2, 4, alt)
	d.cc_write_float(2, 8, pitch)
	d.cc_write_float(2, 12, roll)
	d.cc_write_float(2, 16, yaw)
	-- get the percentage of blue
	blue = d.cc_read_float(6, 0)
	if (blue > 0.5) then
		print("There is so much blue here "..blue.."%")
	end
end