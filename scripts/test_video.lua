-- package.cpath = package.cpath .. ";?.dylib"
d = require("libjakopter")

d.connect()
d.connect_video()

tt = 0
tt_new = 0
while true do
	--ne mettre à jour les données que si elles ont changé
	repeat
		--yield pour éviter d'occuper le CPU avec plein de tests inutiles
		d.yield()
		tt_new = d.cc_get_timestamp(1)
		d.usleep(1000*1000)
	until tt_new > tt

	tt = tt_new
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
end
