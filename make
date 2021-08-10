cd build
rm -r WaywrdVK.app
python3 ../compile_shaders.py
make

if [ "$1" = "run" ]; then
    open WaywardVK.app/Contents/MacOS/WaywardVK
fi
