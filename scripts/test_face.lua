-- package.cpath = package.cpath .. ";?.dylib"
d = require("libjakopter")

if not d.use_visp() then
	print("You need to compile with ViSP in order to use face detection")
	os.exit()
end
if d.connect() < 0 then
	print("Can't connect to the drone")
	os.exit()
end
d.connect_video()

d.takeoff()

d.up(1.0)

repeat
	alt = d.cc_read_int(1, 4)
	print("Alt "..alt)
	d.up(0.8)
	d.up(0.5)
until alt > 900
prev_size = 0
while true do
	bat = d.cc_read_int(1, 0)
	alt = d.cc_read_int(1, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = d.cc_read_float(1, 8) / 1000
	roll = d.cc_read_float(1, 12) / 1000
	yaw = d.cc_read_float(1, 16) / 1000
	--envoi des données à la vidéo
	d.cc_write_int(2, 0, bat)
	d.cc_write_int(2, 4, alt)
	d.cc_write_float(2, 8, pitch)
	d.cc_write_float(2, 12, roll)
	d.cc_write_float(2, 16, yaw)

	if alt < 900 then
		d.up(0.3)
		d.up(0.3)
	end

	if alt > 2000 then
		d.down(0.5)
		d.down(0.3)
	end

	size = d.cc_read_float(6,0)
	if size > 2500 and prev_size == 0 then
		prev_size = size
	end
	x = d.cc_read_float(6,4)
	y = d.cc_read_float(6,8)

	if size > 5000 and prev_size > 4000
	and x > 200 and x < 400
	and y > 90 and y < 125 then
		print("Couché! "..size.." "..x.." "..y)
		break
	end
	prev_size = size
end

d.land()