docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild2 bash -c "cd /work/pico; cmake .; make"

if [ "$1" == "run" ]; then
	while [ ! -e /media/matt/RPI-RP2 ]; do
		sleep 1
	done
	cp ./pico/lptstore.uf2 /media/matt/RPI-RP2/
fi
