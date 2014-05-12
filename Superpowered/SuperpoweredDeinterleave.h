/**
 Deinterleaves a stereo input buffer into two buffers.
 
 @param input The input buffer.
 @param outputLeft The output buffer for the left channel.
 @param outputRight The output buffer for the right channel.
 @param frames How many frames (or samples) to process.
 */
void SuperpoweredDeinterleave(float *input, float *outputLeft, float *outputRight, int frames);
