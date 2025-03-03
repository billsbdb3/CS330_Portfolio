# CS330_Portfolio

# 3D Scene Project Reflection

This document reflects on the design and development of a 3D scene created using OpenGL, GLFW, and C++. The scene simulates a stylized desktop workspace.

## Design Decisions and Reflection

### How do I approach designing software? What new design skills has your work on the project helped you to craft?

My approach to software design involves several key stages: understanding the project requirements, breaking the problem down into smaller components, designing appropriate data structures and classes, and planning the algorithms needed. Before this project, my experience with object-oriented programming was limited. This project solidified my understanding of using classes (`SceneManager`, `ShaderManager`, `ShapeMeshes`) to organize code effectively.

New design skills I crafted include:

*   **3D Graphics Concepts:** A deeper understanding of 3D coordinate systems, transformations (translation, rotation, scaling), and perspective projection.
*   **Modular Design:**  I learned the importance of separating concerns into different classes and functions, making the code more maintainable and reusable.
*   **Interface Design:**  I gained a better appreciation for designing clear interfaces between different parts of the code (e.g., the interaction between `SceneManager` and `ShaderManager`).

### What design process did you follow for your project work?

My design process was highly iterative and incremental:

1.  **Initial Setup:**  GLFW window and OpenGL context.
2.  **Simple Object:** Rendering a single triangle, then a cube.
3.  **Camera:** Implementing camera controls.
4.  **Desk:** Adding the desk as a textured plane.
5.  **Monitor:** Constructing the monitor from multiple boxes.
6.  **Vase and Plants:** Creating the vase and stylized plants.
7.  **Keyboard and Mouse:** Adding these, which involved significant challenges with texture scaling.
8.  **Books:** Stacking simple box shapes.
9. **Organizer:** Multi-tiered design.
10. **Lighting:** Implementing basic directional and point lights.
11. **Materials:** Defining materials to give objects different appearances.
12. **Refactoring and Debugging:** Continuously cleaning up the code and fixing issues.

I tested thoroughly after each step, ensuring that existing functionality worked before adding new features.

### How could tactics from your design approach be applied in future work?

The iterative and modular design approach is broadly applicable to any software development task. Breaking down complex problems, starting with a simple working prototype, and testing frequently are fundamental principles. The 3D graphics skills are directly useful in future graphics, game development, or simulation projects.

### How do I approach developing programs? What new development strategies did you use while working on your 3D scene?

My development approach emphasizes planning, incremental implementation, and frequent testing. I try to start with a clear design, but I'm also prepared to adapt the design as I encounter challenges.

New development strategies I used in this project include:

*   **Using External Libraries:** Integrating GLFW, GLEW, GLM, and stb_image.
*   **Debugging OpenGL:**  Using `glGetError` and other techniques to diagnose rendering issues.
*   **Shader Debugging:** Using print statements (with limitations) and carefully examining output to understand shader behavior.
*   **Abstraction:** Creating higher-level classes and functions to hide low-level OpenGL details.

### How did iteration factor into your development?

Iteration was absolutely essential. Almost nothing worked perfectly the first time.  The keyboard texture, lighting, object positioning, and collision detection all required multiple iterations of debugging, adjusting code, and retesting.  This iterative process was crucial for identifying and fixing errors.

### How has your approach to developing code evolved throughout the milestones?

My approach has become more structured. I've learned to:

*   **Plan More Thoroughly:** Spend more time planning before coding.
*   **Break Down Problems:** Decompose problems into smaller, independent components.
*   **Test More Frequently:** Test after *every* small change.
*   **Isolate Issues:** Simplify the code to isolate the source of problems.
*   **Read Documentation:** Carefully read documentation for libraries and APIs.
*   **Use Debugging Tools:** Effectively use print statements and (ideally) a debugger.
*   **Prioritize Readability:** Write cleaner, well-commented code.

### How can computer science help me in reaching my goals? How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future educational pathway?

Computer science provides the foundational knowledge and skills needed for many technology careers. This project specifically provides a strong base for further studies in computer graphics, game development, scientific visualization, and related fields. It also demonstrates how computer science concepts can be applied to create interactive visual applications.

### How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future professional pathway?

The skills are applicable to:

*   **Game Development:** Directly relevant to roles involving 3D graphics programming.
*   **Simulation and Modeling:** Useful for creating simulations in various industries.
*   **Data Visualization:**  3D visualizations can communicate complex data effectively.
*   **Virtual/Augmented Reality:**  Fundamental principles of 3D graphics are essential for VR/AR development.
*   **Software Engineering:** The problem-solving, design, and programming skills are valuable in *any* software engineering role.

This project has significantly increased my understanding of computer graphics and given me a foundation for future learning. I plan to continue exploring OpenGL, GLSL, and related technologies.
