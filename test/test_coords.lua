d = require("libjakopter")



drone_velocity = function ()
	return {
		vx = d.cc_read_float(1, 20) / 1000,
		vy = d.cc_read_float(1, 24) / 1000,
		vz = d.cc_read_float(1, 28) / 1000 -- not implemented on ardrone2, use navdata_timestamp and diff
	}
end

global_coords = function ()
	return {
--Translations in millimeters
		t_x = d.cc_read_float(5, 4),
		t_y = d.cc_read_float(5, 8),
		t_z = d.cc_read_int(1, 4),
--Rotations in degrees from -180 to 180
		r_x = d.cc_read_float(1, 8) / 1000,
		r_y = d.cc_read_float(1, 12) / 1000,
		r_z = d.cc_read_float(1, 16) / 1000
	}
end

--An iterator for the angle-coordinates tables
function axis_iter(a)
	local i = 0
	local n = #a
	return function ()
		i = i+1
		if (i <= n) then
			return a[i]
		end
	end
end

--convert (angles,translation)->matrix
function transformation_matrix(coords)
	-- Based on ViSP rotation matrix building under GPL
	-- The last line of the matrix is ignored, it is always supposed to be 0,0,0,1
	cos_x = math.cos(math.rad(coords["r_x"]))
	cos_y = math.cos(math.rad(coords["r_y"]))
	cos_z = math.cos(math.rad(coords["r_z"]))
	sin_x = math.sin(math.rad(coords["r_x"]))
	sin_y = math.sin(math.rad(coords["r_y"]))
	sin_z = math.sin(math.rad(coords["r_z"]))
	return {
		cos_y*cos_z,
		-cos_y*sin_z,
		sin_y,
		coords["t_x"],
		cos_x*sin_z + sin_x*sin_y*cos_z,
		cos_x*cos_y - sin_x*sin_y*sin_z,
		-sin_x*cos_y,
		coords["t_y"],
		-cos_x*sin_y*cos_z - sin_x*sin_z,
		cos_x*sin_y*sin_y + cos_z*sin_x,
		cos_x*cos_y,
		coords["t_z"]
	}
end

-- 4*3 and 3*1
function mult_TransformMatrix_Vector(matrix, vector)
	ret = {}
	for i=1,3 do
		ret[i] = 0
		for j=1,3 do
			ret[i] = ret[i] + vector[j]*matrix[i*j]
		end
		ret[i] = ret[i] + matrix[4*i]
	end
	return ret
end

-- PID formulas:
-- diff : the error to correct
-- cumul = cumul + diff
-- move_parameter = gain + p_coeff*diff + i_coeff*cumul + d_coeff*(diff-prev_diff) + v_coeff*velocity
-- In the case of drone, x axis is linked with y axis, we need to apply an incline correction

function create_horizontal_control(p_coeff_, d_coeff_)
	prev_corr_r_x = 0
	prev_corr_r_y = 0
	p_coeff = p_coeff_
	d_coeff = d_coeff_
	return function(diff, prev_diff)
--matrice 4*3 de transformation
		matrix = transformation_matrix(global_coords())
		for index,v in ipairs(matrix) do
			print("[$]m:"..index.." "..v)
		end

		--effect of y rotation, we substract z translation added by the matrix computing
		vector_x = {1, 0, 0}
		vector_x = mult_TransformMatrix_Vector(matrix, vector_x)
		corr_r_y = vector_x[3] - matrix[12]
		-- prop & deriv for x
		xCorr = -0.25*corr_r_y --- 2.1*(corr_r_y - prev_corr_r_y)
		--effect of x rotation, we substract z translation added by the matrix computing
		vector_y = {0, 1, 0}
		vector_y = mult_TransformMatrix_Vector(matrix, vector_y)
		print("v "..vector_y[3])
		corr_r_x = vector_y[3] - matrix[12]
		--prop & deriv for y
		yCorr = 0.25*corr_r_x --+ 2.1*(corr_r_x - prev_corr_r_x)
		--include the incline parameter
		yCorr = yCorr + p_coeff["t_y"]*diff["t_y"]*0.001 --+ d_coeff["t_y"]*(diff["t_y"] - prev_diff["t_y"])
		xCorr  = xCorr + p_coeff["t_x"]*diff["t_x"]*0.001 --+ d_coeff["t_x"]*(diff["t_x"] - prev_diff["t_x"])

		print("[*]corr_r_x "..corr_r_x.." corr_r_y "..corr_r_y.." yCorr "..yCorr.." xCorr "..xCorr.." = "..p_coeff["t_x"].."*"..diff["t_x"].."+"
			..d_coeff["t_x"].."*("..diff["t_x"].."-"..prev_diff["t_x"]..")  ")

		prev_corr_r_x = corr_r_x
		prev_corr_r_y = corr_r_y
		return yCorr, xCorr
	end
end

function rotation_control(diff, prev_diff, p_coeff, d_coeff)
	if(diff > 180) then
		diff = (360-diff)
	end
	return p_coeff*math.rad(diff) --+ d_coeff*(math.rad(diff) - math.rad(prev_diff))
end

function complete_pid(prev_diff_, gain_, p_coeff_, i_coeff_, d_coeff_)
	prev_diff = prev_diff_
	gain = gain_
	p_coeff = p_coeff_
	i_coeff = i_coeff_
	d_coeff = d_coeff_
	horizontal_control = create_horizontal_control(p_coeff_, d_coeff_)
	v_coeff = -2
	old_time = 0
	return function(goal)
		diff = local_coords(global_coords(), goal)
		-- slide, rotate, vertical, angular
		move_velocities = {0.0, 0.0, 0.0, 0.0}

		-- vertical control
		thrust = gain + p_coeff["t_z"]*diff["t_z"]*0.001 --+ d_coeff["t_z"]*(diff["t_z"] - prev_diff["t_z"]) + v_coeff*velocity
		--thrust = 0.5
		yCorr, xCorr = horizontal_control(diff, prev_diff)
		rotCorr = rotation_control(diff["r_z"]*0.001, prev_diff["r_z"]*0.001, p_coeff["r_z"], d_coeff["r_z"])
		print("[$]thrust "..thrust .. " "..(diff["t_z"] - prev_diff["t_z"]))
		print("xCorr "..xCorr .. " "..diff["t_x"])
		print("yCorr "..yCorr .. " "..diff["t_y"])
		print("rotCorr "..rotCorr .. " "..math.rad(diff["r_z"]))

		move_velocities[1] = thrust*(1 - yCorr + xCorr + rotCorr)
		move_velocities[2] = thrust*(1 - yCorr - xCorr - rotCorr)
		move_velocities[3] = thrust*(1 + yCorr - xCorr + rotCorr)
		move_velocities[4] = thrust*(1 + yCorr + xCorr - rotCorr)

		-- put a limit
		-- for i,v in ipairs(move_velocities) do
		-- 	print("[%]Arg "..i.." : "..v)
		-- 	if v > 1.0 then
		-- 		move_velocities[i] = 1.0
		-- 	elseif v < -1.0 then
		-- 		move_velocities[i] = -1.0
		-- 	end
		-- end

		print("goal ".. goal["t_x"] .. " " .. goal["t_y"].. " " .. goal["t_z"] .. " " .. goal["r_z"])
		print("diff ".. diff["t_x"] .. " " .. diff["t_y"].. " " .. diff["t_z"] .. " " .. diff["r_z"])
		if move_velocities[1] ~= 0.0 or move_velocities[2] ~= 0.0
		or move_velocities[3] ~= 0.0 or move_velocities[4] ~= 0.0 then
			--d.move(slide, straight, 0.0, rotate)
			print("Move " .. move_velocities[2] .. " " .. move_velocities[1] .. " " .. move_velocities[3] .. " " .. move_velocities[4])
		else
			return 1
		end
		d.stay()

		for k,v in pairs(diff) do
			prev_diff[k]=v
		end
		return 0
	end
end

function simple_pid(start_point_, prev_diff_, gain_, p_coeff_, d_coeff_)
	prev_diff = prev_diff_
	gain = gain_
	p_coeff = p_coeff_
	d_coeff = d_coeff_
	start_point = start_point_
	old_time = 0
	return function(goal)
		diff = local_coords(global_coords(), goal)

		-- we don't use integral stuff
		thrust = p_coeff["t_z"]*diff["t_z"] + d_coeff["t_z"]*(diff["t_z"] - prev_diff["t_z"])/(os.clock()-old_time)
		print("dt: "..(os.clock()-old_time))
		old_time = os.clock()

		slide = p_coeff["t_y"]*diff["t_y"] - d_coeff["t_y"]*(diff["t_y"] - prev_diff["t_y"])
		straight = p_coeff["t_x"]*diff["t_x"] - d_coeff["t_x"]*(diff["t_x"] - prev_diff["t_x"])
		if diff["r_z"] > 180 then
			diff["r_z"] = -360 + diff["r_z"]
		end
		rotate = p_coeff["r_z"]*diff["r_z"] - d_coeff["r_z"]*(math.rad(diff["r_z"]) - math.rad(prev_diff["r_z"]))
		if rotate < 0.02 and rotate > -0.02 then
			rotate = 0.0
		end
		--Take account of rotation
		reference = local_coords(start_point, global_coords())
		sin_z = math.sin(math.rad(reference["r_z"]))
		cos_z = math.cos(math.rad(reference["r_z"]))
		print("sin : "..sin_z.." cos : "..cos_z)

		-- you need invert if the y axis is on the left of axis of x in the positive direction
		slide = -slide

		-- Get a sinusoidal value to fix forward and left/right - doesn't work
		-- s1 = (straight - math.abs(sin_z)*straight) + (slide - math.abs(cos_z)*slide)
		-- s2 = (slide - math.abs(sin_z)*slide) + (straight - math.abs(cos_z)*straight)
		-- if cos_z < 0 and sin_z < 0 then
		-- 	s1 = -s1
		-- 	s2 = -s2
		-- end
		-- if cos_z > 0 and sin_z < 0 then
		-- 	s1 = -s1
		-- end
		-- if sin_z > 0 and cos_z > 0 then
		-- 	s2 = -s2
		-- end

		-- Get a linear value between 0 and 1 to fix the forward/backward and left/right parameters
		--
		-- diagonal_angle is the angle between the diagonal (0,-1)->(1,0) and x axis
		-- sector_len is the length of the segment between the origin and the diagonal
		-- which has an angle theta from the x axis
		-- theta = math.rad(reference["r_z"])
		-- diagonal_angle = math.pi/4
		-- cos_theta = math.cos(theta)
		-- sin_theta = math.sin(theta)
		-- if (sin_theta > 0 and cos_theta > 0)
		-- 	or (sin_theta < 0 and cos_theta < 0) then
		-- 	diagonal_angle = -diagonal_angle
		-- end
		-- sector_len = 1*math.sin(diagonal_angle)/math.sin(diagonal_angle+theta)
		-- linear_corr = cos_theta*sector_len

		-- s1 = linear_corr*straight + (1 - linear_corr)*slide
		-- s2 = linear_corr*slide + (1 - linear_corr)*straight
		-- if theta > math.pi/4 and theta < 3*math.pi/4 then
		-- 	s2 = -s2
		-- end
		-- if theta > 5*math.pi/4 and theta < 7*math.pi/4 then
		-- 	s1 = -s1
		-- end
		-- --print("s1 "..s1.." s2 "..s2)
		-- print("str "..straight.." sl "..slide)
		-- straight = s1
		-- slide = s2
		if slide < 0.03 and slide > -0.03 then
			slide = 0.0
		end
		if straight < 0.03 and straight > -0.03 then
			straight = 0.0
		end


		print("goal ".. goal["t_x"] .. " " .. goal["t_y"] .. " ".. goal["r_z"])
		print("diff ".. diff["t_x"] .. " " .. diff["t_y"] .. " ".. diff["r_z"])
		print("Move "..straight.." "..slide .. " " .. thrust.." "..rotate)

		if slide ~= 0.0 or straight ~= 0.0 or rotate ~= 0.0 then
			d.move(slide, straight, 0.0, rotate)
			--print(d.log_command())
		else
			return 1
		end
		d.stay()
		--print(d.log_command())

		for k,v in pairs(diff) do
			prev_diff[k]=v
		end

		return 0
	end
end

-- Compute the distance from the start point
function local_distance(start_point, coords)
	local t = {}
	for key, value in pairs(coords) do
		if string.find(key,"^t_") then
			t[key] = math.abs(value - start_point[key])
		elseif string.find(key,"^r_") then
			angle = math.abs(value - start_point[key])
			if angle > 180 then
				angle = 360 - angle
			end
			t[key] = angle
		end
	end
	return t
end

-- Compute the coordinates in a local origin. Angles are comprised between 0 and 360, clockwise from the camera
function local_coords(start_point, coords)
	loc = {}
	for key, value in pairs(coords) do
		loc[key] = value-start_point[key]
		if string.find(key,"^r_") and loc[key] < 0 then
			loc[key] = 360 + loc[key]
		end
	end
	return loc
end


if d.connect() < 0 then
	print("Can't connect to the drone")
	os.exit()
end

if d.connect_coords() < 0 then
	print("Can't connect to the coords listener")
	os.exit()
end

--d.connect_video()

-- Wait the vicon to be ready
repeat
	d.usleep(20*1000)
until d.cc_read_int(5,0) == 1

--d.takeoff()

axis = {
	"t_x",
	"t_y",
	"t_z",
	"r_x",
	"r_y",
	"r_z"
}
start_point = global_coords()
-- goal = global_coords()
-- goal["t_x"] = goal["t_x"] + 100.0
goal = {
	t_x = 0.0,
	t_y = -1000.0,
	t_z = 100.0,
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

gain = 0.5

p_coeff = {}
for v in axis_iter(axis) do
	p_coeff[v] = 0
end
p_coeff["t_x"] = 0.00015
p_coeff["t_y"] = 0.0001
p_coeff["t_z"] = 0.00005
p_coeff["r_z"] = 0.005

i_coeff = {}
for v in axis_iter(axis) do
	i_coeff[v] = 0
end

d_coeff = {}
for v in axis_iter(axis) do
	d_coeff[v] = 0
end
d_coeff["t_x"] = -0.0005
d_coeff["t_y"] = -0.0005
d_coeff["t_z"] = -0.000005
d_coeff["r_z"] = 0.0005

p_coeff2 = {}
for v in axis_iter(axis) do
	p_coeff2[v] = 0
end
p_coeff2["t_x"] = -0.005
p_coeff2["t_y"] = 0.005
p_coeff2["t_z"] = 2
p_coeff2["r_z"] = 0.1

d_coeff2 = {}
for v in axis_iter(axis) do
	d_coeff2[v] = 0
end
d_coeff2["t_x"] = -1
d_coeff2["t_y"] = 1
d_coeff2["t_z"] = 0
d_coeff2["r_z"] = 2
gain2 = 5.335

my_pid = simple_pid(start_point, prev_diff, gain, p_coeff, d_coeff)
ra_pid = complete_pid(prev_diff, gain2, p_coeff2, i_coeff, d_coeff2)

step = 0
id_yaw = -1
while true do
	bat = d.cc_read_int(1, 0)
	--angles en millidegrés, il faut les convertir en degrés.
	yaw = d.cc_read_float(1, 16) / 1000

	-- if (id_yaw ~= -1) then
	-- 	d.draw_remove(id_yaw)
	-- 	id_yaw = -1
	-- end
	-- id_yaw = d.draw_text("Yaw "..yaw,0,120)

	coords = local_coords(start_point, global_coords())

	if my_pid(goal) ~= 0 then
		if step == 0 then
			goal = global_coords()
			print("Situation 1: " .. coords.t_x)
			--pas sûr
			goal["r_z"] = (goal["r_z"] + 90.0)%360
			step = step + 1
			break
		elseif step == 1 then
			goal = global_coords()
			step = step + 1
			print("Angle 1: " .. coords.r_z)
			print("end")
			break
		end
	end

	coords = global_coords()
	if coords.t_x > 1200 or coords.t_x < -1200
		or coords.t_y < -2400 or coords.t_y > 2600 then
		print("END: " .. coords.t_x .. " ".. coords.t_y)
		break
	end

	ra_pid(goal)

end

d.land()
d.stop_video()