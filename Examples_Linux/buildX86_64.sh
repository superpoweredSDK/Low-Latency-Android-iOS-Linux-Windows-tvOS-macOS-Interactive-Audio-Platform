gcc -o offline1 ./src/offline1.cpp -lpthread -lstdc++ -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86_64.a -lm
gcc -o offline2 ./src/offline2.cpp -lpthread -lstdc++ -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86_64.a -lm
gcc -o offline3 ./src/offline3.cpp -lpthread -lstdc++ -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86_64.a -lm
gcc -o hls ./src/hls.cpp -lpthread -lstdc++ -lasound -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86_64.a -lm

