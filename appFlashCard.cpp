#include "raylib.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

using namespace std;

enum AppState {
    MENU,
    ADD_WORD,
    ADD_MEANING,
    ADD_IMAGE,
    ADD_AUDIO,
    DONE,
    START_LEARNING
};

struct FlashCard {
    string word;
    string meaning;
    string imagePath;
    string audioPath;
    Texture2D image{};
    Sound sound{};
    bool hasImage = false;
    bool hasSound = false;
    RenderTexture2D frontTexture; // Corrected from Texture to RenderTexture2D
    RenderTexture2D backTexture;  // Corrected from Texture to RenderTexture2D
};

struct NewCardInput {
    string word;
    string meaning;
    string imagePath;
    string audioPath;
    char buffer[256] = "";
};

void SaveCardToCSV(const string& file, const NewCardInput& card) {
    ofstream f(file, ios::app);
    f << card.word << "," << card.meaning << "," << card.imagePath << "," << card.audioPath << "\n";
}

vector<FlashCard> LoadFlashcards(const string& filename) {
    vector<FlashCard> cards;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cout << "Không thể mở file!" << endl;
        return cards;
    }

    getline(file, line); // Bỏ qua dòng tiêu đề

    if (file.peek() == ifstream::traits_type::eof()) {
        return cards;
    }

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string word, meaning, imagePath, audioPath;

        if (!getline(ss, word, ',') || !getline(ss, meaning, ',') || !getline(ss, imagePath, ',') || !getline(ss, audioPath, ',')) {
            continue;
        }

        FlashCard fc{ word, meaning, imagePath, audioPath };

        if (FileExists(imagePath.c_str())) {
            Image img = LoadImage(imagePath.c_str());
            ImageResize(&img, 300, 200);
            fc.image = LoadTextureFromImage(img);
            UnloadImage(img);
            fc.hasImage = true;
        }

        if (FileExists(audioPath.c_str())) {
            fc.sound = LoadSound(audioPath.c_str());
            fc.hasSound = true;
        }

        fc.frontTexture = LoadRenderTexture(400, 400);
        BeginTextureMode(fc.frontTexture);
        ClearBackground(WHITE);
        if (fc.hasImage) {
            int imgX = (400 - fc.image.width) / 2;
            int imgY = (400 - fc.image.height) / 2;
            DrawTexture(fc.image, imgX, imgY, WHITE);
        }
        int textWidth = MeasureText(fc.word.c_str(), 30);
        DrawText(fc.word.c_str(), (400 - textWidth) / 2, 350, 30, DARKBLUE);
        EndTextureMode();

        fc.backTexture = LoadRenderTexture(400, 400);
        BeginTextureMode(fc.backTexture);
        ClearBackground(WHITE);
        textWidth = MeasureText(fc.meaning.c_str(), 28);
        DrawText(fc.meaning.c_str(), (400 - textWidth) / 2, 200, 28, MAROON);
        EndTextureMode();

        cards.push_back(fc);
    }

    return cards;
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Flashcard App");
    InitAudioDevice();
    SetTargetFPS(60);

    AppState state = MENU;
    Rectangle btnStart = { 250, 200, 300, 60 };
    Rectangle btnAdd = { 250, 300, 300, 60 };
    NewCardInput input;

    vector<FlashCard> cards = LoadFlashcards("assets/flashcards.csv");
    int currentCard = 0;
    bool showFront = true;
    bool isFlipping = false;
    float flipProgress = 0.0f;
    const float flipSpeed = 2.0f;

    while (!WindowShouldClose()) {
        if (isFlipping) {
            flipProgress += flipSpeed * GetFrameTime();
            if (flipProgress >= 1.0f) {
                isFlipping = false;
                showFront = false;
            }
        }

        if (state == START_LEARNING && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (showFront && !isFlipping) {
                isFlipping = true;
                flipProgress = 0.0f;
            } else if (!showFront) {
                currentCard++;
                if (currentCard >= cards.size()) {
                    state = MENU;
                } else {
                    showFront = true;
                }
            }
        }

        BeginDrawing();

        // Fix for C4244: Cast double to float
        float time = (float)GetTime();
        float speed = 1.0f;
        float r = (sin(time * speed) + 1) * 127.5f;
        float g = (sin(time * speed + 2 * PI / 3) + 1) * 127.5f;
        float b = (sin(time * speed + 4 * PI / 3) + 1) * 127.5f;
        Color bgColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
        ClearBackground(bgColor);

        if (state == MENU) {
            DrawText("Flashcard App", 280, 100, 30, DARKGRAY);
            DrawRectangleRec(btnStart, BLUE);
            DrawText("Start Learning", 310, 220, 20, WHITE);
            DrawRectangleRec(btnAdd, DARKGREEN);
            DrawText("Add New Flashcard", 280, 320, 20, WHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, btnStart)) {
                    cards = LoadFlashcards("assets/flashcards.csv");
                    if (!cards.empty()) {
                        currentCard = 0;
                        showFront = true;
                        state = START_LEARNING;
                    }
                }
                if (CheckCollisionPointRec(mouse, btnAdd)) {
                    input = NewCardInput();
                    state = ADD_WORD;
                }
            }
        }

        else if (state == ADD_WORD || state == ADD_MEANING || state == ADD_IMAGE || state == ADD_AUDIO) {
            const char* prompt =
                (state == ADD_WORD) ? "Word " :
                (state == ADD_MEANING) ? "Meaning " :
                (state == ADD_IMAGE) ? "Image " :
                "Audio ";

            DrawText(prompt, 100, 200, 24, DARKGRAY);
            DrawRectangle(250, 190, 400, 40, LIGHTGRAY);
            DrawText(input.buffer, 260, 200, 20, BLACK);

            if (IsKeyPressed(KEY_ENTER)) {
                if (state == ADD_WORD && strlen(input.buffer) > 0) {
                    input.word = input.buffer;
                    state = ADD_MEANING;
                }
                else if (state == ADD_MEANING && strlen(input.buffer) > 0) {
                    input.meaning = input.buffer;
                    state = ADD_IMAGE;
                }
                else if (state == ADD_IMAGE) {
                    state = ADD_AUDIO;
                }
                else if (state == ADD_AUDIO) {
                    SaveCardToCSV("assets/flashcards.csv", input);
                    state = DONE;
                }
                input.buffer[0] = '\0';
            }

            int key = GetCharPressed();
            while (key > 0 && strlen(input.buffer) < 255) {
                input.buffer[strlen(input.buffer)] = (char)key;
                input.buffer[strlen(input.buffer) + 1] = '\0';
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(input.buffer) > 0) {
                input.buffer[strlen(input.buffer) - 1] = '\0';
            }

            if (IsFileDropped()) {
                FilePathList dropped = LoadDroppedFiles();
                if (dropped.count > 0) {
                    std::string src = dropped.paths[0];
                    std::string name = GetFileName(src.c_str());
                    std::string dst = (state == ADD_IMAGE) ? "assets/images/" + name : "assets/sounds/" + name;
                    std::ifstream s(src, ios::binary); std::ofstream d(dst, ios::binary); d << s.rdbuf();
                    if (state == ADD_IMAGE) { input.imagePath = dst; state = ADD_AUDIO; }
                    else { input.audioPath = dst; SaveCardToCSV("assets/flashcards.csv", input); state = DONE; }
                }
                UnloadDroppedFiles(dropped);
            }
        }

        else if (state == DONE) {
            DrawText("Flashcard saved!", 280, 250, 28, DARKGREEN);
            DrawText("Press ENTER to return", 260, 300, 20, GRAY);
            if (IsKeyPressed(KEY_ENTER)) {
                cards = LoadFlashcards("assets/flashcards.csv");
                state = MENU;
            }
        }

        else if (state == START_LEARNING) {
            FlashCard& card = cards[currentCard];

            Rectangle sourceRec = { 0, 0, (float)card.frontTexture.texture.width, -(float)card.frontTexture.texture.height };
            Rectangle destRec = { 200, 100, 400, 400 };
            Vector2 origin = { 0, 0 };

            if (isFlipping) {
                float theta = flipProgress * PI;
                float scale = cos(theta);
                destRec.width = 400 * scale;
                if (scale > 0) {
                    DrawTexturePro(card.frontTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
                } else {
                    DrawTexturePro(card.backTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
                }
            } else {
                if (showFront) {
                    destRec.width = 400;
                    DrawTexturePro(card.frontTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
                } else {
                    destRec.width = 400;
                    DrawTexturePro(card.backTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
                }
            }

            if (showFront) {
                DrawText("Click to flip", 320, 500, 20, GRAY);
            } else {
                DrawText("Click to continue", 300, 500, 20, GRAY);
            }
        }

        EndDrawing();
    }

    for (auto& c : cards) {
        if (c.hasImage) UnloadTexture(c.image);
        if (c.hasSound) UnloadSound(c.sound);
        UnloadRenderTexture(c.frontTexture);
        UnloadRenderTexture(c.backTexture);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}