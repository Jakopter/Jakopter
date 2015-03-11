--Boucle de contrôle du drone + affichage vidéo

l=require("libjakopter")
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
		l.yield
		tt_new = l.cc_get_timestamp(2)
	until tt_new > tt

	tt = tt_new
	bat = l.read_int(1, 0)
	alt = l.read_int(1, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = l.read_float(1, 8) / 1000
	roll = l.read_float(1, 12) / 1000
	yaw = l.read_float(1, 16) / 1000
	--envoi des données à la vidéo
	l.write_int(2, 0, bat)
	l.write_int(2, 4, alt)
	l.write_float(2, 8, pitch)
	l.write_float(2, 12, roll)
	l.write_float(2, 16, yaw)
end
l.stop_video()
l.disconnect()

