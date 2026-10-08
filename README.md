Stanford CS 144 Networking Lab
==============================

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`

To Pull Branch: 
```markdown
# 拉取 upstream 最新分支信息
git fetch upstream

# 基于 upstream/check1-startercode 创建本地分支 check1-startercode，并切过去
git checkout -b check1-startercode upstream/check1-startercode

# 第一次推送到你的 origin，并建立跟踪
git push -u origin check1-startercode
```