#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_BROKER "34.83.74.176"
#define PORT "1883"
#define MOVESEND "esp32/led"
#define BOARD "esp32/board"
#define ERROR "esp32/error"
#define TURNORDER "esp32/gameinfo"
#define MENU "esp32/menu"

void publish(const char* topic,  const char* payload) {
    char command[256];
    snprintf(command, sizeof(command), "mosquitto_pub -h %s -p %s -t %s -m %s", DEFAULT_BROKER, PORT, topic, payload);
    system(command);
}
void gamemove(char board[3][3]) {
    char universe[1024];
    char buffer[1024];
    FILE *fp;
    snprintf(universe, sizeof(universe), "mosquitto_sub -h %s -p %s -t %s -C 1", DEFAULT_BROKER, PORT, BOARD);
    while (1) {
        fp = popen(universe, "r");

        if (fp == NULL) {
            perror("Failed to open file");
            printf("failed");
        }
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        
            if (strlen(buffer) > 0) {
                int ind = 0;
                printf("Received data: '%s'\n", buffer);
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        if (buffer[ind] != ' ') {  // Ignore spaces in the buffer
                            board[i][j] = buffer[ind];
                        }
                        else {
                            board[i][j] = ' ';
                        }
                        ind++;  // Move to the next character in the buffer
                    }
                }
            }
        }
        else {
        }
        pclose(fp);
        return;
    }
}
void printboard(char board[3][3]) {
    printf("--1---2---3--\n1");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf(" %c ", board[i][j]);
            if (j < 2) printf("|");
        }
        printf("\n");
        if (i == 0) printf("----+---+---\n2");
        if (i == 1) printf("----+---+---\n3");
    }
}

int sendMove(char player, int row, int col) {
    char message[50];
    snprintf(message, sizeof(message), "%c,%d,%d", player, row, col);
    publish(MOVESEND, message);
    char command[256];
    snprintf(command, sizeof(command), "mosquitto_sub -h %s -p %s -t %s -C 1", DEFAULT_BROKER, PORT, ERROR);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return 0;
    }
    const char *keyword = "ERROR";

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strstr(buffer, keyword) != NULL) {
            pclose(fp);
            return 0;
        }
    }
    pclose(fp);

    return 1;
}

char gameCheck() {
    FILE *fp;
    char command[256];
    char buffer[256];
    snprintf(command, sizeof(command), "mosquitto_sub -h %s -p %s -t %s -C 1", DEFAULT_BROKER, PORT, TURNORDER);
    fp = popen(command, "r");
    while (fp == NULL) {
        snprintf(command, sizeof(command), "mosquitto_sub -h %s -p %s -t %s -C 1", DEFAULT_BROKER, PORT, TURNORDER);
        fp = popen(command, "r");
    }

    const char *win = "Winner";
    const char *player1 = "Player X";
    const char *player2 = "Player O";
    if (fp == NULL) {
        perror("Failed to open file");
        return 'X';
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strstr(buffer, win) != NULL) {
            printf("%s\n", buffer);
            exit(0);
        }
        else if (strstr(buffer, player1) != NULL) {
            int row = buffer[28] - '0';
            int col = buffer[41] - '0';
            printf("Player O placed a O on row %d and column %d.\n", row, col);
            return 'X';
        }
        else if (strstr(buffer, player2) != NULL) {
            int row = buffer[28] - '0';
            int col = buffer[41] - '0';
            printf("Player X placed a X on row %d and column %d.\n", row, col);
            return 'O';
        }
    }
}

void clearBoard(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';  // Use space to represent empty cell
        }
    }
}

int main(int argc, char *argv[]) {
    char board[3][3];
    clearBoard(board);
    int game = 0;
    int row = 0;
    int col = 0;
    char turn = 'X';
    int menu;
    int cube = 0;
    char turn1 = 'X';
    int num = 0;

    printf("Menu options\n1: Play against COM\n2: Two Player Mode\n3: Two Bots Go head to head\nPlease type in option now: ");
    scanf("%d", &menu);
    char buffer[50];
    sprintf(buffer, "%d", menu);
    publish (MENU, buffer);
    if (menu == 1) {
        while (game == 0) {
            while(num == 0){
            publish (MENU, buffer);
            char command[256];
            snprintf(command, sizeof(command),"mosquitto_sub -h %s -p %s -t %s", DEFAULT_BROKER, PORT, "esp32/log");

            FILE *fp = popen(command, "r");
                if (fp == NULL) {
                perror("popen failed");
            }
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                const char* fuck = "Done";
                if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    if (strstr(buffer, fuck) != NULL) {
                        num = 1;
                        pclose(fp);
                    }
                }
            } 
        }  
            if (cube == 1){
                gamemove(board);
            }
            printboard(board);
            int move = 0;
            while (move == 0) {
                printf("\nPlayer X, what row do you want to place: ");
                scanf("%d", &row);
                printf("\nPlayer X, what column do you want to place: ");
                scanf("%d", &col);
                move = sendMove(turn, row - 1, col - 1);
            }
            cube = 1;
            do {
                turn = gameCheck();
            } while (turn != 'X');
              
        }
    }
    else if (menu == 2) {
        while (game == 0) {
            if (cube == 1){
                gamemove(board);
            }
            printboard(board);
            turn1 = turn;
            int move = 0;
            while (move == 0) {
                printf("\nPlayer %c, what row do you want to place: ", turn);
                scanf("%d", &row);
                printf("\nPlayer %c, what column do you want to place: ", turn);
                scanf("%d", &col);
                move = sendMove(turn, row - 1, col - 1);
            }
            cube = 1;
            while (turn == turn1){
            turn = gameCheck();
            }
        }
    }
    else if (menu == 3) {
        while (game == 0) {
            while(num == 0){
                char command[256];
                snprintf(command, sizeof(command),"mosquitto_sub -h %s -p %s -t %s -C 1 -W 5", DEFAULT_BROKER, PORT, "esp32/log");
                FILE *fp = popen(command, "r");
                    if (fp == NULL) {

                }
                printf("test");
                char buffer[256];
                const char* fuck = "Done";
                if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    if (strstr(buffer, fuck) != NULL) {
                        num = 1;
                        pclose(fp);
                    }
                } 
            }  
                if (cube == 1){
                    gamemove(board);
                }
            turn1 = turn;
            printboard(board);
            if (turn == 'X') {
                printf("\nIt's X's turn.\n");
            }
            else if (turn == 'O') {
                printf("\nIt's O's turn.\n");
            }
            cube = 1;
            while (turn == turn1){
                turn = gameCheck();
            }
        }
    }
    else {
        printf("Sorry, invalid menu option. Goodbye.\n");
    }
    printf("End");

    return 0;
}
