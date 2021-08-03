#include <check.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

#include "../../include/util.h"
#include "../../include/filenames.h"

/*while (len_remaining > 0) {
        uint8_t len;
        if (len_remaining > 255) {
            len_remaining -= 255;
            len = 255;
        }
        else {
            len = len_remaining;
            len_remaining = 0;
        }
        void* temp_buffer = malloc(len);
        uint8_t disp = test_length - len_remaining;
        memcpy(temp_buffer, send_buffer + disp, len);
        sendAll(sender, temp_buffer, &len);
}*/

void setupTestSockets(int* recver, int* sender) {
    int listener = setupSocket(1, NULL);
    const char localhost[9] = {"127.0.0.1"};
    *sender = setupSocket(0, localhost);
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    *recver = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
}

void performSendAllTest(uint8_t test_length, char* data, int recver, int sender) {
    void* send_buffer = malloc(test_length);
    void* send_buffer_copy = malloc(test_length);
    memcpy(send_buffer, data, test_length);
    memcpy(send_buffer_copy, send_buffer, test_length);
    uint8_t len = test_length;
    sendAll(sender, send_buffer, &len);
    void* recv_buffer = malloc(test_length);
    int bytes_recv = recv(recver, recv_buffer, test_length, 0); //should recieve all at once since this is local
    ck_assert_msg(
        memcmp(send_buffer_copy, recv_buffer, test_length) == 0,
        "sendAll() failed to send correct packet to socket\n"
    );
    ck_assert_int_eq(len, test_length);
    ck_assert_int_eq(bytes_recv, test_length);
    close(sender);
    close(recver);
    free(send_buffer_copy);
    free(recv_buffer);
}

void performRecvAllTest(uint8_t test_length, char* data, int recver, int sender) {
    void* send_buffer = malloc(test_length);
    void* send_buffer_copy = malloc(test_length);
    memcpy()
}

START_TEST (test_recvAll_small) {
    int recver, sender;
    setupTestSockets(&recver, &sender);

} END_TEST

START_TEST (test_sendAll_small) {
    int recver, sender;
    setupTestSockets(&recver, &sender);
    char* small_str = "smallstring";
    performSendAllTest(strlen(small_str), small_str, recver, sender);
} END_TEST

START_TEST (test_sendAll_large) {
    int recver, sender;
    setupTestSockets(&recver, &sender);
    uint8_t test_size = 255;
    char large_str[255];
    memset(large_str, 65, test_size);
    performSendAllTest(test_size, large_str, recver, sender);
} END_TEST

START_TEST (test_constructPacket) {
    const uint8_t test_size = 20;
    const char* str = "thisisafilenameyes";
    const char expected_output[20] = {
        "\x14Gthisisafilenameyes"
    };
    char tag = 'G';
    void* packet = malloc(test_size);
    constructPacket(test_size, &tag, (void*)str, &packet);
    ck_assert_msg(
        memcmp((const void*)expected_output, packet, test_size) == 0,
        "constructPacket() failed to construct packet correctly\n"
    );
} END_TEST

Suite* sendSuite(void) {
    Suite* s = suite_create("Send");
    TCase* tc_core;
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_constructPacket);
    tcase_add_test(tc_core, test_sendAll_small);
    tcase_add_test(tc_core, test_sendAll_large);
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite* s = sendSuite();
    SRunner* sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}