apt-get update
apt-get install build-essential tar curl zip unzip git gcc gdb rsync ninja-build libx11-dev libxft-dev libxext-dev
DIR="dependencies"
git clone https://github.com/Microsoft/vkpkg.git "$DIR/vcpkg"
cd "$DIR/vcpkg"
./bootstrap-vcpkg.sh -disableMetrics
./vcpkg integrate install