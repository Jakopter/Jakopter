--Boucle de contrôle du drone + affichage vidéo + lecture entrée clavier

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
cnt = 0
while true do
	key_val = l.cc_read_int(cck, 0)
	if cnt == 100 then
		print("valeur key_val", key_val)
		cnt = 0
	end
	cnt = cnt + 1
	if key_val ~= previous then
		previous = key_val
--escape
		if key_val == 27 then
			break
--up arrow
		elseif key_val == 65 then
			l.takeoff()
--down arrow
		elseif key_val == 66 then
			l.land()
--left arrow
		elseif key_val == 68 then
			l.left(0.5)
--right arrow
		elseif key_val == 67 then
			l.right(0.5)
-- space for pause
		elseif key_val == 32 then
			l.stay()
-- letter "z" pour forward
		elseif key_val == 122 then
			l.forward(0.5)
-- letter "s" pour backward
		elseif key_val == 115 then
			l.backward(0.5)
-- letter "q" pour slide on left
		elseif key_val == 113 then
			l.slide_left(0.5)
-- letter "d" pour slide on right
		elseif key_val == 100 then
			l.slide_right(0.5)
-- letter "r" for up
		elseif key_val == 114 then
			l.up(0.3)
-- letter "f" for down
		elseif key_val == 102 then
			l.down(0.3)
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
l.land()
l.disconnect()
l.stop_video()