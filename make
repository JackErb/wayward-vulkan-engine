cd build
make

if [ "$1" = "shaders" ]; then
    python3 ../compile_shaders.py
fi

if [ "$1" = "run" ]; then
    python3 ../compile_shaders.py
	open DinoHerder.app/Contents/MacOS/DinoHerder
fi
