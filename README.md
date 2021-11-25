# How to use
```sh
git clone https://github.com/X-rays5/cbsodata_dumper.git --recursive
cd cbsodata_dumper
cmake . -B build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build . --config Release
./cbsodata_dumper <thread count>
```

output will then be store in data/<dataset_name>
