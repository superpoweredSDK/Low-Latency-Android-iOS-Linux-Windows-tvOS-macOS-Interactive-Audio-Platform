gcc -o offline1 ./src/offline1.cpp -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86.a
gcc -o offline2 ./src/offline2.cpp -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86.a
gcc -o offline3 ./src/offline3.cpp -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86.a
gcc -o hls      ./src/hls.cpp      -lm -lpthread -lstdc++ -lasound -I../Superpowered ../Superpowered/libSuperpoweredLinuxX86.a
