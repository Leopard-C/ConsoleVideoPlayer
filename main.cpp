/****************************************************
 * Description: Play video in terminal(Linux)
 *      Author: github@Leopard-C
 *      GitHub: Leopard-C/ConsoleVideo
 * Last Change: 2020-04-05
****************************************************/

#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <random>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

void run(const char* filename);

int FRAME_WIDTH = 0;
int FRAME_HEIGHT = 0;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Error, please input filename." << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  console_video [filename]" << std::endl;
        return 1;
    }

    srand(time(NULL));
    
    run(argv[1]);
    return 0;
}


void run(const char* filename) {
    cv::VideoCapture cap;
    cap.open(filename);
    if (!cap.isOpened()) {
        std::cout << "Open file failed" << std::endl;
        return;
    }

    cv::Mat firstFrame;
    cap >> firstFrame;
    if (firstFrame.empty()) {
        std::cout << "Empty frame" << std::endl;
        return;
    }

    // Get console size
    struct winsize wSize;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&wSize);
    int wWidth = wSize.ws_col;
    int wHeight = wSize.ws_row;
    double wAspectRatio = double(wWidth) / wHeight;

    // Get video frame size
    int fWidth = firstFrame.cols;
    int fHeight = firstFrame.rows;
    double fAspectRatio = double(fWidth) / fHeight;

    // Calculate new frame size: Size(FRAME_WIDTH, FRAME_HEIGHT)
    if (wAspectRatio > fAspectRatio) {
        FRAME_HEIGHT = wHeight;
        FRAME_WIDTH = FRAME_HEIGHT * fAspectRatio;
    }
    else {
        FRAME_WIDTH = wWidth;
        FRAME_HEIGHT = FRAME_WIDTH * fAspectRatio;
    }
    cv::Size size(FRAME_WIDTH, FRAME_HEIGHT);

    // Frame rate
    double rate = cap.get(CV_CAP_PROP_FPS);
    int sleepMicroSeconds = 1000000 / rate;

    // Character buffer
    char charBuff[22] = { 0 };
    const int ROW_LEN = FRAME_WIDTH * 3 * 22 + 1;
    char* frameBuff = new char[ROW_LEN * FRAME_HEIGHT];

    // clear screen
    printf("\033[2J");

    int numSkippedFrame = 0;

    auto start = std::chrono::system_clock::now();
    auto end = start;

    while (1) {
        cv::Mat frameOri;
        cv::Mat frameNew;
        cap >> frameOri;
        if (frameOri.empty())
            break;

        if (numSkippedFrame > 0) {
            numSkippedFrame--;
            continue;
        }

        cv::resize(frameOri, frameNew, size,  (0, 0), (0, 0), cv::INTER_LINEAR);

        int offset = 0;

        // Get frame character image
        for (int i = 0; i < FRAME_HEIGHT; i++) {
            const uchar* data = frameNew.ptr<uchar>(i);
            // Each row
            for (int j = 0; j < FRAME_WIDTH; ++j) {
                memset(charBuff, 0, 22);
                int b = data[j * 3];
                int g = data[j * 3 + 1];
                int r = data[j * 3 + 2];
                char ch = rand() % 26 + 97;
                //sprintf(charBuff, "\033[38;2;%d;%d;%dmoo", r, g, b);  // Fixed character: o
                sprintf(charBuff, "\033[38;2;%d;%d;%dm%c%c", r, g, b, ch, ch);  // Random character
                int charBuffLen = strlen(charBuff);
                memcpy(frameBuff + offset, charBuff, charBuffLen);
                offset += charBuffLen;
            }
            if (i < FRAME_HEIGHT - 1)
                frameBuff[offset++] = '\n'; // new line
        }

        frameBuff[offset] = 0;

        // print a whole frame once
        printf("%s", frameBuff);
        fflush(stdout);

        // Sleep some time
        // According to the frame rate and the time of printing character image
        end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // If the font is too small
        // It will spend much time to render
        // And then, we should skip some frames
        if (sleepMicroSeconds < duration) {
            numSkippedFrame = 1;
            while (sleepMicroSeconds * (numSkippedFrame + 1) < duration) {
                ++numSkippedFrame;
            }
        }
        std::cout << " "  << numSkippedFrame;   // Show the number of skipped frames
        auto sleepTime = std::chrono::microseconds(sleepMicroSeconds * (numSkippedFrame + 1) - duration);
        std::this_thread::sleep_for(sleepTime);

        // Start timekeeping here
        //   Instead of the beginnng of while-loop
        start = std::chrono::system_clock::now();

        // clear screen
        //printf("\033[2J");
        
        // move cursor to top-left
        //   instead of clearing the screent
        // Prevent screen flickering
        printf("\033[0;0H");
    }

    delete[] frameBuff;

    getchar();
}

