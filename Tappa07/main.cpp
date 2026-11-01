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
layout(location = 2) in vec3 aNormal;
uniform mat4 MVP;
out vec3 fragColor;
out vec3 fragNormal;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
    fragColor = aColor;
    fragNormal = aNormal;
}
)";

//Fragment Shader
const char* fragSrc = R"(
#version 410 core
in vec3 fragColor;
in vec3 fragNormal;
uniform vec3 lightDir; //Direzione in cui viaggia la luce
out vec4 outColor;
void main() {
    vec3 n = normalize(fragNormal);

    float ambient = 0.2;
    float diffuse = max(dot(n, -lightDir), 0.0);

    outColor = vec4(fragColor * (ambient + diffuse), 1.0);
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

    //Tutta la faccia con un solo colore preso dall'altezza
    float avgHeight = (a.y + b.y + c.y) / 3.0f;
    glm::vec3 col = altitudeColor(avgHeight);

    glm::vec3 pts[3] = { a, b, c};
    for(int i = 0; i < 3; i++){
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
        "Low-Poly Landscape - Tappa 7",
        sf::Style::Close | sf::Style::Titlebar,
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
    
    //Stato della camera
    glm::vec3 cameraPos(0.0f, 2.0f, 2.5f);
    float yaw = -90.0f; //gradi:  -90 = guarda verso -Z
    float pitch = -40.f; //gradi: negativo = guarda verso il basso 

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );
    
    unsigned int shaderProgram = createProgram();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    //Aggiunta wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");

    int lightLoc = glGetUniformLocation(shaderProgram, "lightDir");
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.7f, -1.0f, -0.3f));

    glUseProgram(shaderProgram);
    glUniform3fv(lightLoc, 1, glm::value_ptr(lightDir));

    sf::Clock clock;
    float speed = 1.5f; //al secondo

    //cursore
    window.setMouseCursorVisible(false);
    sf::Vector2i center(400, 300);
    sf::Mouse::setPosition(center, window);
    float sensitivity = 0.05f;

    //Loop principale
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Movimento con frecce
        float rotSpeed = 60.f; //gradi al secondo
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) yaw -= rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) yaw += rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) pitch += rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) pitch -= rotSpeed * dt;

        //Mouse look
        if(window.hasFocus()){
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            yaw += (mouse.x - center.x) * sensitivity;
            pitch -= (mouse.y - center.y) * sensitivity; //y dello schermo va verso il basso
            sf::Mouse::setPosition(center, window);
        }

        //Limiti di pitch
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        //Direzione in cui guarda la camera, ricavata da yaw e pitch
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

        //Movimento da tastiera con dt
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cameraPos += front * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cameraPos -= front * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cameraPos += right * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cameraPos -= right * speed * dt;

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0, 1, 0));
        glm::mat4 MVP = projection * view * model;

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