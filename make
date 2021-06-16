cd build
make
if [ "$1" = "run" ]; then
	open DinoHerder.app/Contents/MacOS/DinoHerder
fi
