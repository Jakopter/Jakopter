--Boucle de contrôle du drone + affichage vidéo

l=require("libjakopter")
path="../../resources/test.png")
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
while true do
	--ne mettre à jour les données que si elles ont changé
	repeat
		--yield pour éviter d'occuper le CPU avec plein de tests inutiles
		l.yield()
		tt_new = l.cc_get_timestamp(1)
	until tt_new > tt

	tt = tt_new
	bat = l.cc_read_int(1, 0)
	alt = l.cc_read_int(1, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = l.cc_read_float(1, 8) / 1000
	roll = l.cc_read_float(1, 12) / 1000
	yaw = l.cc_read_float(1, 16) / 1000
	--envoi des données à la vidéo
	l.cc_write_int(2, 0, bat)
	l.cc_write_int(2, 4, alt)
	l.cc_write_float(2, 8, pitch)
	l.cc_write_float(2, 12, roll)
	l.cc_write_float(2, 16, yaw)
	l.draw_icon(path, x, y)
	-- l.draw_text("Batterie : " .. bat)
end

