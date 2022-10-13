
build:
	make -C cmake-build-debug

run:stop build
	sudo cmake-build-debug/zipfs -d -f -o umask=0002 -o uid=1003 -o gid=1003 -o allow_other mnt --zipfile test/data.zip
stop:
	sudo killall  zipfs || echo "stop"
	sudo umount ./mnt/ || echo ""
