#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "lodepng.h"

#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

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

#define GLOBAL_BACKGROUND_COLOR C2D_Color32(0x1A, 0x1A, 0x1A, 0xFF)
#define GLOBAL_MAIN_TEXT_COLOR C2D_Color32(0x4C, 0xE4, 0x9D, 0xFF)
#define GLOBAL_SECONDARY_TEXT_COLOR C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)

// Class for the boxes displayed on the screen
typedef struct {
    float x, y;
    float width, height;
    int UID;
    C2D_Image image;
} Box;

typedef struct {
    int UID;
    char* GameName;
    char* GameDescription;
} Record;

Record database[] = {
    {0, "Pokémon Alpha Sapphire", "An epic adventure in the Hoenn region with your Pokémon. Hello how are you"},
    {1, "Super Mario 3D Land", "Join Mario in a 3D platforming adventure full of fun."},
    {2, "Super Smash Bros. for Nintendo 3DS", "Battle with famous characters in ultimate brawling."},
    {3, "The Legend of Zelda: Ocarina of Time 3D", "Embark on a quest to save Hyrule with Link."},
    {4, "Metroid: Samus Returns", "Samus Aran returns for a Metroid adventure on 3DS."}
    // Add more records here...
};

C2D_TextBuf gTextBuf;

void initTextBuffers() {
    gTextBuf = C2D_TextBufNew(4096); // Adjust size as needed
}

void cleanupTextBuffers() {
    C2D_TextBufDelete(gTextBuf);
}


C2D_Image convertPNGToC2DImage(const char* filename) {
    unsigned error;
    unsigned char* image;
    unsigned width, height;
    size_t pngsize;
    LodePNGState state;

    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_RGBA;

    error = lodepng_load_file(&image, &pngsize, filename);
    if (error) printf("error %u: %s\n", error, lodepng_error_text(error));

    error = lodepng_decode(&image, &width, &height, &state, image, pngsize);
    if (error) printf("error %u: %s\n", error, lodepng_error_text(error));

    C2D_Image img;
    img.tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    Tex3DS_SubTexture subtex = {(u16)width, (u16)height, 0.0f, 1.0f, width / 512.0f, 1.0f - (height / 512.0f)};
    img.subtex = &subtex;

    C3D_TexInit(img.tex, 512, 512, GPU_RGBA8);
    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    img.tex->border = 0xFFFFFFFF;
    C3D_TexSetWrap(img.tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

    for (u32 x = 0; x < width && x < 512; x++) {
        for (u32 y = 0; y < height && y < 512; y++) {
            const u32 dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) +
                                ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
                                ((x & 4) << 2) | ((y & 4) << 3))) * 4;

            const u32 srcPos = (y * width + x) * 4;
            ((uint8_t *)img.tex->data)[dstPos + 0] = image[srcPos + 3];
            ((uint8_t *)img.tex->data)[dstPos + 1] = image[srcPos + 2];
            ((uint8_t *)img.tex->data)[dstPos + 2] = image[srcPos + 1];
            ((uint8_t *)img.tex->data)[dstPos + 3] = image[srcPos + 0];
        }
    }

    free(image);
    lodepng_state_cleanup(&state);
    
    printf("Loading image");
    return img;
}

// Load in the games
void initializeBoxes(Box* boxes) {
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x = i * (BOX_WIDTH + BOX_SPACING);
        boxes[i].y = BOX_TOP_MARGIN;
        boxes[i].width = 128;
        boxes[i].height = 130;
        boxes[i].UID = i;  // Assign a unique UID to each box

        // Load the PNG image for the game
        char filename[256];
        sprintf(filename, "game%d.png", i);  // Assuming the images are named game0.png, game1.png, etc.
        boxes[i].image = convertPNGToC2DImage(filename);
    }
}

// Launch the selected title
void launchTitle(int UID) {
    // Look up the box's UID in the database
    char* game_name = NULL;
    for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
        if (database[j].UID == UID) {
            game_name = database[j].GameName;
            break;
        }
    }

    C2D_TextBufClear(gTextBuf);  // Clear the buffer for reuse
    C2D_Text text;

    float textScale = 0.5f; // Adjust this value to change the size of the text

    C2D_TextParse(&text, gTextBuf, "Launching Game");
    C2D_TextOptimize(&text);

    // Adjust these values to change the position of the text
    float textX = 10.0f; // Adjust this value to change the horizontal position of the text
    float textY = 10.0f; // Adjust this value to change the vertical position of the text

    // Adjust this value to change the size of the margin
    float margin = 60.0f;

    C2D_DrawText(&text, C2D_WithColor | C2D_WordWrap, textX, textY, 0.5f, textScale, textScale, SELECTED_BOX_COLOR, BOTTOM_SCREEN_WIDTH - 2 * margin);
}

// Print the description of the currently selected game
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
        float textScale = 0.5f; // Adjust this value to change the size of the text

        C2D_TextBufClear(gTextBuf);  // Clear the buffer for reuse
        C2D_Text text;

        // Reuse the existing text buffer
        C2D_TextParse(&text, gTextBuf, game_description);
        C2D_TextOptimize(&text);

        // Adjust these values to change the position of the text
        float textX = 10.0f; // Adjust this value to change the horizontal position of the text
        float textY = 10.0f; // Adjust this value to change the vertical position of the text

        // Adjust this value to change the size of the margin
        float margin = 100.0f;

        C2D_DrawText(&text, C2D_WithColor | C2D_WordWrap, textX, textY, 0.5f, textScale, textScale, GLOBAL_SECONDARY_TEXT_COLOR, BOTTOM_SCREEN_WIDTH - textX * 2);
    }
}

// Render the carousel
int drawCarousel(Box* boxes) {
    u32 colors[NUM_BOXES] = {C2D_Color32(0xFF, 0x00, 0x00, 0xFF),  // Red
                             C2D_Color32(0x00, 0xFF, 0x00, 0xFF),  // Green
                             C2D_Color32(0x00, 0x00, 0xFF, 0xFF),  // Blue
                             C2D_Color32(0xFF, 0xFF, 0x00, 0xFF),  // Yellow
                             C2D_Color32(0xFF, 0x00, 0xFF, 0xFF)}; // Magenta



    int selectedUID = -1;

    for (int i = 0; i < NUM_BOXES; i++) {
        // Check if the box is near the center of the screen
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedUID = boxes[i].UID;

            // Look up the box's UID in the database
            char* game_name = NULL;
            for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
                if (database[j].UID == boxes[i].UID) {
                    game_name = database[j].GameName;
                    break;
                }
            }

            // Set the text for the selected box
            if (game_name != NULL) {
                C2D_Text text;
                float textScale = 0.5f; // Adjust this value to change the size of the text
                float textHeight = 10.0f; // Adjust this value to change the spacing below the box

                C2D_TextParse(&text, gTextBuf, game_name);
                C2D_TextOptimize(&text);
                float textWidth = text.width * textScale;
                C2D_DrawText(&text, C2D_WithColor, boxes[i].x + boxes[i].width / 2 - textWidth / 2, boxes[i].y + boxes[i].height + textHeight, 0.5f, textScale, textScale, GLOBAL_MAIN_TEXT_COLOR);
            }

        }
        C2D_DrawImageAt(boxes[i].image, boxes[i].x, boxes[i].y, 0.5f, NULL, 1.0f, 1.0f);
    }

    return selectedUID;
}

// Scroll the carousel left
void scrollCarouselLeft(Box* boxes) {
    // Find the selected box
    int selectedIndex = -1;
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
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

// Scroll the carousel right
void scrollCarouselRight(Box* boxes) {
    // Find the selected box
    int selectedIndex = -1;
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
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

    // Remove for debugging
	//consoleInit(GFX_BOTTOM, NULL);

    // Create screens
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    Box boxes[NUM_BOXES];  // Create an array of NUM_BOXES boxes
    initializeBoxes(boxes);
    initTextBuffers();

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
            scrollCarouselRight(boxes);;
        }

        // Render the top scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, GLOBAL_BACKGROUND_COLOR);
        C2D_SceneBegin(top);

        /* Draw the image at position (x, y) */
        float x = 0.0f; /* Replace with your x-coordinate */
        float y = 0.0f; /* Replace with your y-coordinate */

        // Grab the UID of the currently selected box
        int selectedUID = drawCarousel(boxes);

        // Launch the selected game
        if (kHeld & KEY_A) {
            launchTitle(selectedUID);
        }

        //  Exit the application
        if (kDown & KEY_START) {
            break;
        } 

        // Render the bottom scene
        C2D_SceneBegin(bot);
        C2D_TargetClear(bot, GLOBAL_BACKGROUND_COLOR);

        // Check if the selected box has reached the target position
        int selectedIndex = -1;
        for (int i = 0; i < NUM_BOXES; i++) {
            if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
                selectedIndex = i;
                break;
            }
        }
        if (selectedIndex != -1 && abs(boxes[selectedIndex].x - target) < SCROLL_SPEED) {
            target = -1;  // Reset the target position
        }

        printDescription(selectedUID);

        C3D_FrameEnd(0);
    }

    cleanupTextBuffers();

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}