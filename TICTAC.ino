#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>  // Include Wire library for I2C communication

// WiFi and MQTT Broker credentials
const char* ssid = "Beans";          // Replace with your WiFi SSID
const char* password = "1239874650";  // Replace with your WiFi password
const char* mqtt_server = "34.83.74.176";  // Replace with your MQTT Broker IP
bool gameCountPublished = false;

WiFiClient espClient;
PubSubClient client(espClient);

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust the I2C address if necessary

// Game variables
char board[3][3];  // 3x3 Tic-Tac-Toe board
int winsX = 0, winsO = 0, games = 0;
int draws = 0;
bool gameRunning = false;
char currentPlayer = 'X';  // Start with 'X' (randomized below)
int row = 0;
int col = 0;

// Function to clear the board
void clearBoard() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      board[i][j] = ' ';
}

void sendBoardState() {
  char boardStr[10];  // 9 characters + null terminator
  int idx = 0;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      boardStr[idx++] = board[i][j];
    }
  }
  boardStr[9] = '\0';  // Null-terminate the string

  client.publish("esp32/board", boardStr);
}

// Function to check for a winner
char checkWin() {
  // Check rows and columns
  for (int i = 0; i < 3; i++) {
    if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
      return board[i][0];
    if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
      return board[0][i];
  }
  // Check diagonals
  if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
    return board[0][0];
  if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
    return board[0][2];

  return ' ';  // No winner yet
}

bool checkDraw() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (board[i][j] == ' ') {
        return false;  // Empty space found, game is not a draw
      }
    }
  }
  return true;  // Board is full and no winner, it's a draw
}

// Callback function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to a string
  char message[20];
  memcpy(message, payload, length);
  message[length] = '\0';

  // Parse the message to get player move (e.g., "X,1,2")
  char player = message[0];
  row = message[2] - '0';  // Convert from '0', '1', '2' to int
  col = message[4] - '0';

  // Check if the space is already occupied
  if (board[row][col] != ' ') {
    // Send error message to the error topic with player's turn
    String errorMessage = "ERROR: Player " + String(currentPlayer) + " attempted to move to (" + String(row) + "," + String(col) + ") but it's already occupied. Try again!";
    client.publish("esp32/error", errorMessage.c_str());

    // Notify the BASH script to try again
    Serial.println(errorMessage);
    return;
  }

  // Place the move if the space is free
  board[row][col] = player;

  // Send board to console
  delay(3000);

  
  client.publish("esp32/error", "All Good");

  // Update the LCD screen with scores
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("X: ");
  lcd.print(winsX);
  lcd.print(" O: ");
  lcd.print(winsO);
  lcd.print(" Draw ");
  lcd.print(draws);
  lcd.setCursor(0, 1);
  lcd.print("Games: ");
  lcd.print(games + 1);
  // Alternate player
  gameRunning = true;  // Allow game to continue after valid move
}

// Function to reconnect to MQTT broker
void reconnect() {
  // Loop until we're connected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Try to connect with a unique client ID
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      
      // Once connected, subscribe to the topic
      client.subscribe("esp32/led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait for 5 seconds before trying again
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();    // Initialize LCD
  lcd.backlight();  // Turn on backlight

  // Initialize custom I2C pins (optional: change to whatever you need)
  Wire.begin(18, 19);  // Use GPIO18 for SDA and GPIO19 for SCL

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");

  // Connect to MQTT broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize board and display initial message
  clearBoard();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for moves...");

  // Randomly select the starting player
  currentPlayer = (random(0, 2) == 0) ? 'X' : 'O';
}

void loop() {
  // Reconnect if disconnected from MQTT
  if (!client.connected()) {
    reconnect();
  }

  client.loop();  // Keep processing incoming MQTT messages

  // If the game is running, check for a winner and handle game completion
  if (gameRunning) {
    char winner = checkWin();
    if (winner != ' ') {
      if (winner == 'X') {
        client.publish("esp32/gameinfo", "Player X is the Winner");
        winsX++;
      } else {
        client.publish("esp32/gameinfo", "Player O is the Winner");
        winsO++;
      }
      sendBoardState();

      games++;

      // Display score and number of games
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("X: ");
      lcd.print(winsX);
      lcd.print(" O: ");
      lcd.print(winsO);
      lcd.print(" Draw ");
      lcd.print(draws);
      lcd.setCursor(0, 1);
      lcd.print("Games: ");
      lcd.print(games + 1);
      
      // Show the scores for 2 seconds

      // Reset game status
      if (games >= 100){
        esp_deep_sleep_start();
      }
      currentPlayer = 'X';
    }
    else if (checkDraw()) {
      draws++;
      games++;  // Increment game counter
      sendBoardState();

      client.publish("esp32/gameinfo", "Its a draw, no Winner");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("X: ");
      lcd.print(winsX);
      lcd.print(" O: ");
      lcd.print(winsO);
      lcd.print(" Draw ");
      lcd.print(draws);
      lcd.setCursor(0, 1);
      lcd.print("Games: ");
      lcd.print(games + 1);
      if (games >= 100){
        esp_deep_sleep_start();
      }
      currentPlayer = 'X';
    }
    else {
      sendBoardState();
      String message = " Player " + String(currentPlayer) + " placed a " + String(currentPlayer) + " on row " + String(row + 1) + " and column " + String(col + 1) + ".";
      currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
      client.publish("esp32/gameinfo", message.c_str());
    }
  }

  delay(1000);  // Delay to prevent excessive CPU usage
}
