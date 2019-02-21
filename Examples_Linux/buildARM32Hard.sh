gcc -o offline1 ./src/offline1.cpp -mfloat-abi=hard -mfpu=neon -DHAVE_NEON=1 -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxARM32Hard.a
gcc -o offline2 ./src/offline2.cpp -mfloat-abi=hard -mfpu=neon -DHAVE_NEON=1 -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxARM32Hard.a
gcc -o offline3 ./src/offline3.cpp -mfloat-abi=hard -mfpu=neon -DHAVE_NEON=1 -lm -lpthread -lstdc++          -I../Superpowered ../Superpowered/libSuperpoweredLinuxARM32Hard.a
gcc -o hls      ./src/hls.cpp      -mfloat-abi=hard -mfpu=neon -DHAVE_NEON=1 -lm -lpthread -lstdc++ -lasound -I../Superpowered ../Superpowered/libSuperpoweredLinuxARM32Hard.a
