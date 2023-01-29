docker run -ti --rm -v`pwd`:/work --name lpthdpico  picobuild2 bash -c "cd /work/pico; cmake .; make"
