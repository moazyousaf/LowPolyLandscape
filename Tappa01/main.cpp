#include <SFML/Window.hpp>
#include <glad/glad.h>
#include <iostream>

//Vertex Shader
const char* vertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 fragColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    fragColor = aColor;
}
)";

//Fragment Shader
const char* fragSrc = R"(
#version 410 core
in vec3 fragColor;
out vec4 outColor;
void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

//Compila un shader e controlla errori
unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Errore shader: " << log << std::endl;
    }
    return shader;
}

//Crea il programma shader (vertex + fragment)
unsigned int createProgram() {
    unsigned int vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

int main() {
    //Crea finestra SFML con contesto OpenGL 4.1 Core
    sf::ContextSettings settings;
    settings.depthBits      = 24;
    settings.stencilBits    = 8;
    settings.majorVersion   = 4;
    settings.minorVersion   = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::Window window(
        sf::VideoMode({800u, 600u}),
        "Low-Poly Landscape - Tappa 1",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );
    window.setVerticalSyncEnabled(true);

    //Inizializza glad
    if (!gladLoadGL()) {
        std::cerr << "Errore: GLAD non inizializzato" << std::endl;
        return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    //Dati del triangolo: posizione (x,y,z) + colore (r,g,b)
    float vertices[] = {
       //  x       y      z      r     g     b
       -0.5f,  -0.5f,  0.0f,  1.0f, 0.0f, 0.0f,  //basso sinistra - rosso
        0.5f,  -0.5f,  0.0f,  0.0f, 1.0f, 0.0f,  //basso destra   - verde
        0.0f,   0.5f,  0.0f,  0.0f, 0.0f, 1.0f,  //alto centro    - blu
    };

    //VAO + VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        //Attributo 0: posizione (primi 3 float)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        //Attributo 1: colore (secondi 3 float)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int shaderProgram = createProgram();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    //Loop principale
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.display();
    }

    //Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    return 0;
}