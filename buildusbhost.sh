docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild3 bash -c "cd /work/usbhost; cmake .; make $1"
