--Boucle de contrôle du drone + affichage vidéo

l=require("libjakopter")
l.connect_video()
l.connect()
--Canal de com navdata (lecture des données)
ccn = l.get_cc(1)
--Canal de com de la video pour envoyer les infos
ccv = l.get_cc(2)
--Envoi des navdata au module vidéo
--vérifier le timestamp pour voir si de nouvelles données sont arrivées
--tt = l.get_cc_time(ccv)
while true do
	bat = l.read_int(ccn, 0)
	alt = l.read_int(ccn, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = l.read_float(ccn, 8) / 1000
	roll = l.read_float(ccn, 12) / 1000
	yaw = l.read_float(ccn, 16) / 1000
	--envoi des données à la vidéo
	l.write_int(ccv, 0, bat)
	l.write_int(ccv, 4, alt)
	l.write_float(ccv, 8, pitch)
	l.write_float(ccv, 12, roll)
	l.write_float(ccv, 16, yaw)
end
l.stop_video()
l.disconnect()

