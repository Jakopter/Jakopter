require("coords")
require("utils")

-- PID formulas:
-- diff : the error to correct
-- cumul = cumul + diff
-- move_parameter = gain + p_coeff*diff + i_coeff*cumul + d_coeff*(diff-prev_diff) + v_coeff*velocity
-- In the case of drone, x axis is linked with y axis, we need to apply an incline correction

function create_horizontal_control(p_coeff_, d_coeff_)
	local prev_corr_r_x = 0
	local prev_corr_r_y = 0
	local p_coeff = p_coeff_
	local d_coeff = d_coeff_
	return function(diff, prev_diff)
--matrice 4*3 de transformation
		local vector_x = {1, 0, 0}
		local vector_y = {0, 1, 0}

		matrix = angles_to_homogeneous_matrix(global_coords())

		--effect of y rotation, we substract z translation added by the matrix computing
		vector_x = mult_TransformMatrix_Vector(matrix, vector_x)
		local corr_r_y = vector_x[3] - matrix[3][4]
		-- prop & deriv for x
		local xCorr = p_coeff["r_y"]*corr_r_y --+ d_coeff["r_y"]*(corr_r_y - prev_corr_r_y)
		--effect of x rotation, we substract z translation added by the matrix computing
		vector_y = mult_TransformMatrix_Vector(matrix, vector_y)
		local corr_r_x = vector_y[3] - matrix[3][4]
		--prop & deriv for y
		local yCorr = p_coeff["r_x"]*corr_r_x --+ d_coeff["r_x"]*(corr_r_x - prev_corr_r_x)
		--include the incline parameter to the translation
		yCorr = yCorr + p_coeff["t_y"]*diff["t_y"]*0.001 --+ d_coeff["t_y"]*(diff["t_y"] - prev_diff["t_y"])*0.001
		xCorr  = xCorr + p_coeff["t_x"]*diff["t_x"]*0.001 --+ d_coeff["t_x"]*(diff["t_x"] - prev_diff["t_x"])*0.001

		prev_corr_r_x = corr_r_x
		prev_corr_r_y = corr_r_y
		return xCorr, yCorr
	end
end

function rotation_control(diff, prev_diff, p_coeff, d_coeff)
	if(diff > 180) then
		diff = (360-diff)
	end
	return p_coeff*math.rad(diff) --+ d_coeff*(math.rad(diff) - math.rad(prev_diff))
end

function complete_pid(prev_diff_, gain_, p_coeff_, i_coeff_, d_coeff_)
	local prev_diff = prev_diff_
	local gain = gain_
	local p_coeff = p_coeff_
	local i_coeff = i_coeff_
	local d_coeff = d_coeff_
	local horizontal_control = create_horizontal_control(p_coeff_, d_coeff_)
	local old_time = 0
	return function(goal)
		local diff = local_coords(global_coords(), goal)
		-- slide, rotate, vertical, angular
		local move_velocities = {0.0, 0.0, 0.0, 0.0}

		-- vertical control
		local thrust = gain + p_coeff["t_z"]*diff["t_z"]*0.001 --+ d_coeff["t_z"]*(diff["t_z"] - prev_diff["t_z"])*0.001
		print("Delay "..os.clock()-old_time)
		old_time = os.clock()
		local xCorr, yCorr = horizontal_control(diff, prev_diff)
		local rotCorr = rotation_control(diff["r_z"]*0.001, prev_diff["r_z"]*0.001, p_coeff["r_z"], d_coeff["r_z"])
		print("[$]thrust "..thrust .. " "..(diff["t_z"] - prev_diff["t_z"]))
		print("xCorr "..xCorr .. " "..diff["t_x"])
		print("yCorr "..yCorr .. " "..diff["t_y"])
		print("rotCorr "..rotCorr .. " "..math.rad(diff["r_z"]))

		-- vitesse pour chaque moteur
		move_velocities[1] = thrust*(1 - yCorr + xCorr + rotCorr)
		move_velocities[2] = thrust*(1 + yCorr - xCorr + rotCorr)
		move_velocities[3] = thrust*(1 - (yCorr + xCorr + rotCorr))
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

function simple_pid(start_point_, prev_diff_, p_coeff_)
	local prev_diff = prev_diff_
	local p_coeff = p_coeff_
	local start_point = start_point_
	return function(goal)
		--Take account of rotation
		local reference = local_coords(start_point, global_coords())
		local point = {
			t_x = goal["t_x"],
			t_y = goal["t_y"],
			t_z = goal["t_z"],
			r_x = goal["r_x"],
			r_y = goal["r_y"],
			r_z = goal["r_z"]
		}
		-- print("t_x "..point["t_x"])
		-- print("t_y "..point["t_y"])
		-- local old_t_x = point["t_x"]
		-- point["t_x"] = point["t_x"]*math.cos(-math.rad(reference["r_z"]))-point["t_y"]*math.sin(math.rad(reference["r_z"]))+global_coords()["t_x"]
		-- point["t_y"] = old_t_x*math.sin(-math.rad(reference["r_z"]))+point["t_y"]*math.cos(math.rad(reference["r_z"]))+global_coords()["t_y"]


		local diff = local_coords(global_coords(), point)
		print("TRUE goal "      .. goal["t_y"] .. " " .. goal["t_x"] .. " " .. goal["t_z"] .. " ".. goal["r_z"])
		local coords = global_coords()

		local vertical = p_coeff["t_z"]*diff["t_z"]
		if vertical < 0.02 and vertical > -0.02 then
			vertical = 0.0
		end
		local rotate = p_coeff["r_z"]*diff["r_z"]
		if rotate < 0.02 and rotate > -0.02 then
			rotate = 0.0
		end


		--Take account of rotation (see how to rotate a reference frame)
		local straight = p_coeff["t_y"]*diff["t_y"]

		local slide = p_coeff["t_x"]*diff["t_x"]

		-- you need invert if the y axis is on the left of axis of x in the positive direction
		--slide = -slide

		-- Get a linear value between 0 and 1 to fix the forward/backward and left/right parameters
		--
		-- diagonal_angle is the angle between the diagonal (0,-1)->(1,0) and x axis
		-- sector_len is the length of the segment between the origin and the diagonal
		-- which has an angle theta from the x axis
		-- Note that this isn't perfect: at x*pi/4 there is a threshold where the drone hesitate to choose left or right
		-- local theta = math.rad(reference["r_z"])
		-- local diagonal_angle = math.pi/4
		-- local cos_theta = math.cos(theta)
		-- local sin_theta = math.sin(theta)
		-- if (sin_theta > 0 and cos_theta < 0)
		-- 	or (sin_theta < 0 and cos_theta > 0) then
		-- 	diagonal_angle = -diagonal_angle
		-- end
		-- local sector_len = 1*math.sin(diagonal_angle)/math.sin(diagonal_angle+theta)
		-- local linear_corr = cos_theta*sector_len

		-- local s1 = linear_corr*straight + (1 - linear_corr)*slide
		-- local s2 = linear_corr*slide + (1 - linear_corr)*straight
		-- if theta > math.pi/4 and theta < 3*math.pi/4 then
		-- 	s2 = -s2
		-- end
		-- if theta > 3*math.pi/4 or theta < -3*math.pi/4 then
		-- 	s1 = -s1
		-- 	s2 = -s2
		-- end
		-- if theta < -math.pi/4 and theta > -3*math.pi/4 then
		-- 	s1 = -s1
		-- end
		-- --print("str "..straight.." sl "..slide)
		-- straight = s1
		-- slide = s2
		if slide < 0.03 and slide > -0.03 then
			slide = 0.0
		end
		if straight < 0.03 and straight > -0.03 then
			straight = 0.0
		end

		print("coords "    ..coords["t_y"].. " " ..coords["t_x"].. " " ..coords["t_z"].. " ".. coords["r_z"])
		print("point "     .. point["t_y"].. " " .. point["t_x"].. " " .. point["t_z"].. " ".. point["r_z"])
		print("diff "      .. diff["t_y"] .. " " .. diff["t_x"] .. " " .. diff["t_z"] .. " ".. diff["r_z"])
		print("[PID] Move ".. straight    .. " " .. slide       .. " " .. vertical    .. " ".. rotate)
		if slide ~= 0.0 or straight ~= 0.0 or vertical ~= 0.0 or rotate ~= 0.0 then
			d.move(slide, straight, vertical, rotate)
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