docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild3 bash -c "cd /work/pico; cmake .; make"

mp=/media/`whoami`/RPI-RP2

if [ "$1" == "run" ]; then
	while [ ! -e ${mp} ]; do
		sleep 1
	done
	cp ./pico/lptstore.uf2 ${mp}
fi
