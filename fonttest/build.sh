clang++ -std=c++11 *.cc -o fonttest \
-I/Users/nona/homebrew/Cellar/goocanvas/2.0.2/include/goocanvas-2.0/ \
`/Users/nona/homebrew/bin/pkg-config gtk+-3.0 --cflags --libs` \
-L/Users/nona/homebrew/lib/ \
-lglog \
-lgoocanvas-2.0
