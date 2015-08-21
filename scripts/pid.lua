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
	return p_coeff*diff --+ d_coeff*(diff - prev_diff)
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
		-- roll, rotate, vertical, angular
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
		print("rotCorr "..rotCorr .. " "..diff["r_z"])

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
			--d.move(roll, pitch, 0.0, rotate)
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

function simple_pid(start_point_, prev_diff_, p_coeff_, d_coeff_)
	local prev_diff = prev_diff_
	local p_coeff = p_coeff_
	local d_coeff = d_coeff_
	local start_point = start_point_
	return function(goal)
		--Take account of rotation on the goal
		local reference = local_coords(start_point, global_coords())

		local point = local_coords(global_coords(), goal)
		local diff = local_coords(global_coords(), goal)
		print("The goal "      .. goal["t_y"] .. " " .. goal["t_x"] .. " " .. goal["t_z"] .. " ".. goal["r_z"])
		local coords = global_coords()
		print("t_x "..diff["t_x"])
		print("t_y "..diff["t_y"])
		local old_t_x = diff["t_x"]
		diff["t_x"] = diff["t_x"]*math.cos(reference["r_z"])
					- diff["t_y"]*math.sin(reference["r_z"])
		diff["t_y"] = - old_t_x*math.sin(reference["r_z"])
					+ diff["t_y"]*math.cos(reference["r_z"])

		local vertical = p_coeff["t_z"]*diff["t_z"]
		vertical = 0.0
		if vertical < 0.02 and vertical > -0.02 then
			vertical = 0.0
		end

		if diff["r_z"] < -math.pi then
			diff["r_z"] = diff["r_z"] + 2*math.pi
		elseif diff["r_z"] > math.pi then
			diff["r_z"] = diff["r_z"] - 2*math.pi
		end
		local rotate = p_coeff["r_z"]*diff["r_z"]
		if rotate < 0.1 and rotate > -0.1 then
			rotate = 0.0
		end

		local d_pitch = d_coeff["t_y"]*(diff["t_y"] - prev_diff["t_y"])
		if d_pitch > 0.0 then
			d_pitch = 0.0
		end
		print("Pitch d "..d_pitch)
		local d_roll = d_coeff["t_x"]*(diff["t_x"] - prev_diff["t_x"])
		if d_roll > 0.0 then
			d_roll = 0.0
		end
		print("Roll d "..d_roll)
		local pitch = p_coeff["t_y"]*diff["t_y"] + d_pitch
		local roll = p_coeff["t_x"]*diff["t_x"] + d_roll

		if roll < 0.03 and roll > -0.03 then
			print("Bound roll "..diff["t_x"])
			roll = 0.0
		end
		if pitch < 0.03 and pitch > -0.03 then
			print("Bound pitch "..diff["t_y"])
			pitch = 0.0
		end

		if roll > 0.5 or roll < -0.5 then
			roll = 0.5
		end
		if pitch > 0.5 or pitch < -0.5 then
			pitch = 0.5
		end

		print("coords "    ..coords["t_y"].. " " ..coords["t_x"].. " " ..coords["t_z"].. " ".. coords["r_z"])
		print("point "     .. point["t_y"].. " " .. point["t_x"].. " " .. point["t_z"].. " ".. point["r_z"])
		print("diff "      .. diff["t_y"] .. " " .. diff["t_x"] .. " " .. diff["t_z"] .. " ".. diff["r_z"])
		print("[PID] Move ".. pitch    .. " " .. roll       .. " " .. vertical    .. " ".. rotate)
		if roll ~= 0.0 or pitch ~= 0.0 or vertical ~= 0.0 or rotate ~= 0.0 then
			d.move(roll, pitch, vertical, rotate)
			d.stay()
		else
			if diff["t_y"] < 250 and diff["t_y"] > -250
			and diff["t_x"] < 250 and diff["t_x"] > -250
			then
				return 1
			end
		end

		for k,v in pairs(diff) do
			prev_diff[k] = v
		end

		return 0
	end
end

function rotation_pid(p_coeff, error_dist)
	if error_dist < -math.pi then
		error_dist = error_dist["r_z"] + 2*math.pi
	elseif error_dist > math.pi then
		error_dist = error_dist - 2*math.pi
	end

	local rotate = p_coeff*error_dist

	if rotate < 0.1 and rotate > -0.1 then
		d.stay()
		return 1
	else
		d.move(0.0, 0.0, 0.0, rotate)
		print("[PID] Turn ".. rotate)
	end
	return 0
end

function vertical_pid(p_coeff, error)
	local vertical = p_coeff*error
	if vertical < 0.02 and vertical > -0.02 then
		d.stay()
		return 1
	else
		d.move(0.0, 0.0, vertical, 0.0)
		print("[PID] Up ".. vertical)
	end
	return 0
end

function horizontal_pid(p_coeff, error)
	local pitch = p_coeff["t_y"]*error["t_y"]
	local roll = p_coeff["t_x"]*error["t_x"]

	if roll < 0.03 and roll > -0.03 then
		roll = 0.0
	end
	if pitch < 0.03 and pitch > -0.03 then
		pitch = 0.0
	end

	if roll ~= 0.0 or pitch ~= 0.0 then
		d.move(roll, pitch, 0.0, 0.0)
		d.stay()
		print("[PID] Go ".. pitch .. " " .. roll)
	else
		return 1
	end
	return 0
end

function careful_pid(start_point_, prev_error_, p_coeff_)
	local prev_error = prev_error_
	local p_coeff = p_coeff_
	local start_point = start_point_
	return function(goal)
		-- Compute position of the drone with the start point as origin
		local reference = local_coords(start_point, global_coords())
		-- Compute position of the goal with the current position as origin
		local error_dist = local_coords(global_coords(), goal)

		-- Take account of rotation to compute the error
		local old_t_x = error_dist["t_x"]
		error_dist["t_x"] = error_dist["t_x"]*math.cos(reference["r_z"])
					- error_dist["t_y"]*math.sin(reference["r_z"])
		error_dist["t_y"] = - old_t_x*math.sin(reference["r_z"])
					+ error_dist["t_y"]*math.cos(reference["r_z"])

		--[[local ret = rotation_pid(p_coeff["r_z"], error_dist["r_z"])
		if ret == 1 then
			ret = vertical_pid(p_coeff["t_z"], error_dist["t_z"])
			if ret == 1 then
				ret = horizontal_pid(p_coeff, error_dist)
				if ret == 1 then
					print("[PID] Reached")
					return 1
				end
			end
		end]]--
		local ret = rotation_pid(p_coeff["r_z"], error_dist["r_z"])
		ret = vertical_pid(p_coeff["t_z"], error_dist["t_z"])
		ret = horizontal_pid(p_coeff, error_dist)
		if ret == 3 then
			print("[PID] Reached")
			return 1
		end
		print("[PID] Going")
		for key,val in pairs(error_dist) do
			prev_error[key] = val
		end

		return 0
	end
end