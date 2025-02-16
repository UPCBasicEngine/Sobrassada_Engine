# Sobrassada_Engine
SecondAssignment repository for the basic game engine of UPC Master Degree.

Our engine provides a robust scene editor for creating and manipulating game objects and components, while also ensuring high-quality asset rendering without compromising performance. This balance between visual fidelity and efficient workflow forms the core of our engine’s functionality.

Link to GitHub Repository: [https://github.com/UPCBasicEngine/Sobrassada_Engine](https://github.com/UPCBasicEngine/Sobrassada_Engine)
 
## How to use
### Running the Engine
1. Download the latest release from GitHub repository.
2. Unzip the release package into your directory.
3. Execute **SobrassadaEngine.exe**, located in SobrassadaEngine directory.

### Main Menus
Inside the Engine, you have diferent types of menu, depenging on the functionalities needed to be used.
- **File**: Handles file-related tasks, including opening/saving scenes and assets, and importing resources.
- **General**: Displays essential engine information, featuring real-time console output for debugging and a Quadtree view to optimize object distribution in the scene.
- **Window**: Provides tools for scene manipulation, allowing you to create, edit, or remove game objects and their components. It also gives quick access to transform properties such as position, rotation, and scale.
- **Settings**: Collects all major engine configurations, covering performance metrics, window properties, camera options, and OpenGL settings.

### Controls
The following list has all the inputs required in order to move the camera through the 3D space in the scene:
- `W/S/A/D keys` : Move the camera forward, backward, left, or right, respectively.
- `Hold RIGHT mouse button` : Enables free-look (rotate the camera view).
- `Hold RIGHT mouse button + ALT key + UP/DOWN mouse movement` : Zoom the camera in or out.
- `Hold SHIFT key` : Doubles camera's movement speed.

### Editing the Scene
- **Add/Remove GameObjects**: Use the Add/Delete GameObject button from hierarchy panel or context menu (right click on gameObject from hierarchy) to create/delete a gameObject.
- **Add/Remove Components**: From the inspector window, when a GameObject is selected from the hierarchy, select "Add component" (mesh renderer, directional light, etc.) or "Remove Component" to remove an existing one.
- **Transformations**: Each GameObject has a local and global transform. You can edit its position, rotation and scale.

## Additional functionality

## License
This Engine is available under the **MIT License**. For more information, visit [choosealicense.com](https://choosealicense.com/licenses/mit/).
