cd build
rm -r DinoHerder.app
python3 ../compile_shaders.py
make

if [ "$1" = "run" ]; then
    open DinoHerder.app/Contents/MacOS/DinoHerder
fi
