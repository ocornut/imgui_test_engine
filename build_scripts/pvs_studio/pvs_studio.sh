#!/usr/bin/env sh
cd $(dirname $(realpath $0))
cd ../../../imgui/examples/example_null
make clean
pvs-studio-analyzer trace -- make EXTRA_WARNINGS=1 -j$(nproc --all)
pvs-studio-analyzer analyze -e ../../imstb_rectpack.h -e ../../imstb_textedit.h -e ../../imstb_truetype.h -l ../../../pvs-studio.lic -o pvs-studio.log
plog-converter -a 'GA:1,2;OP:1' -t errorfile -w pvs-studio.log
make clean
rm -f pvs-studio.log strace_out
