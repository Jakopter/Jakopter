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
	t_y = 3000.0,
	t_z = 1500.0,
	r_x = 0.0,
	r_y = 0.0,
	r_z = start_point["r_z"]
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
p_coeff["r_z"] = 0.005


p_coeff2 = {}
for v in iterator(axis) do
	p_coeff2[v] = 0
end
p_coeff2["t_x"] = -0.005
p_coeff2["t_y"] = 0.005
p_coeff2["t_z"] = 2
p_coeff2["r_x"] = 0.25
p_coeff2["r_y"] = -0.25
p_coeff2["r_z"] = 0.1

i_coeff = {}
for v in iterator(axis) do
	i_coeff[v] = 0
end

d_coeff2 = {}
for v in iterator(axis) do
	d_coeff2[v] = 0
end
d_coeff2["t_x"] = -1
d_coeff2["t_y"] = 1
d_coeff2["t_z"] = 0
d_coeff2["r_x"] = 2.1
d_coeff2["r_y"] = -2.1
d_coeff2["r_z"] = 2
gain2 = -2.6 --5.335

my_pid = simple_pid(start_point, prev_diff, p_coeff)
ra_pid = complete_pid(prev_diff, gain2, p_coeff2, i_coeff, d_coeff2)
step = 0

while true do
	-- Compute the correction
	if my_pid(goal) ~= 0 then
		if step == 0 then
			goal = global_coords()
			print("Situation 1: " .. goal.t_x .. " " .. goal.t_y)
			goal["t_y"] = 2000.0
			goal["t_x"] = 500.0
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
		or coords.t_y < 1300 or coords.t_y > 4600 then
		print("END: " .. coords.t_x .. " ".. coords.t_y)
		break
	end

	--ra_pid(goal)
end

d.land()
d.stop_video()