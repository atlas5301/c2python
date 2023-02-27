# c2python

convert c code to python

how to use?

STEP1. Install llvm and clang:(this step may vary on different versions of clang and llvm)

```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build && cd build
cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON   # Enable tests; default is off.
ninja
ninja check
ninja clang-test
ninja install
```

STEP2. clone this repo in folder(assuming you are still in build folder)

```
cd ../clang-tools-extra
git clone https://github.com/atlas5301/c2python.git
echo 'add_subdirectory(c2python)' >> clang-tools-extra/CMakeLists.txt
cd ../build
ninja

```

STEP3. now you can use the tool to convert your c++ code to python code

for example:(this example print converted code from ~/test-files/simple-loops.cc to stdout)

```
bin/loop-convert ~/test-files/simple-loops.cc
```
