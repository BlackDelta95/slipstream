#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <c2d/text.h>

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define BOX_WIDTH 128
#define BOX_HEIGHT 114
#define BOX_SPACING 20
#define SCROLL_SPEED  2.0f
#define NUM_BOXES 5  // Number of boxes

typedef struct {
    float x;
    float targetX;
    int y;
    u32 color;
    C2D_Text text;
} Box;

bool animating = false;
int direction = 0;  // 1 for right, -1 for left

void initializeBoxes(Box* boxes) {
    u32 colors[NUM_BOXES] = {
        C2D_Color32(255, 0, 0, 255),   // Red
        C2D_Color32(0, 255, 0, 255),   // Green
        C2D_Color32(0, 0, 255, 255),   // Blue
        C2D_Color32(255, 255, 0, 255), // Yellow
        C2D_Color32(0, 255, 255, 255)  // Cyan
    };
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i] = (Box){i * (BOX_WIDTH + BOX_SPACING), i * (BOX_WIDTH + BOX_SPACING), SCREEN_HEIGHT / 2 - BOX_HEIGHT / 2, colors[i]};
        char colorText[20];
        sprintf(colorText, "Color: %u", boxes[i].color);
        C2D_TextBuf textBuf = C2D_TextBufNew(4096); // Create a text buffer
        C2D_TextParse(&boxes[i].text, textBuf, colorText); // Parse the text to create a C2D_Text object
        C2D_TextOptimize(&boxes[i].text); // Optimize the text for drawing
    }
}

void updateBoxPositions(Box* boxes) {
    int totalWidth = NUM_BOXES * (BOX_WIDTH + BOX_SPACING);
    for (int i = 0; i < NUM_BOXES; i++) {
        // If the box is off the screen, instantly move it to its targetX position
        if (boxes[i].x + BOX_WIDTH < 0 || boxes[i].x - BOX_WIDTH > SCREEN_WIDTH) {
            boxes[i].x = boxes[i].targetX;
        } else if (abs(boxes[i].x - boxes[i].targetX) > SCROLL_SPEED) {
            if (boxes[i].x < boxes[i].targetX) {
                boxes[i].x += SCROLL_SPEED;
            } else if (boxes[i].x > boxes[i].targetX) {
                boxes[i].x -= SCROLL_SPEED;
            }
        } else {
            boxes[i].x = boxes[i].targetX;
            // Wrap the x position around if it goes off the screen
            boxes[i].x = fmod(boxes[i].x, totalWidth);
        }
    }
    // Check if all boxes have reached their target positions
    animating = false;
    for (int i = 0; i < NUM_BOXES; i++) {
        if (boxes[i].x != boxes[i].targetX) {
            animating = true;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    // Init libs
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);

    // Create screens
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    Box boxes[NUM_BOXES];  // Create an array of NUM_BOXES boxes
    initializeBoxes(boxes);

    int totalWidth = NUM_BOXES * (BOX_WIDTH + BOX_SPACING);

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input
        u32 hatDown = hidKeysHeld();
        
        if (!animating && (hatDown & KEY_DRIGHT)) {
            direction = 1;
            // Update target positions for scrolling to the right
            for (int i = 0; i < NUM_BOXES; i++) {
                float newTargetX = fmod(boxes[i].targetX + (BOX_WIDTH + BOX_SPACING), totalWidth);
                // If the box is moving off the right end of the carousel, update its x position to be off the screen
                if (newTargetX < boxes[i].targetX) {
                    boxes[i].x = newTargetX - (BOX_WIDTH + BOX_SPACING);
                }
                boxes[i].targetX = newTargetX;
            }
            animating = true;
        } else if (!animating && (hatDown & KEY_DLEFT)) {
            direction = -1;
            // Update target positions for scrolling to the left
            for (int i = 0; i < NUM_BOXES; i++) {
                float newTargetX = fmod(boxes[i].targetX - (BOX_WIDTH + BOX_SPACING) + totalWidth, totalWidth);
                boxes[i].targetX = newTargetX;
            }
            animating = true;
        }

        if (animating) {
            updateBoxPositions(boxes);
            // After updating box positions, check if any box has moved off the screen
            for (int i = 0; i < NUM_BOXES; i++) {
                if (boxes[i].x + BOX_WIDTH < 0) {
                    // If the box has moved off the screen, update its x position to be at the right end of the carousel
                    boxes[i].x += totalWidth;
                    // Shift the box to the end of the array
                    Box temp = boxes[i];
                    for (int j = i; j < NUM_BOXES - 1; j++) {
                        boxes[j] = boxes[j + 1];
                    }
                    boxes[NUM_BOXES - 1] = temp;
                }
            }
        }

        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        C2D_SceneBegin(top);

        if (direction == 1) {  // Scrolling to the right
            for (int i = 0; i < NUM_BOXES; i++) {  // Iterate over NUM_BOXES boxes
                C2D_DrawRectangle(boxes[i].x - BOX_WIDTH / 2, boxes[i].y, 0, BOX_WIDTH, BOX_HEIGHT, boxes[i].color, boxes[i].color, boxes[i].color, boxes[i].color);
                C2D_DrawText(&boxes[i].text, C2D_WithColor, boxes[i].x, boxes[i].y + BOX_HEIGHT, 0.5f, 0.5f, 0.5f, boxes[i].color);
            }
        } else if (direction == -1) {  // Scrolling to the left
            for (int i = NUM_BOXES - 1; i >= 0; i--) {  // Iterate over NUM_BOXES boxes in reverse order
                C2D_DrawRectangle(boxes[i].x - BOX_WIDTH / 2, boxes[i].y, 0, BOX_WIDTH, BOX_HEIGHT, boxes[i].color, boxes[i].color, boxes[i].color, boxes[i].color);
                C2D_DrawText(&boxes[i].text, C2D_WithColor, boxes[i].x, boxes[i].y + BOX_HEIGHT, 0.5f, 0.5f, 0.5f, boxes[i].color);
            }
        }dfdf

        C3D_FrameEnd(0);
    }

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
