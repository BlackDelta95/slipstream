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

#define BOX_TOP_MARGIN 20  // Adjust this value to control the vertical spacing from the top of the screen

#define SCROLL_SPEED 4.0f  // Adjust this value to control the speed of the animation
float target = -1;  // Global variable to store the target position

#define SELECTED_BOX_COLOR C2D_Color32(0x00, 0x00, 0x00, 0xFF)  // Black
#define SELECTION_THRESHOLD 10.0f  // Adjust this value to control how close to the center a box needs to be to be selected
#define OUTLINE_THICKNESS 3.0f  // Adjust this value to control the thickness of the outline

// Class for the boxes displayed on the screen
typedef struct {
    float x, y;
    float width, height;
    int UID;  // Add this line
} Box;

typedef struct {
    int UID;
    char* GameName;
    char* GameDescription;  // Add this line
} Record;

Record database[] = {
    {0, "Galaga", "A popular space shooter game from the 80s"},
    {1, "Super Mario Bros", "A classic platformer featuring the famous plumber, Mario"},
    {2, "Donkey Kong", "An arcade game where Mario must rescue Pauline from Donkey Kong"},
    {3, "Contra", "A run and gun action game featuring soldiers Bill and Lance"},
    {4, "Legend of Zelda", "An action-adventure game set in the fantasy land of Hyrule"}
    // Add more records here...
};

void initializeBoxes(Box* boxes) {
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x = i * (BOX_WIDTH + BOX_SPACING);
        boxes[i].y = BOX_TOP_MARGIN;
        boxes[i].width = 128;
        boxes[i].height = 113;
        boxes[i].UID = i;  // Assign a unique UID to each box
    }
}

void printDescription(int UID) {
    // Look up the box's UID in the database
    char* game_description = NULL;
    for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
        if (database[j].UID == UID) {
            game_description = database[j].GameDescription;
            break;
        }
    }

    // Print the game description on the bottom screen
    if (game_description != NULL) {
        C2D_TextBuf textBuf = C2D_TextBufNew(4096); // Create a text buffer
        C2D_Text text;
        float textScale = 0.5f; // Adjust this value to change the size of the text

        C2D_TextParse(&text, textBuf, game_description);
        C2D_TextOptimize(&text);
        float textWidth = text.width * textScale;

        // Adjust these values to change the position of the text
        float textX = SCREEN_WIDTH / 2 - textWidth / 2;
        float textY = SCREEN_HEIGHT - 20;

        C2D_DrawText(&text, C2D_WithColor, textX, textY, 0.5f, textScale, textScale, SELECTED_BOX_COLOR);

        C2D_TextBufDelete(textBuf); // Delete the text buffer
    }
}

void drawCarousel(Box* boxes) {
    u32 colors[NUM_BOXES] = {C2D_Color32(0xFF, 0x00, 0x00, 0xFF),  // Red
                             C2D_Color32(0x00, 0xFF, 0x00, 0xFF),  // Green
                             C2D_Color32(0x00, 0x00, 0xFF, 0xFF),  // Blue
                             C2D_Color32(0xFF, 0xFF, 0x00, 0xFF),  // Yellow
                             C2D_Color32(0xFF, 0x00, 0xFF, 0xFF)}; // Magenta

    C2D_TextBuf textBuf = C2D_TextBufNew(4096); // Create a text buffer
    C2D_Text text;
    float textScale = 0.5f; // Adjust this value to change the size of the text
    float textHeight = 10.0f; // Adjust this value to change the spacing below the box
    float textWidth = text.width * textScale;

    for (int i = 0; i < NUM_BOXES; i++) {
        // Check if the box is near the center of the screen
        if (abs(boxes[i].x + boxes[i].width / 2 - SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            // Draw an outline around the box
            C2D_DrawRectangle(boxes[i].x - OUTLINE_THICKNESS, boxes[i].y - OUTLINE_THICKNESS, 0.5f, boxes[i].width + 2 * OUTLINE_THICKNESS, boxes[i].height + 2 * OUTLINE_THICKNESS, SELECTED_BOX_COLOR, SELECTED_BOX_COLOR, SELECTED_BOX_COLOR, SELECTED_BOX_COLOR);

            // Look up the box's UID in the database
            char* game_name = NULL;
            for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
                if (database[j].UID == boxes[i].UID) {
                    game_name = database[j].GameName;
                    printDescription(boxes[i].UID)
                    break;
                }
            }

            // Set the text for the selected box
            if (game_name != NULL) {
                C2D_TextParse(&text, textBuf, game_name);
                C2D_TextOptimize(&text);
                float textWidth = text.width * textScale;
                C2D_DrawText(&text, C2D_WithColor, boxes[i].x + boxes[i].width / 2 - textWidth / 2, boxes[i].y + boxes[i].height + textHeight, 0.5f, textScale, textScale, SELECTED_BOX_COLOR);
            }

        }
        C2D_DrawRectSolid(boxes[i].x, boxes[i].y, 0.5f, boxes[i].width, boxes[i].height, colors[i]);
    }

    C2D_TextBufDelete(textBuf); // Delete the text buffer
}

void scrollCarouselLeft(Box* boxes) {
    // Find the selected box
    int selectedIndex = -1;
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedIndex = i;
            break;
        }
    }

    // Calculate the target position for the next box
    target = boxes[(selectedIndex - 1 + NUM_BOXES) % NUM_BOXES].x;

    // Scroll the carousel
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x -= SCROLL_SPEED;
        if (boxes[i].x + boxes[i].width < 0) {
            boxes[i].x += totalWidth;
        }
    }
}

void scrollCarouselRight(Box* boxes) {
    // Find the selected box
    int selectedIndex = -1;
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedIndex = i;
            break;
        }
    }

    // Calculate the target position for the next box
    target = boxes[(selectedIndex + 1) % NUM_BOXES].x;

    // Scroll the carousel
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

    // Create screens
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);


    Box boxes[NUM_BOXES];  // Create an array of NUM_BOXES boxes
    initializeBoxes(boxes);

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

        if (kHeld & KEY_DRIGHT) {
            scrollCarouselLeft(boxes);
        }
        else if (kHeld & KEY_DLEFT) {
            scrollCarouselRight(boxes);
        }

        // Render the top scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
        C2D_SceneBegin(top);

        drawCarousel(boxes);

        // Render the bottom scene
        C2D_SceneBegin(bot);
        C2D_TargetClear(bot, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

        // Check if the selected box has reached the target position
        int selectedIndex = -1;
        for (int i = 0; i < NUM_BOXES; i++) {
            if (abs(boxes[i].x + boxes[i].width / 2 - SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
                selectedIndex = i;
                break;
            }
        }
        if (selectedIndex != -1 && abs(boxes[selectedIndex].x - target) < SCROLL_SPEED) {
            target = -1;  // Reset the target position
        }

        C3D_FrameEnd(0);
        

    }

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}