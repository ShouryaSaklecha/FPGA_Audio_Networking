#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

// to deal with overspeed, we reduce the sample rate for playing back
#define SAMPLE_RATE 40000 // we're using 44.1kHz sample rate
#define NUM_CHANNELS 1 // number of channels - we have mono sound
#define BITS_PER_SAMPLE 24 // we have 24 bits per sample but in reality we only use 18 (only 18 bits valid)

/*
    Writes bytes into a file in little endian format
*/
void write_little_endian(uint32_t word, int num_bytes, FILE *wav_file)
{
    unsigned buf;
    while(num_bytes>0) {
        buf = word & 0xff; // extract the least significant byte from the word
        fwrite(&buf, 1,1, wav_file);
        num_bytes--;
        word >>= 8; // move the next least significant byte down to the least significant bytes position
    }
}

/**
 * @brief
 *      this function reversed the given 32 bits number for 24 times (reverse and save the last 24 bits)
 *      and store the reversed number into the give file when the number is not 0
 *      we ignore 0 at here to avoid electronic noise
 * @param word the 32 bits sample we get from the i2s
 * @param wav_file the output wavefile we want to create
 */
void write_backward_24(uint32_t word, FILE *wav_file) {
    uint32_t real = 0;
    uint32_t temp = 0;
    for(int i = 0; i < 24; i++) {
        real = real << 1;
        temp = word & 1; // the LSB of the word
        real = real + temp;
        word = word >> 1;
    }
    if (real != 0) {
        fwrite(&real, 3, 1, wav_file);
    }
}

int main(void){
    // create variables for UDP socket and package storage
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[2000], client_message[2000];
    uint32_t wavefile_data_recv[512]; // where we store the data we received
    socklen_t server_struct_length = sizeof(server_addr);
    ssize_t status = 0;

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    memset(wavefile_data_recv, '\0', sizeof(wavefile_data_recv));

    //////////////////// create file & write the header ////////////////////
    FILE *wav_file = fopen("filename.wav", "wb");
    // Audio information
    unsigned int sample_rate = SAMPLE_RATE;
    unsigned int num_channels = NUM_CHANNELS;
    unsigned int bits_per_sample = BITS_PER_SAMPLE;

    // Wav file information
    unsigned int byte_rate = (sample_rate * num_channels * bits_per_sample) / 8;
    unsigned int chunk_size_var_sec = (bits_per_sample / 8) * 1 * SAMPLE_RATE * num_channels; // the variation of chunksize per second

    unsigned int chunk_size = 36 + (bits_per_sample / 8) * 1 * SAMPLE_RATE * num_channels;
    unsigned int sub_chunk_1 = 16;
    unsigned int sub_chunk_2 = (bits_per_sample / 8) * 1 * SAMPLE_RATE * num_channels;

    // RIFF header
    fwrite("RIFF", 1, 4, wav_file); // ChunkID
    write_little_endian(chunk_size, 4, wav_file); // ChunkSize - 4 for fseek
    fwrite("WAVE", 1, 4, wav_file); // Format

    // fmt subchunk
    fwrite("fmt ", 1, 4, wav_file); // SubChunk1ID
    write_little_endian(sub_chunk_1, 4, wav_file); // SubChunk1Size
    write_little_endian(1, 2, wav_file); // PCM = 1
    write_little_endian(num_channels, 2, wav_file); // NumChannels
    write_little_endian(sample_rate, 4, wav_file); // SampleRate
    write_little_endian(byte_rate, 4, wav_file); // ByteRate (SampleRate * NumChannels * BitsPerSample) / 8
    write_little_endian(num_channels * bits_per_sample / 8, 2, wav_file); // BlockAlign
    write_little_endian(bits_per_sample, 2, wav_file); // BitsPerSample

    // data subchunk
    fwrite("data", 1, 4, wav_file); // SubChunk2ID
    write_little_endian(sub_chunk_2, 4, wav_file); // SubChunk2Size - 40 for fseek
    //////////////////// END OF create file & write the header ////////////////////

    //////////////////// create socket and setup for connection ////////////////////
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.149");

    // Get input from the user:
    printf("Enter message: ");
    fgets(client_message, sizeof(client_message), stdin);
    printf("size:%ld\n", sizeof(client_message));
    // Send the message to server:
    if(sendto(socket_desc, client_message, strlen(client_message), 0,
         (struct sockaddr*)&server_addr, server_struct_length) < 0){
        printf("Unable to send message\n");
        return -1;
    }

    // Receive the server's response:
    if(recvfrom(socket_desc, server_message, sizeof(server_message), 0,
         (struct sockaddr*)&server_addr, &server_struct_length) < 0){
        printf("Error while receiving server's msg\n");
        return -1;
    }
    //////////////////// connection established, receive mic data begin ////////////////////

    ///////////////////////////////////////////////
    // start receiving raw data from the server
    ///////////////////////////////////////////////

    long counter = 0;

    while(1) {
        if (counter % 175 == 0) {
            // means we received enough for 1s, need to change the header for an extra sec.
            printf("continuously receiving %ld!\n", counter / 175);

            chunk_size = chunk_size + chunk_size_var_sec;
            sub_chunk_2 = sub_chunk_2 + chunk_size_var_sec;
            // update the chunk size
            fseek(wav_file, 4, SEEK_SET);
            write_little_endian(chunk_size, 4, wav_file);
            // update the subchunk2 size
            fseek(wav_file, 40, SEEK_SET);
            write_little_endian(sub_chunk_2, 4, wav_file);

            fseek(wav_file, 0, SEEK_END); // go back to the end of the file for continuous writting
        }

        // get mic data from the server(board)
        status = recvfrom(socket_desc, wavefile_data_recv, 2048, 0,
        (struct sockaddr*)&server_addr, &server_struct_length);

        // check receiving issues
        if (status < 0) {
            printf("error of transfer");
            break;
        }
        if (status == 0) {
            printf("end of receiving");
            break;
        }

        // proceed the received data and write it into the wav file (if data != 0)
        for(int i = 0; i < 512; i++) {
            write_backward_24((unsigned int) wavefile_data_recv[i], wav_file);
        }

        counter++;
    }

    // Close the socket:
    close(socket_desc);
    fclose(wav_file);
    return 0;
}