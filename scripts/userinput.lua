--Boucle de contrôle du drone + affichage vidéo + lecture entrée clavier
-- package.cpath = package.cpath .. ";?.dylib"
l=require("libjakopter")
l.connect_video()
l.connect()
--Canal de com navdata (lecture des données)
ccn = 1
--Canal de com de la video pour envoyer les infos
ccv = 2
-- Cana de com de l'entrée clavier
cck = 4
previous =-1
while true do
	valk1 = l.cc_read_int(cck,0)
	print("valeur valk1",valk1)
	if valk1 ~= previous then
		previous = valk1
		if valk1 == 10 then
			break
		elseif valk1 == 65 then
			l.takeoff()
		elseif valk1 == 66 then
			l.land()
		elseif valk1 == 68 then
			l.left(0.5)
		elseif valk1 == 67 then
			l.right(0.5)
		elseif valk1 == 97 then
-- letter "a" pour arret
			l.stay()
		elseif valk1 == 102 then
-- letter "f" pour forward
			l.forward(0.5)
		elseif valk1 == 98 then
-- letter "b" pour backward
			l.backward(0.5)
		else if valk1 == 100 then
-- letter "d" pour down
			l.down(0.3)
		else if valk1 == 117 then
-- letter "u" pour up
			l.up(0.3)
		end
	end
	bat = l.cc_read_int(ccn, 0)
	alt = l.cc_read_int(ccn, 4)
	--angles en millidegrés, il faut les convertir en degrés.
	pitch = l.cc_read_float(ccn, 8) / 1000
	roll = l.cc_read_float(ccn, 12) / 1000
	yaw = l.cc_read_float(ccn, 16) / 1000
	--envoi des données à la vidéo
	l.cc_write_int(ccv, 0, bat)
	l.cc_write_int(ccv, 4, alt)
	l.cc_write_float(ccv, 8, pitch)
	l.cc_write_float(ccv, 12, roll)
	l.cc_write_float(ccv, 16, yaw)
end

l.disconnect()
l.stop_video()