g++ -m32 -std=c++11 loader.cc -o loader -g -Wl,-Ttext-segment=0x2000000 -ldl
gcc -m32 factorial.c -o factorial
gcc -m32 helloworld.c -o helloworld
