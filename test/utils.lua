-- This file was adapted from ViSP project, a library for visual servoing.
-- Copyright 2014-2014 the ViSP authors by INRIA.
-- Copyright (C) 2014 ViSP authors at INRIA <http://www.irisa.fr/lagadic/visp/visp.html>
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 2 of the License, or
-- (at your option) any later version.
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- Contact: <http://github.com/Jakopter/Jakopter>

axis = {
	"t_x",
	"t_y",
	"t_z",
	"r_x",
	"r_y",
	"r_z"
}

quaternion = {
	"w",
	"x",
	"y",
	"z"
}

--An iterator for tables
function iterator(t)
	local i = 0
	local n = #t
	return function ()
		i = i+1
		if (i <= n) then
			return t[i]
		end
	end
end


--convert values in (angles,translation)-> homogeneous matrix
function angles_to_homogeneous_matrix(coords)
	-- Based on ViSP rotation matrix building under GPL
	-- The last line of the matrix is ignored, it is always supposed to be 0,0,0,1
	local cos_x = math.cos(math.rad(coords["r_x"]))
	local cos_y = math.cos(math.rad(coords["r_y"]))
	local cos_z = math.cos(math.rad(coords["r_z"]))
	local sin_x = math.sin(math.rad(coords["r_x"]))
	local sin_y = math.sin(math.rad(coords["r_y"]))
	local sin_z = math.sin(math.rad(coords["r_z"]))
	return {
		{
			cos_y*cos_z,
			-cos_y*sin_z,
			sin_y,
			coords["t_x"]
		},
		{
			cos_x*sin_z + sin_x*sin_y*cos_z,
			cos_x*cos_y - sin_x*sin_y*sin_z,
			-sin_x*cos_y,
			coords["t_y"]
		},
		{
			-cos_x*sin_y*cos_z - sin_x*sin_z,
			cos_x*sin_y*sin_y + cos_z*sin_x,
			cos_x*cos_y,
			coords["t_z"]
		}
	}
end


-- 4*3 and 3*1
function mult_TransformMatrix_Vector(matrix, vector)
	local ret = {}
	for i=1,3 do
		ret[i] = 0
		for j=1,3 do
			ret[i] = ret[i] + vector[j]*matrix[i][j]
		end
		ret[i] = ret[i] + matrix[i][4]
	end
	return ret
end

function homogeneous_matrix_to_angles(matrix)
	-- Based on ViSP eulerRxyz building under GPL
	local coeffMinRot = 1e-6
	local phi

	if math.abs(matrix[2][3]) < coeffMinRot and math.abs(matrix[3][3]) < coeffMinRot then
		phi = 0
	else
		phi = math.atan(-matrix[2][3], matrix[3][3])
	end
	local sin = math.sin(phi)
	local cos = math.cos(phi)
	local theta = math.atan(matrix[1][3], -sin*matrix[2][3] + cos*matrix[3][3])
	local psi = math.atan(cos*matrix[2][1] + sin*matrix[3][1], cos*matrix[2][2] + sin*matrix[3][2])

	return {
		t_x = matrix[1][4],
		t_y = matrix[2][4],
		t_z = matrix[3][4],
		r_x = phi,
		r_y = theta,
		r_z = psi
	}
end

function rotation_matrix_to_thetaUV(matrix)
	-- Based on ViSP thetaUV building under GPL
	local s = (matrix[2][1]-matrix[1][2])*(matrix[2][1]-matrix[1][2])
		+ (matrix[3][1]-matrix[1][3])*(matrix[3][1]-matrix[1][3])
		+ (matrix[3][2]-matrix[2][3])*(matrix[3][2]-matrix[2][3])
	s = math.sqrt(s) / 2.0
	local c = (matrix[1][1]+matrix[2][2]+matrix[3][3]-1.0)/2.0
	local theta = math.atan(s, c) --theta in [0, PI] since s > 0

	local r1
	local r2
	local r3
	-- General case when theta != pi. If theta=pi, c=-1
	if 1 + c > 1e-4 then -- Since -1 <= c <= 1, no math.abs(1+c) is required
		local sinc
		if math.abs(theta) < 1e-6 then
			sinc = 1.0
		else
			sinc = s / theta
		end

		r1 = (matrix[3][2]-matrix[2][3])/(2*sinc)
		r2 = (matrix[1][3]-matrix[3][1])/(2*sinc)
		r3 = (matrix[2][1]-matrix[1][2])/(2*sinc)
	else -- theta near PI
		if matrix[1][1]-c < 1e-5 then
			r1 = 0.0
		else
			r1 = theta*math.sqrt((matrix[1][1]-c)/(1-c))
		end
		if matrix[3][2]-matrix[2][3] < 0 then
			r1 = -r1
		end

		if matrix[2][2]-c < 1e-5 then
			r1 = 0.0
		else
			r1 = theta*math.sqrt((matrix[2][1]-c)/(1-c))
		end

		if matrix[1][3]-matrix[3][1] < 0 then
			r2 = -r2
		end

		if matrix[3][3]-c < 1e-5 then
			r1 = 0.0
		else
			r1 = theta*math.sqrt((matrix[3][1]-c)/(1-c))
		end

		if matrix[2][1]-matrix[1][2] <  0 then
			r3 = -r3
		end
	end

	return {
		r1,
		r2,
		r3
	}

end

function homogeneous_matrix_to_quaternion(matrix)
	-- Based on ViSP quaternion building under GPL
	local thetaUV = rotation_matrix_to_thetaUV(matrix)
	local theta = math.sqrt(thetaUV[1]*thetaUV[1] + thetaUV[2]*thetaUV[2] + thetaUV[3]*thetaUV[3])

	if (math.abs(theta) < 1e-5) then
		return {x=0.0, y=0.0, z=0.0, w=0.0}
	end
	thetaUV = {thetaUV[1]/theta, thetaUV[2]/theta, thetaUV[3]/theta}

	theta = theta*0.5
	local sin_theta = math.sin(theta)

	return {
		x = thetaUV[1]*sin_theta,
		y = thetaUV[2]*sin_theta,
		z = thetaUV[3]*sin_theta,
		w = math.cos(theta)
	}
end

function quaternion_to_homogeneous_matrix(quaternion)
	-- Based on ViSP rotation matrix building under GPL
	local w = quaternion["w"]
	local x = quaternion["x"]
	local y = quaternion["y"]
	local z = quaternion["z"]

	return {
		{
			w*w + x*x + y*y + z*z,
			2*x*y - 2*w*z,
			2*w*y + 2*x*z
		},
		{
			2*w*z + 2*x*y,
			w*w - x*x + y*y - z*z,
			2*y*z - 2*w*x
		},
		{
			2*x*z - 2*w*y,
			2*w*x + 2*y*z,
			w*w - x*x - y*y + z*z
		}
	}
end

function angles_to_quaternion(coords)
	return homogeneous_matrix_to_quaternion(angles_to_homogeneous_matrix(coords))
end

function quaternion_to_angles(quaternion)
	return homogeneous_matrix_to_angles(quaternion_to_homogeneous_matrix(quaternion))
end