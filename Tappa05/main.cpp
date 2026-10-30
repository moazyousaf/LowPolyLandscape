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

//Altezza del terreno in un punto (x, z)
float terrainHeight(float x, float z){
    return sin(x * 3.0f) * cos(z * 3.0f) *0.3f;
}

//Punto 3D della griglia alla posizione (x, z) in indici interi
glm::vec3 gridPoint(int x, int z){
    float fx = (float)x / (N-1) * 2.0f - 1.0f;
    float fz = (float)z / (N-1) * 2.0f - 1.0f;
    return glm::vec3(fx, terrainHeight(fx, fz), fz);
}

//Aggiunge un triangolo al vettore: 3 vertici, ognuno con posizione, colore e normale
void addTriangle(std::vector<float>&v, glm::vec3 a, glm::vec3 b, glm::vec3 c){
    
    //La normale della faccia: prodotto vettoriale di due lati
    glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));

    glm::vec3 pts[3] = { a, b, c};
    for(int i = 0; i < 3; i++){
        glm::vec3 col = altitudeColor(pts[i].y);
        v.push_back(pts[i].x); v.push_back(pts[i].y); v.push_back(pts[i].z);
        v.push_back(col.r); v.push_back(col.g); v.push_back(col.b);
        v.push_back(normal.x); v.push_back(normal.y); v.push_back(normal.z); 
    }
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
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            glm::vec3 tl = gridPoint(x, z);
            glm::vec3 tr = gridPoint(x + 1, z);
            glm::vec3 bl = gridPoint(x, z + 1);
            glm::vec3 br = gridPoint(x + 1, z + 1);
            
            addTriangle(vertices, tl, bl, tr); //triangolo 1
            addTriangle(vertices, tr, bl, br); //triangolo 2
            
        }
    }
    int vertexCount = (int)(vertices.size() / 9); //9 float per vertice

    //VAO + VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 
                    vertices.size() * sizeof(float),
                    vertices.data(), GL_STATIC_DRAW);
        
        //Attributo 0: posizione
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        //Attributo 1: colore
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        //Attributo 2: normale
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
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

    int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");

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
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        
        window.display();
    }

    //Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    return 0;
}