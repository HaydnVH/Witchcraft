set DIR="dependencies"
git clone https://github.com/Microsoft/vkpkg.git "%DIR%/vcpkg"
cd "%DIR%/vcpkg"
bootstrap-vcpkg.bat -disableMetrics
vcpkg integrate install