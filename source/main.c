#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define NUM_BOXES 5
#define BOX_WIDTH 128
#define BOX_SPACING 10
#define totalWidth NUM_BOXES * (BOX_WIDTH + BOX_SPACING)

#define SCROLL_SPEED 2.0f  // Adjust this value to control the speed of the animation


typedef struct {
    float x, y;
    float width, height;
} Box;

void initializeBoxes(Box* boxes) {
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x = i * (BOX_WIDTH + BOX_SPACING);
        boxes[i].y = 0;
        boxes[i].width = 128;
        boxes[i].height = 113;
    }
}

void drawCarousel(Box* boxes) {
    u32 colors[NUM_BOXES] = {C2D_Color32(0xFF, 0x00, 0x00, 0xFF),  // Red
                             C2D_Color32(0x00, 0xFF, 0x00, 0xFF),  // Green
                             C2D_Color32(0x00, 0x00, 0xFF, 0xFF),  // Blue
                             C2D_Color32(0xFF, 0xFF, 0x00, 0xFF),  // Yellow
                             C2D_Color32(0xFF, 0x00, 0xFF, 0xFF)}; // Magenta

    for (int i = 0; i < NUM_BOXES; i++) {
        C2D_DrawRectSolid(boxes[i].x, boxes[i].y, 0.5f, boxes[i].width, boxes[i].height, colors[i]);
    }
}

void scrollCarouselLeft(Box* boxes) {
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x -= SCROLL_SPEED;
        if (boxes[i].x > totalWidth - BOX_WIDTH) {
            boxes[i].x -= totalWidth;
        }
    }
}

void scrollCarouselRight(Box* boxes) {
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x += SCROLL_SPEED;
        if (boxes[i].x > totalWidth - BOX_WIDTH) {
            boxes[i].x -= totalWidth;
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

    int frameCounter = 0;
    int scrollEveryNFrames = 60;  // Adjust this value to control the speed

    // Main loop
    while (aptMainLoop()) {
	    //Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		//hidKeysHeld returns information about which buttons have are held down in this frame
		u32 kHeld = hidKeysHeld();

		//hidKeysUp returns information about which buttons have been just released
		u32 kUp = hidKeysUp();

        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        C2D_SceneBegin(top);

        drawCarousel(boxes);

        if (kDown & KEY_DRIGHT) {
            scrollCarouselLeft(boxes);
        }
        if (kDown & KEY_DLEFT) {
            scrollCarouselRight(boxes);
        }

        C3D_FrameEnd(0);
    }

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}