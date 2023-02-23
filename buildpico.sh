

if [ "$1" == "clean" ]; then
docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild4 bash -c "cd /work/pico; make clean"
else
docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild4 bash -c "cd /work/pico; cmake .; make"
fi
mp=/media/`whoami`/RPI-RP2

if [ "$1" == "run" ]; then
	while [ ! -e ${mp} ]; do
		sleep 1
	done
	cp ./pico/lptstore.uf2 ${mp}
fi
