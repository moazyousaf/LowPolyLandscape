#include <SFML/Window.hpp>
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

const int N = 20; //griglia 20x20

//Vertex Shader
const char* vertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 MVP;
out vec3 fragColor;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
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

//Restituisce un colore RGB in base all'altezza h
glm::vec3 altitudeColor(float h){
    if(h < -0.05f) return glm::vec3(0.1f, 0.3f, 0.8f); //Acqua blu
    if(h < 0.05f) return glm::vec3(0.8f, 0.7f, 0.4f); //Sabbia giallo
    if(h < 0.20f) return glm::vec3(0.2f, 0.6f, 0.2f); //Erba verde

    return glm::vec3(0.9f, 0.9f, 0.9f); //Neve bianco
    
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
        "Low-Poly Landscape - Tappa 4",
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

    //Abilitazione depth test per la pulizia del buffer
    glEnable(GL_DEPTH_TEST);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    //Generazione di vertici della griglia (con altezza variabile)
    std::vector<float> vertices;
    for(int z = 0; z < N; z++){
        for(int x = 0; x < N; x++){
            float fx = (float)x / (N-1) * 2.0f - 1.0f; //da -1 a +1
            float fz = (float)z / (N-1) * 2.0f - 1.0f;
            float h = sin(fx * 3.0f) * cos(fz * 3.0f) *0.3f;

            glm::vec3 col = altitudeColor(h);

            //Posizione
            vertices.push_back(fx);
            vertices.push_back(h);
            vertices.push_back(fz);

            //Colore
            vertices.push_back(col.r);
            vertices.push_back(col.g);
            vertices.push_back(col.b);
        }
    }

    //Generazione indici (2 triangoli per cella)
    //  tl---tr
    //  | \  |
    //  |  \ |
    //  bl---br
    std::vector<unsigned int> indices;
    for (int z = 0; z < N-1; z++) {
        for (int x = 0; x < N-1; x++) {
            unsigned int tl = z*N + x;
            unsigned int tr = z*N + x+1;
            unsigned int bl = (z+1)*N + x;
            unsigned int br = (z+1)*N + x+1;

            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);

            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    //VAO + VBO + EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 
                    vertices.size() * sizeof(float),
                    vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                    indices.size() * sizeof(unsigned int),
                    indices.data(), GL_STATIC_DRAW);
        
        //Attributo 0: posizione (primi 3 float)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        //Attributo 1: colore (secondi 3 float con offset di 3)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    //Costruzione matrici
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 2.0f, 2.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    //Matrice combinata
    glm::mat4 MVP = projection * view * model;
    

    unsigned int shaderProgram = createProgram();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    //Aggiunta wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
        int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        //Disegna i triangoli usando i vertici generati
        glDrawElements(GL_TRIANGLES, (int)indices.size(), GL_UNSIGNED_INT, 0);

        window.display();
    }

    //Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    return 0;
}