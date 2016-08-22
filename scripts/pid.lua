require("coords")
require("utils")

-- PID formulas:
-- diff : the error to correct
-- integral: cumul = cumul + diff but here, the centrifugal force
--  increases progressively the integral value
-- move_parameter = gain + p_coeff*diff + i_coeff*cumul + d_coeff*(diff-prev_diff)
-- In the case of drone, x axis is linked with y axis, rotation changes the error in x and y
-- We assume that dt is constant for now, this will be added later
--
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
-- PID used in a simulation of quadropter to control each rotor
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
		local old_t_y = diff["t_y"]
		-- formula to rotate a frame of reference
		diff["t_x"] = diff["t_x"]*math.cos(reference["r_z"])
					- diff["t_y"]*math.sin(reference["r_z"])
		diff["t_y"] = - old_t_x*math.sin(reference["r_z"])
					+ diff["t_y"]*math.cos(reference["r_z"])
		local old_prev_t_x = prev_diff["t_x"]
		prev_diff["t_x"] = prev_diff["t_x"]*math.cos(reference["r_z"])
						- prev_diff["t_y"]*math.sin(reference["r_z"])
		prev_diff["t_y"] = - old_prev_t_x*math.sin(reference["r_z"])
						+ prev_diff["t_y"]*math.cos(reference["r_z"])

		local vertical = p_coeff["t_z"]*diff["t_z"]
		vertical = 0.0
		if vertical < 0.07 and vertical > -0.07 then
			vertical = 0.0
		end

		if diff["r_z"] < -math.pi then
			diff["r_z"] = diff["r_z"] + 2*math.pi
		elseif diff["r_z"] > math.pi then
			diff["r_z"] = diff["r_z"] - 2*math.pi
		end

		local d_rotate = d_coeff["r_z"]*(diff["r_z"] - prev_diff["r_z"])
		print("Rotate d "..d_rotate)
		if (prev_diff["r_z"] > diff["r_z"] and diff["r_z"] > 0)
			or (d_rotate < 0 and prev_diff["r_z"] < diff["r_z"] and diff["r_z"] < 0) then
			d_rotate = 0.0
		end

		local rotate = p_coeff["r_z"]*diff["r_z"] + d_rotate
		if rotate < 0.1 and rotate > -0.1 then
			rotate = 0.0
		end

		local d_roll = d_coeff["t_x"]*(diff["t_x"] - prev_diff["t_x"])
		print("Roll d "..d_roll)
		if (prev_diff["t_x"] > diff["t_x"] and diff["t_x"] > 0)
			or (prev_diff["t_x"] < diff["t_x"] and diff["t_x"] < 0) then
			d_roll = 0.0
		end
		local roll = p_coeff["t_x"]*diff["t_x"] + d_roll

		local d_pitch = d_coeff["t_y"]*(diff["t_y"] - prev_diff["t_y"])
		print("Pitch d "..d_pitch)
		if (prev_diff["t_y"] > diff["t_y"] and diff["t_y"] > 0)
			or (prev_diff["t_y"] < diff["t_y"] and diff["t_y"] < 0) then
			d_pitch = 0.0
		end
		local pitch = p_coeff["t_y"]*diff["t_y"] + d_pitch

		if roll < 0.03 and roll > -0.03 then
			print("Bound roll "..diff["t_x"])
			roll = 0.0
		end
		if pitch < 0.03 and pitch > -0.03 then
			print("Bound pitch "..diff["t_y"])
			pitch = 0.0
		end

		if roll > 0.5 then
			roll = 0.5
		elseif roll < -0.5 then
			roll = -0.5
		end
		if pitch > 0.5 then
			pitch = 0.5
		elseif pitch < -0.2 then
			pitch = -0.2
		end

		print("coords "    ..coords["t_x"].. " " .. coords["t_y"].." ".. coords["t_z"].. " "..coords["r_z"])
		print("point "     .. point["t_x"].. " " .. point["t_y"].. " ".. point["t_z"].. " ".. point["r_z"])
		print("diff "      .. diff["t_x"] .. " " .. diff["t_y"] .. " ".. diff["t_z"] .. " ".. diff["r_z"])
		print("[PID] Move ".. roll        .. " " .. pitch       .. " ".. vertical    .. " ".. rotate)
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

		print("OLD x: ".. old_t_x)
		print("OLD y: ".. old_t_y)
		prev_diff["t_x"] = old_t_x
		prev_diff["t_y"] = old_t_y


		return 0
	end
end

function compute_pid(diff, prev_diff, p_coeff, d_coeff)
	local d_value = d_coeff*(diff - prev_diff)
	--print("D is "..d_value.." = "..diff.." - "..prev_diff)
	if (prev_diff > diff and diff > 0)
		or (prev_diff < diff and diff < 0) then
		d_value = 0.0
	end
	if d_value > 1.0 then print("d_value > 1.0: "..d_value) end
	if d_value < -1.0 then print("d_value < -1.0: "..d_value) end

	return p_coeff*diff + d_value
end

function rotation_pid(diff, prev_diff, p_coeff, d_coeff)
	if diff < -math.pi then
		diff = diff + 2*math.pi
	elseif diff > math.pi then
		diff = diff - 2*math.pi
	end

	assert(diff < 2*math.pi, "diff > 2*math.pi: "..diff)
	assert(diff > -2*math.pi, "diff < -2*math.pi: "..diff)

	--print("Rotate")
	local rotate = compute_pid(diff, prev_diff, p_coeff, d_coeff)
	if rotate < 0.2 and rotate > -0.2 then
		rotate = 0.0
	end
	if rotate > 1.0 then print("rotate > 1.0: "..rotate) end
	if rotate < -1.0 then print("rotate < -1.0: "..rotate) end
	return rotate
end

function vertical_pid(diff, prev_diff, p_coeff, d_coeff)
	local vertical = compute_pid(diff, prev_diff, p_coeff, d_coeff)
	if vertical < 0.02 and vertical > -0.02 then
		vertical = 0.0
	end
	if vertical > 1.0 then print("vertical > 1.0") end
	if vertical < -1.0 then print("vertical < -1.0") end
	return vertical
end

function horizontal_pid(diff, prev_diff, p_coeff, d_coeff)
	--print("Pitch-roll")
	local pitch = compute_pid(diff["t_y"], prev_diff["t_y"], p_coeff["t_y"], d_coeff["t_y"])
	local roll = compute_pid(diff["t_x"], prev_diff["t_x"], p_coeff["t_x"], d_coeff["t_x"])

	if pitch > 1.0  then print("pitch > 1.0: "..pitch) end
	if pitch < -1.0 then print("pitch < -1.0: "..pitch) end
	if roll > 1.0   then print("pitch > 1.0: "..roll) end
	if roll < -1.0  then print("pitch < -1.0: "..roll) end

	if roll < 0.03 and roll > -0.03 then
		roll = 0.0
	end
	if pitch < 0.03 and pitch > -0.03 then
		pitch = 0.0
	end
	if roll > 0.5 then
		roll = 0.5
	elseif roll < -0.5 then
		roll = -0.5
	end
	if pitch > 0.5 then
		pitch = 0.5
	elseif pitch < -0.2 then
		pitch = -0.2
	end


	return pitch, roll
end

function careful_pid(start_point_, prev_diff_, p_coeff_, d_coeff_, mode)
	local prev_diff = prev_diff_
	local p_coeff = p_coeff_
	local d_coeff = d_coeff_
	local start_point = start_point_
	-- mode is a binary value (111 (7) to use all the pids)
	local use_vertical = (mode & 4) == 4
	local use_rotation = (mode & 2) == 2
	local use_horizontal = (mode & 1) == 1
	local old_time = 0.0
	local prev_pitch_velocity = 0.0
	local prev_roll_velocity = 0.0
	return function(goal)
		local reached = 0
		--print("The goal " .. goal["t_y"] .. " " .. goal["t_x"] .. " " .. goal["t_z"] .. " ".. goal["r_z"])
		local point = local_coords(global_coords(), goal)
		local diff = local_coords(global_coords(), goal)
		local coords = global_coords()
		local old_t_x = diff["t_x"]
		local old_t_y = diff["t_y"]
		local old_prev_t_x = prev_diff["t_x"]
		--Take account of rotation on the goal
		local reference = local_coords(start_point, global_coords())
		-- formula to rotate a frame of reference
		diff["t_x"] = diff["t_x"]*math.cos(reference["r_z"])
					- diff["t_y"]*math.sin(reference["r_z"])
		diff["t_y"] = - old_t_x*math.sin(reference["r_z"])
					+ diff["t_y"]*math.cos(reference["r_z"])
		prev_diff["t_x"] = prev_diff["t_x"]*math.cos(reference["r_z"])
						- prev_diff["t_y"]*math.sin(reference["r_z"])
		prev_diff["t_y"] = - old_prev_t_x*math.sin(reference["r_z"])
						+ prev_diff["t_y"]*math.cos(reference["r_z"])
		assert(diff["t_x"] < 1.0e+14, "diff out of bounds: "..diff["t_x"])
		assert(diff["t_x"] > -1.0e+14, "diff out of bounds: "..diff["t_x"])
		assert(diff["t_y"] < 1.0e+14, "diff out of bounds: "..diff["t_y"])
		assert(diff["t_y"] > -1.0e+14, "diff out of bounds: "..diff["t_y"])

		assert(prev_diff["t_x"] < 1.0e+14, "prev_diff out of bounds: "..prev_diff["t_x"])
		assert(prev_diff["t_x"] > -1.0e+14, "prev_diff out of bounds: "..prev_diff["t_x"])
		assert(prev_diff["t_y"] < 1.0e+14, "prev_diff out of bounds: "..prev_diff["t_y"])
		assert(prev_diff["t_y"] > -1.0e+14, "prev_diff out of bounds: "..prev_diff["t_y"])

		local dt = os.clock() - old_time
		old_time = os.clock()
		--print("DT : "..dt)
		local pitch_velocity = 0.001*(diff["t_y"] - prev_diff["t_y"])/dt
		local roll_velocity = 0.001*(diff["t_x"] - prev_diff["t_x"])/dt
		--print("Velocities : "..pitch_velocity..", "..roll_velocity)
		local angle_1 = math.deg(math.atan(pitch_velocity, 9.81*0.001*diff["t_y"]))
		local angle_2 = math.deg(math.atan(roll_velocity, 9.81*0.001*diff["t_y"]))
		--print("Angles "..angle_1..","..angle_2)
		local d_diff1 = pitch_velocity - prev_pitch_velocity
		local d_diff2 = roll_velocity - prev_roll_velocity
		--print("D diff "..d_diff1..","..d_diff2)
		prev_pitch_velocity = pitch_velocity
		prev_roll_velocity = roll_velocity

		local vertical = 0.0
		if use_vertical then
			vertical = vertical_pid(diff["t_z"], prev_diff["t_z"], p_coeff["t_z"], d_coeff["t_z"])
		end

		local rotate = 0.0
		if use_rotation then
			rotate = rotation_pid(diff["r_z"], prev_diff["r_z"], p_coeff["r_z"], d_coeff["r_z"])
		end

		local pitch = 0.0
		local roll = 0.0
		if use_horizontal then
			pitch, roll = horizontal_pid(diff, prev_diff, p_coeff, d_coeff)
		end

		print("point "     .. point["t_y"].. " " .. point["t_x"].. " " .. point["t_z"].. " ".. point["r_z"])
		print("diff "      .. diff["t_y"] .. " " .. diff["t_x"] .. " " .. diff["t_z"] .. " ".. diff["r_z"])
		print("[PID] Move ".. pitch       .. " " .. roll        .. " " .. vertical    .. " ".. rotate)
		if roll ~= 0.0 or pitch ~= 0.0 or vertical ~= 0.0 or rotate ~= 0.0 then
			d.move(roll, pitch, vertical, rotate)
			d.stay()
			--print("coords "    ..coords["t_y"].. " " ..coords["t_x"].. " " ..coords["t_z"].. " ".. coords["r_z"])
			--print("DT : "..dt)
			--print("Velocities : "..pitch_velocity..", "..roll_velocity)
			--print("Angles "..angle_1..","..angle_2)
			--print("D_diff "..d_diff1..","..d_diff2)
		else
			-- if we are close to the goal we stop, else drone uses its inertia to move
			if diff["t_y"] < 250 and diff["t_y"] > -250
			and diff["t_x"] < 250 and diff["t_x"] > -250
			then
				reached = 1
			end
		end

		for k,v in pairs(diff) do
			prev_diff[k] = v
		end

		prev_diff["t_x"] = old_t_x
		prev_diff["t_y"] = old_t_y

		return reached
	end
end