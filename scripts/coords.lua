drone_velocity = function ()
	return {
		vx = d.cc_read_float(1, 20) / 1000,
		vy = d.cc_read_float(1, 24) / 1000,
		vz = d.cc_read_float(1, 28) / 1000 -- not implemented on ardrone2, use navdata_timestamp and diff
	}
end

drone_coords = function ()
	return {
		--Translations in millimeters
		t_x = 0.0,
		t_y = 0.0,
		t_z = d.cc_read_int(1, 4),
		--Rotations in degrees from -180 to 180, clockwise
		r_x = d.cc_read_float(1, 8) / 1000,
		r_y = d.cc_read_float(1, 12) / 1000,
		r_z = d.cc_read_float(1, 16) / 1000
	}
end

global_coords = function ()
	return {
--Translations in millimeters
		t_x = d.cc_read_float(5, 0),
		t_y = d.cc_read_float(5, 4),
		t_z = d.cc_read_float(5, 8),
-- Rotations in degree from -180 to 180, 0 is aligned with positive y axis, clockwise
		r_x = math.deg(d.cc_read_float(5, 12)),
		r_y = math.deg(d.cc_read_float(5, 16)),
		r_z = -math.deg(d.cc_read_float(5, 20))
	}
end

-- Compute the distance from the start point in angle-coordinates
function local_distance(start_point, coords)
	local t = {}
	for key, value in pairs(coords) do
		if string.find(key,"^t_") then
			t[key] = math.abs(value - start_point[key])
		elseif string.find(key,"^r_") then
			local angle = math.abs(value - start_point[key])
			if angle > 180 then
				angle = 360 - angle
			end
			t[key] = angle
		end
	end
	return t
end

-- Compute the coordinates in a local origin in angle-coordinates metrics.
-- Angles are comprised between -180 and 180, clockwise from the camera
function local_coords(start_point, coords)
	local loc = {}
	for key, value in pairs(coords) do
		loc[key] = value - start_point[key]
	end
	return loc
end