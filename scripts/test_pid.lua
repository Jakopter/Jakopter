-- package.cpath = package.cpath .. ";?.dylib"
d = require("libjakopter")
require("pid")

if d.connect() < 0 then
	print("Can't connect to the drone")
	os.exit()
end

if d.connect_coords() < 0 then
	print("Can't connect to the coords listener")
	os.exit()
end

d.takeoff()

start_point = global_coords()
goal = {
	t_x = 0.0,
	t_y = 2000.0,
	t_z = 700.0,
	r_x = 0.0,
	r_y = 0.0,
	r_z = start_point["r_z"]+math.pi
}

prev_diff = {
	t_x = 0,
	t_y = 0,
	t_z = 0,
	r_x = 0,
	r_y = 0,
	r_z = 0
}

p_coeff = {}
for v in iterator(axis) do
	p_coeff[v] = 0
end
p_coeff["t_x"] = 0.00015
p_coeff["t_y"] = 0.0001
p_coeff["t_z"] = 0.00075
p_coeff["r_z"] = 0.2

my_pid = simple_pid(start_point, prev_diff, p_coeff)
step = 0

while true do
	-- Compute the correction
	if my_pid(goal) ~= 0 then
		if step == 0 then
			goal = global_coords()
			print("Situation 1: " .. goal.t_x .. " " .. goal.t_y)
			goal["t_y"] = 2000.0
			goal["t_x"] = 500.0
		break
			step = step + 1
		elseif step == 1 then
			goal = global_coords()
			step = step + 1
			print("Angle 1: " .. goal.r_z)
			print("end")
			break
		end
	end

	--Bounding box
	coords = global_coords()
	if coords.t_x > 1200 or coords.t_x < -1200
		or coords.t_y < -2000 or coords.t_y > 3000 then
		print("END: " .. coords.t_x .. " ".. coords.t_y)
		break
	end
end

d.land()
d.stop_video()