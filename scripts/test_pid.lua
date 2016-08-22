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
	t_x = start_point["t_x"],
	t_y = start_point["t_y"]-1000.0,
	t_z = 0.0,
	r_x = 0.0,
	r_y = 0.0,
	r_z = start_point["r_z"]
}

prev_diff = local_coords(global_coords(), goal)

-- speed
p_coeff = {}
for v in iterator(axis) do
	p_coeff[v] = 0
end
p_coeff["t_x"] = 0.00015
p_coeff["t_y"] = 0.0001
p_coeff["t_z"] = 0.00075
p_coeff["r_z"] = 0.3
-- brake
d_coeff = {}
for v in iterator(axis) do
	d_coeff[v] = 0
end
d_coeff["t_x"] = 0.00075
d_coeff["t_y"] = 0.0005
d_coeff["t_z"] = 0.0
d_coeff["r_z"] = 0.01

pid_rotation = careful_pid(start_point, prev_diff, p_coeff, d_coeff, 0x3)
pid_move = careful_pid(start_point, prev_diff, p_coeff, d_coeff, 0x1)
pid_up = careful_pid(start_point, prev_diff, p_coeff, d_coeff, 0x5)
step = 0
my_pid = pid_rotation

while true do
	-- Compute the correction
	if my_pid(goal) ~= 0 then
		if step == 0 then
			goal = global_coords()
			print("Situation 1: " .. goal.t_x .. " " .. goal.t_y)
			print("Angle 1: " .. goal.r_z)
			goal["t_x"] = 0.0
			goal["t_y"] = 1000.0
			my_pid = pid_move
			step = step + 1
		elseif step == 1 then
			goal = global_coords()
			print("Situation 2: " .. goal.t_x .. " " .. goal.t_y)
			print("Angle 2: " .. goal.r_z)
			step = step + 1
		elseif step == 2 then
			print("Situation 3: " .. goal.t_x .. " " .. goal.t_y)
			print("Angle 3: " .. goal.r_z)
			print("end")
			break
		end
	end

	--Bounding box
	coords = global_coords()
	if coords.t_x > 1400 or coords.t_x < -1000
		or coords.t_y < -2500 or coords.t_y > 2500 then
		print("END: " .. coords.t_x .. " ".. coords.t_y)
		break
	end
end

d.land()
d.stop_video()