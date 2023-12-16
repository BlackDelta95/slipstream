#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "lodepng.h"

// Screen dimensions
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

// Box dimensions and carousel settings
#define NUM_BOXES 5
#define BOX_WIDTH 128
#define BOX_SPACING 10
#define totalWidth (NUM_BOXES * (BOX_WIDTH + BOX_SPACING))
#define BOX_TOP_MARGIN 20 // Vertical spacing from top of the screen

// Animation and interaction settings
#define SCROLL_SPEED 4.0f // Speed of carousel animation
#define SELECTION_THRESHOLD 10.0f // Proximity to center for selection
#define OUTLINE_THICKNESS 3.0f // Thickness of the box outline

// Color definitions
#define SELECTED_BOX_COLOR C2D_Color32(0x00, 0x00, 0x00, 0xFF) // Black
#define GLOBAL_BACKGROUND_COLOR C2D_Color32(0x1A, 0x1A, 0x1A, 0xFF)
#define GLOBAL_MAIN_TEXT_COLOR C2D_Color32(0x4C, 0xE4, 0x9D, 0xFF)
#define GLOBAL_SECONDARY_TEXT_COLOR C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)

// Global variable for target position in carousel
float target = -1;

// Struct definition for Box
typedef struct {
    float x, y;
    float width, height;
    int UID;
    C2D_Image image;
    C2D_Text description;
} Box;

// Struct definition for game database records
typedef struct {
    int UID;
    char* GameName;
    char* GameDescription;
} Record;


Record database[] = {
    {0, "Pokémon Alpha Sapphire", "An epic adventure in the Hoenn region with your Pokemon. HELLO THIS IS A TEST. HELLO THIS IS A TEST."},
    {1, "Super Mario 3D Land", "Join Mario in a 3D platforming adventure full of fun."},
    {2, "Super Smash Bros. for Nintendo 3DS", "Battle with famous characters in ultimate brawling."},
    {3, "The Legend of Zelda: Ocarina of Time 3D", "Embark on a quest to save Hyrule with Link."},
    {4, "Metroid: Samus Returns", "Samus Aran returns for a Metroid adventure on 3DS."}
    // Add more records here...
};

u32 calculateTexturePosition (
/*
    SYNOPSIS
        Calculates the position in the texture buffer for a given pixel coordinate.

    DESCRIPTION
        This function takes the x and y pixel coordinates and calculates the corresponding
        position in the texture buffer. It is used for converting image data into a format
        suitable for graphics rendering. The calculation involves bit manipulation to
        efficiently determine the position in a linear texture buffer.

    EXAMPLE
        u32 position = calculateTexturePosition(10, 20);
        // This will calculate the texture buffer position for the pixel at coordinates (10, 20).
*/
    // X coordinate of the pixel
    u32 x, 
    // Y coordinate of the pixel
    u32 y
) {
    return ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) +
            ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
            ((x & 4) << 2) | ((y & 4) << 3))) * 4;
}

// Converts a PNG image file to a C2D_Image format
C2D_Image convertPNGToC2DImage (
/*
    SYNOPSIS
        Converts a PNG image file to a C2D_Image format for rendering in a graphics application.

    DESCRIPTION
        Loads a PNG file specified by the filename, decodes it, and then converts the image
        data into a C2D_Image format suitable for rendering. The function handles loading errors
        and allocates necessary resources for the image conversion.

    EXAMPLE
        C2D_Image image = convertPNGToC2DImage("path/to/image.png");

        Converts the PNG image at the specified path to a C2D_Image.
*/
    // Filename of the PNG image to be converted
    const char* filename
) {
    unsigned error;
    unsigned char* image;
    unsigned width, height;
    size_t pngsize;
    LodePNGState state;

    // Initialize the PNG state with RGBA color type
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_RGBA;

    // Load the PNG file and handle any errors
    error = lodepng_load_file(&image, &pngsize, filename);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return (C2D_Image){0}; // Return an empty image in case of error
    }

    // Decode the PNG file into raw image data
    error = lodepng_decode(&image, &width, &height, &state, image, pngsize);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        free(image); // Ensure to free the loaded image in case of error
        return (C2D_Image){0}; // Return an empty image in case of error
    }

    // Create a C2D_Image and allocate memory for its texture
    C2D_Image img;
    img.tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));

    // Set up the sub-texture parameters based on image dimensions
    Tex3DS_SubTexture subtex = {
        (u16)width, (u16)height, 0.0f, 1.0f, 
        width / 512.0f, 1.0f - (height / 512.0f)
    };
    img.subtex = &subtex;

    // Initialize the texture with RGBA format and set filters
    C3D_TexInit(img.tex, 512, 512, GPU_RGBA8);
    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    img.tex->border = 0xFFFFFFFF;
    C3D_TexSetWrap(img.tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

    // Convert the PNG image data to texture format
    for (u32 x = 0; x < width && x < 512; x++) {
        for (u32 y = 0; y < height && y < 512; y++) {
            // Calculate destination and source positions for texture data
            const u32 dstPos = calculateTexturePosition(x, y);
            const u32 srcPos = (y * width + x) * 4;

            // Assign the RGBA values to the texture
            ((uint8_t *)img.tex->data)[dstPos + 0] = image[srcPos + 3];
            ((uint8_t *)img.tex->data)[dstPos + 1] = image[srcPos + 2];
            ((uint8_t *)img.tex->data)[dstPos + 2] = image[srcPos + 1];
            ((uint8_t *)img.tex->data)[dstPos + 3] = image[srcPos + 0];
        }
    }

    // Clean up the loaded image and PNG state
    free(image);
    lodepng_state_cleanup(&state);

    printf("Image loaded: %s\n", filename);
    return img; // Return the created C2D_Image
}

C2D_Text getDescription (
/*
    SYNOPSIS
        Retrieves the description of a game from the database and prepares it for rendering.

    DESCRIPTION
        Searches the database for a game record matching the given unique identifier (UID).
        If found, it retrieves the game's description and prepares a C2D_Text object for rendering
        this description on the screen.

    EXAMPLE
        int gameUID = 2; // Example UID
        C2D_Text gameText = getDescription(gameUID);

        Retrieves and prepares the description of the game with UID 2 for rendering.
*/
    // A unique identifier for the game
    int UID
) {
    char* game_description = NULL; // Holds the game description retrieved from the database

    // Loop through the database to find the record matching the given UID
    for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
        if (database[j].UID == UID) {
            game_description = database[j].GameDescription; // Assign the description if found
            break;
        }
    }

    // If the game description is found, prepare it for rendering
    if (game_description != NULL) {
        C2D_TextBuf tempBuf; // Buffer for text rendering
        tempBuf = C2D_TextBufNew(4096); // Allocate a new text buffer
        C2D_Text text; // The text object to be rendered

        // Parse and optimize the game description for rendering
        C2D_TextParse(&text, tempBuf, game_description);
        C2D_TextOptimize(&text);

        // Delete the temporary buffer as it's no longer needed
        C2D_TextBufDelete(tempBuf);

        return text; // Return the prepared text object
    }
}

void initializeBoxes (
/*
    SYNOPSIS
        Initializes the boxes in the carousel.

    DESCRIPTION
        Sets the position, dimensions, and unique identifier (UID) for each box in the carousel.
        Also loads the corresponding image and description for each box. This function is essential
        for setting up the initial state of the carousel with all its boxes.

    PARAMETER boxes
        A pointer to an array of 'Box' structures. This array is filled with the initialized data for
        each box, including position, dimensions, UID, image, and description.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);

        Initializes an array of boxes for the carousel.
*/
    // Pointer to an array of 'Box' structures
    Box* boxes
) {
    for (int i = 0; i < NUM_BOXES; i++) {
        // Set position and dimensions for each box
        boxes[i].x = i * (BOX_WIDTH + BOX_SPACING);
        boxes[i].y = BOX_TOP_MARGIN;
        boxes[i].width = 128;
        boxes[i].height = 130;

        // Assign a unique UID to each box
        boxes[i].UID = i;

        // Load the PNG image for the game
        char filename[256];
        sprintf(filename, "game%d.png", i);  // Assuming the images are named game0.png, game1.png, etc.
        boxes[i].image = convertPNGToC2DImage(filename);

        // Retrieve and store the game description
        boxes[i].description = getDescription(i);
    }
}

void launchTitle (
/*
    SYNOPSIS
        Launches the selected game title.

    DESCRIPTION
        Looks up the selected game in the database by its UID and launches it.
        It also displays a message indicating that the game is launching.

    EXAMPLE
        int selectedUID = 1; // Assume this is the selected UID
        launchTitle(selectedUID);

        Launches the game corresponding to the provided UID.
*/
    int UID
) {
    char* game_name = NULL; // Variable to hold the game name retrieved from the database

    // Loop through the database to find the game with the matching UID
    for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
        if (database[j].UID == UID) {
            game_name = database[j].GameName; // Assign the game name if found
            break;
        }
    }

    // Prepare a text buffer for rendering text on the screen
    C2D_TextBuf tempBuf;
    tempBuf = C2D_TextBufNew(4096);
    C2D_Text text; // The text object to be rendered

    // Parse and optimize the launch message for rendering
    C2D_TextParse(&text, tempBuf, "Launching Game");
    C2D_TextOptimize(&text);

    // Draw the launch message on the screen
    C2D_DrawText(
        &text, 
        C2D_WithColor | C2D_WordWrap, 
        10.0f, // X position
        10.0f, // Y position
        0.5f,  // Z depth
        0.5f,  // Text scale (X)
        0.5f,  // Text scale (Y)
        SELECTED_BOX_COLOR, 
        BOTTOM_SCREEN_WIDTH - 2 * 60.0f // Screen width minus margins
    );

    // Delete the temporary text buffer
    C2D_TextBufDelete(tempBuf);
}

int checkSelectedBoxReachedTarget (
/*
    SYNOPSIS
        Determines which box in the carousel is currently "selected".

    DESCRIPTION
        Iterates through each box to identify the one closest to the center of the top screen,
        considering it as the "selected" box. Additionally, it checks if the selected box has
        reached a specified target position.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);
        float targetPosition = 100.0f; // Example target position
        int selectedIndex = checkSelectedBoxReachedTarget(boxes, NUM_BOXES, &targetPosition);

        Determines the selected box and checks if it has reached the target position.
*/

    // Array of 'Box' structures representing the boxes on screen
    Box* boxes, 

    // Number of boxes in the array
    int numBoxes, 

    // Pointer to the target position variable
    float* target
) {
    int selectedIndex = -1; // Variable to hold the index of the selected box

    // Loop through each box to find the one that is closest to the center of the top screen
    for (int i = 0; i < numBoxes; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedIndex = i; // Assign the index of the selected box
            break;
        }
    }

    // Check if the selected box has reached the target position
    if (selectedIndex != -1 && abs(boxes[selectedIndex].x - *target) < SCROLL_SPEED) {
        *target = -1; // Reset the target position if reached
    }

    return selectedIndex; // Return the index of the selected box
}

int drawCarouselTop (
/*
    SYNOPSIS
        Renders the top half of the carousel.

    DESCRIPTION
        Draws each box in the top half of the carousel and identifies the box that is
        closest to the center of the screen as the selected box.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);
        int selectedUID = drawCarouselTop(boxes);

        Renders the top half of the carousel and returns the UID of the selected box.
*/
    Box* boxes
) {
    int selectedUID = -1; // Variable to hold the UID of the selected box

    // Create a text buffer for rendering text
    C2D_TextBuf tempBuf;
    tempBuf = C2D_TextBufNew(4096);

    // Loop through each box to render them and identify the selected box
    for (int i = 0; i < NUM_BOXES; i++) {
        // Check if the box is near the center of the screen
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedUID = boxes[i].UID; // Assign the UID of the selected box

            // Retrieve the game name for the selected box from the database
            char* game_name = NULL;
            for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
                if (database[j].UID == boxes[i].UID) {
                    game_name = database[j].GameName; // Assign the game name if found
                    break;
                }
            }

            // Render the name of the selected game
            if (game_name != NULL) {
                C2D_Text text; // The text object to be rendered
                float textScale = 0.5f; // Text size scaling factor
                float textHeight = 10.0f; // Vertical position adjustment for the text

                // Parse and optimize the game name for rendering
                C2D_TextParse(&text, tempBuf, game_name);
                C2D_TextOptimize(&text);

                // Calculate the width of the rendered text
                float textWidth = text.width * textScale;

                // Draw the text on the screen
                C2D_DrawText(
                    &text, 
                    C2D_WithColor, 
                    boxes[i].x + boxes[i].width / 2 - textWidth / 2, // X position
                    boxes[i].y + boxes[i].height + textHeight, // Y position
                    0.5f, // Z depth
                    textScale, // Text scale (X)
                    textScale, // Text scale (Y)
                    GLOBAL_MAIN_TEXT_COLOR
                );
            }
        }

        // Draw each box in the carousel
        C2D_DrawImageAt(
            boxes[i].image, 
            boxes[i].x, 
            boxes[i].y, 
            0.5f, // Z depth
            NULL, // Parameters (not used here)
            1.0f, // Scale X
            1.0f  // Scale Y
        );
    }

    // Delete the temporary text buffer
    C2D_TextBufDelete(tempBuf);

    return selectedUID; // Return the UID of the selected box
}

int drawCarouselBottom (
/*
    SYNOPSIS
        Renders the bottom half of the carousel.

    DESCRIPTION
        Draws each box in the bottom half of the carousel. If a box is near the center
        of the screen, it displays additional information like the game description.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);
        C2D_TextBuf bottomScreenTextBuffer = C2D_TextBufNew(4096);
        drawCarouselBottom(boxes, bottomScreenTextBuffer);

        Renders the bottom half of the carousel with additional game information.
*/
    Box* boxes,
    C2D_TextBuf Buffer
) {
    int selectedUID = -1; // Variable to hold the UID of the selected box

    // Loop through each box to render them and identify the selected box
    for (int i = 0; i < NUM_BOXES; i++) {
        // Check if the box is near the center of the screen
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedUID = boxes[i].UID; // Assign the UID of the selected box

            // Retrieve the game description for the selected box from the database
            char* game_description = NULL;
            for (int j = 0; j < sizeof(database) / sizeof(Record); j++) {
                if (database[j].UID == boxes[i].UID) {
                    game_description = database[j].GameDescription; // Assign the game description if found
                    break;
                }
            }

            // Render the description of the selected game
            if (game_description != NULL) {
                C2D_Text text; // The text object to be rendered
                float textScale = 0.5f; // Text size scaling factor
                float textX = 10.0f; // Horizontal position adjustment for the text
                float textY = 10.0f; // Vertical position adjustment for the text

                // Parse and optimize the game description for rendering
                C2D_TextParse(&text, Buffer, game_description);
                C2D_TextOptimize(&text);

                // Draw the text on the screen
                C2D_DrawText(
                    &text, 
                    C2D_WithColor | C2D_WordWrap, 
                    textX, // X position
                    textY, // Y position
                    0.5f, // Z depth
                    textScale, // Text scale (X)
                    textScale, // Text scale (Y)
                    GLOBAL_SECONDARY_TEXT_COLOR, 
                    BOTTOM_SCREEN_WIDTH / 2 - textX // Width constraint for word wrapping
                );            
            }
        }
    }

    return selectedUID; // Return the UID of the selected box
}

void scrollCarouselLeft (
/*
    SYNOPSIS
        Scrolls the carousel to the left.

    DESCRIPTION
        Moves each box in the carousel to the left. It finds the currently selected box,
        then adjusts the positions of all boxes accordingly. If a box moves past the left edge
        of the carousel, it wraps around to the right.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);
        scrollCarouselLeft(boxes);

        Initializes a carousel of boxes and then scrolls it to the left.
*/
    Box* boxes
) {
    int selectedIndex = -1; // Variable to hold the index of the selected box

    // Find the selected box
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedIndex = i; // Assign the index of the selected box
            break;
        }
    }

    // Calculate the target position for the box to the left of the selected one
    float target = boxes[(selectedIndex - 1 + NUM_BOXES) % NUM_BOXES].x;

    // Move each box in the carousel to the left
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x -= SCROLL_SPEED; // Scroll each box to the left
        // Wrap around if a box moves past the left edge
        if (boxes[i].x + boxes[i].width < 0) {
            boxes[i].x += totalWidth; // Reset box position to the right
        }
    }
}

void scrollCarouselRight (
/*
    SYNOPSIS
        Scrolls the carousel to the right.

    DESCRIPTION
        Moves each box in the carousel to the right. It finds the currently selected box,
        then adjusts the positions of all boxes accordingly. If a box moves past the right edge
        of the carousel, it wraps around to the left.

    EXAMPLE
        Box boxes[NUM_BOXES];
        initializeBoxes(boxes);
        scrollCarouselRight(boxes);

        Initializes a carousel of boxes and then scrolls it to the right.
*/
    // Array of 'Box' structures representing the boxes in the carousel
    Box* boxes
) {
    int selectedIndex = -1; // Variable to hold the index of the selected box

    // Find the selected box
    for (int i = 0; i < NUM_BOXES; i++) {
        if (abs(boxes[i].x + boxes[i].width / 2 - TOP_SCREEN_WIDTH / 2) < SELECTION_THRESHOLD) {
            selectedIndex = i; // Assign the index of the selected box
            break;
        }
    }

    // Calculate the target position for the box to the right of the selected one
    float target = boxes[(selectedIndex + 1) % NUM_BOXES].x;

    // Move each box in the carousel to the right
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].x += SCROLL_SPEED; // Scroll each box to the right
        // Wrap around if a box moves past the right edge
        if (boxes[i].x > totalWidth - BOX_WIDTH) {
            boxes[i].x -= totalWidth; // Reset box position to the left
        }
    }
}

// Execute the program
int main (
/*
    SYNOPSIS
    Entry point of the program.

    DESCRIPTION
    Initializes the necessary libraries and resources, creates the carousel of boxes,
    handles user inputs for interactions like scrolling the carousel or selecting a box,
    and renders the carousel on the screen. It runs in a loop until the user decides to exit.

    EXAMPLE
    int main(int argc, char* argv[]) {
        // The main function is automatically called when the program starts.
    }
*/
    int argc, 
    char* argv[]
) {
    // Initialize graphics libraries
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Uncomment for debugging
    // consoleInit(GFX_BOTTOM, NULL);

    // Create render targets for the top and bottom screens
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize an array of boxes for the carousel
    Box boxes[NUM_BOXES];
    initializeBoxes(boxes);

    // Initialize the text buffer for the bottom screen
    C2D_TextBuf bottomScreenTextBuffer = C2D_TextBufNew(4096);

    // Main application loop
    while (aptMainLoop()) {
        // Scan the current input state
        hidScanInput();

        // Get the state of buttons just pressed, held, or released
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        // Scroll carousel left or right based on input
        if (kHeld & KEY_DRIGHT) {
            scrollCarouselLeft(boxes);
        } else if (kHeld & KEY_DLEFT) {
            scrollCarouselRight(boxes);
        }

        // Begin rendering the top screen
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, GLOBAL_BACKGROUND_COLOR);
        C2D_SceneBegin(top);

        // Draw the carousel and get the selected box's UID
        int selectedUID = drawCarouselTop(boxes);

        // Begin rendering the bottom screen
        C2D_SceneBegin(bot);
        C2D_TargetClear(bot, GLOBAL_BACKGROUND_COLOR);

        // Check for selected box and draw the bottom carousel
        checkSelectedBoxReachedTarget(boxes, NUM_BOXES, &target);
        drawCarouselBottom(boxes, bottomScreenTextBuffer);

        // Launch game if 'A' button is pressed
        if (kHeld & KEY_A) {
            launchTitle(selectedUID);
        }

        // Exit the application if 'START' button is pressed
        if (kDown & KEY_START) {
            break;
        }

        // End the frame
        C3D_FrameEnd(0);
    }

    // Clean up and deinitialize libraries
    C2D_TextBufDelete(bottomScreenTextBuffer);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
