# slipstream Launcher

## Description
This Nintendo 3DS application provides a carousel interface for selecting and launching games. It utilizes Citro2D and Citro3D libraries for rendering 2D graphics on the console. The application showcases a dynamic carousel with game box arts, names, and descriptions.

## Features
- Carousel-style interface for game selection.
- Display of game box art, name, and description.
- Smooth scrolling animation for carousel navigation.
- Support for launching games directly from the interface.

## Prerequisites
- A Nintendo 3DS console with homebrew capabilities.
- Development libraries: `citro2d`, `citro3d`, and `lodepng` for image processing.
- A basic understanding of C programming and Nintendo 3DS homebrew development.

## Building and Running
To build and run this application, follow these steps:

1. **Setup Development Environment**: Ensure that your 3DS development environment is set up with `devkitPro`, `citro2d`, and `citro3d`.
2. **Clone the Repository**: Clone this repository to your local machine.
   ```bash
   git clone https://github.com/BlackDelta95/slipstream
3. **Build the application**: Navigate to the cloned directory and run the `make` command.
4. **Transfer to 3DS**: After successful build, transfer the generated `.3dsx` file to your 3DS's SD card. Also transfer the `images` folder to the same directory as the `.3dsx` file as well.
5. **Run the Application**: Use a homebrew launcher to run the application on your 3DS.

## Usage
Use the D-pad to navigate through the carousel.
Press 'A' to launch the selected game.
Press 'START' to exit the application.

## Contributing
Contributions to this project are welcome. Please adhere to the following guidelines:

1. Fork the repository and create a new branch for your feature or fix.
2. Write clean, documented, and well-tested code.
3. Submit a pull request with a clear description of your changes.

## License
This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
Thanks to the citro2d and citro3d contributors.
Special thanks to the Nintendo 3DS homebrew community.

## Disclaimer
This application is a homebrew project and is not affiliated with or endorsed by Nintendo.