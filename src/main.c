#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/poll.h>
#include <stdlib.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

float A = 0, B = 0, C = 0;
float cubeWidth = 10;
int width = 80, height = 40;
float zBuffer[80 * 40];
char buffer[80 * 40];
char* colorBuffer[80 * 40];
float K1 = 40;
const int distanceFromCamera = 50;
float incrementSpeed = 0.2;

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // Turn off echoing and line-by-line reading
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


float calculateX(float i, float j, float k) {
    return j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
           j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
}

float calculateY(float i, float j, float k) {
    return j * cos(A) * cos(C) + k * sin(A) * cos(C) -
           j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) -
           i * cos(B) * sin(C);
}

float calculateZ(float i, float j, float k) {
    return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
}

void renderPoint(float cubeX, float cubeY, float cubeZ, char ch, char* color) {
    float x = calculateX(cubeX, cubeY, cubeZ);
    float y = calculateY(cubeX, cubeY, cubeZ);
    float z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCamera;
    float ooz = 1 / z;
    int xp = (int)(width / 2 + K1 * ooz * x * 2.0);
    int yp = (int)(height / 2 + K1 * ooz * y);
    int idx = xp + yp * width;
    if (idx >= 0 && idx < width * height) {
        if (ooz > zBuffer[idx]) {
            zBuffer[idx] = ooz;
            buffer[idx] = ch;
            colorBuffer[idx] = color;
        }
    }
}

int main(int argc, char *argv[]) {
    int controlMode = 0;
    if (argc > 1 && strcmp(argv[1], "--control") == 0) {
        controlMode = 1;
        enableRawMode();
    }

    printf("\x1b[2J");
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };

    while (1) {
        memset(buffer, ' ', width * height);
        for(int i=0; i < width*height; i++) zBuffer[i] = 0;

        for (float v = -cubeWidth; v < cubeWidth; v += incrementSpeed) {
            for (float h = -cubeWidth; h < cubeWidth; h += incrementSpeed) {
                renderPoint(v, h, cubeWidth, '@', RED);
                renderPoint(v, h, -cubeWidth, '.', BLUE);
                renderPoint(cubeWidth, v, h, '$', GREEN);
                renderPoint(-cubeWidth, v, h, '~', YELLOW);
                renderPoint(v, cubeWidth, h, '#', MAGENTA);
                renderPoint(v, -cubeWidth, h, ';', CYAN);
            }
        }

        printf("\x1b[H");
        for (int k = 0; k < width * height; k++) {
            if (k % width == 0) putchar(10);
            if (buffer[k] != ' ') {
                printf("%s%c%s", colorBuffer[k], buffer[k], RESET);
            } else {
                putchar(' ');
            }
        }

        if (controlMode) {
            // Check for key press (10ms timeout)
            if (poll(&pfd, 1, 10) > 0) {
                char c;
                read(STDIN_FILENO, &c, 1);
                if (c == '\x1b') { // Arrow keys start with ESC [
                    char seq[2];
                    if (read(STDIN_FILENO, &seq[0], 1) == 0) break;
                    if (read(STDIN_FILENO, &seq[1], 1) == 0) break;
                    if (seq[0] == '[') {
                        if (seq[1] == 'A') A -= 0.1; // Up
                        if (seq[1] == 'B') A += 0.1; // Down
                        if (seq[1] == 'C') B += 0.1; // Right
                        if (seq[1] == 'D') B -= 0.1; // Left
                    }
                } else if (c == 'q') break; // Quit
            }
        } else {
            // Auto-rotation
            A += 0.05;
            B += 0.05;
            C += 0.01;
            usleep(16000);
        }
    }
    return 0;
}