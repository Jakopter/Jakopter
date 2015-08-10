--Boucle de contrôle du drone + affichage vidéo

-- package.cpath = package.cpath .. ";?.dylib"
l=require("libjakopter")
path="../../resources/test.png"
if d.use_visp() then
	print("ViSP doesn't handle yet incrusting")
	os.exit()
end
l.connect_video()
l.connect()
--Canal de com navdata (lecture des données)
--ccn = l.get_cc(1)
--Canal de com de la video pour envoyer les infos
--ccv = l.get_cc(2)
--Envoi des navdata au module vidéo
--vérifier le timestamp pour voir si de nouvelles données sont arrivées
--initialement, il vaut 0.
tt = 0
tt_new = 0
id_bat = -1
id_icon = -1
while true do
	--ne mettre à jour les données que si elles ont changé
	repeat
		--yield pour éviter d'occuper le CPU avec plein de tests inutiles
		l.yield()
		tt_new = l.cc_get_timestamp(1)
		l.usleep(500*1000)
	until tt_new > tt

	tt = tt_new
	bat = l.cc_read_int(1, 0)
	alt = l.cc_read_int(1, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = l.cc_read_float(1, 8) / 1000
	roll = l.cc_read_float(1, 12) / 1000
	yaw = l.cc_read_float(1, 16) / 1000
	--send data to video
	l.cc_write_int(2, 0, bat)
	l.cc_write_int(2, 4, alt)
	l.cc_write_float(2, 8, pitch)
	l.cc_write_float(2, 12, roll)
	l.cc_write_float(2, 16, yaw)
	if yaw > 90 and id_icon == -1 then
		id_icon = l.draw_icon(path, 120, 120, 64, 64)
	elseif yaw < 90 and  id_icon ~= -1 then
		l.draw_remove(id_icon)
		--don't forget to reset id_icon otherway it tries each round to delete the icon
		id_icon = -1
	end


	if (id_bat ~= -1) then
		l.draw_remove(id_bat)
		id_bat = -1
	end
	id_bat = l.draw_text("Yaw : " .. yaw, 0, 120)
end